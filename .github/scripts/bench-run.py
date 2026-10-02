#!/usr/bin/env python3
"""bench_test.exe を回し、今のコミット (head) と比較対象 (base) の速さを比べる。

.github/workflows/ci.yml の build ジョブ (Windows ランナー) から呼ばれる。やること:
  1. base (比較対象の exe) を用意する。--base auto なら、master の CI が上げた
     アーティファクト bench-exe-<構成> を gh で探して落とす。
       master       : この実行より前の master の実行のうち、直前のもの (previous-master)。
                      古い実行を再実行しても、それより新しい master とは比べない
       ほかのブランチ: 分岐元 (merge-base)。無ければ最新の master (latest-master)
     見つからない・落とせないときは警告だけ出して base なしで続ける。base が無い
     ことはジョブを落とす理由にならない (初回やアーティファクトの期限切れで普通に起きる)。
     分岐元や実行番号が取れないだけで base は使えるときは、notice を出して続ける
  2. head と base を同じ VM で交互に R ラウンド回す (偶数ラウンドは head → base、
     奇数ラウンドは base → head)。GitHub のランナーは VM ごとに CPU が違い、同じ VM
     でも時間とともに速さが揺れる。別々の実行の数字を並べても 5 % 程度の差は揺れに
     埋もれるので、同じ VM で交互に測ってラウンドごとの比 (head / base) を見る
  3. 比からブートストラップで 95 % 信頼区間を出し、悪化・改善を判定する
     (判定の中身は bench_common.py)
  4. <out-dir>/result.json (スキーマ nox-bench-run/1) を書き、ジョブのサマリーに表を足す

--startup-exe に runtime.exe を渡すと、起動の速さも同じ仕組みで比べる。各ラウンドで bench_test の
後に runtime.exe を --startup-launches 回 (既定 3) 起動し、1 フレーム回して終了させる。runtime.exe が
書く起動レポート (--startup-report、スキーマ nox-startup/1) を区間に分け、startup/* のベンチマーク
として同じラウンドの結果に足す (区間の定義は bench_common.py の STARTUP_INTERVALS)。
  - 最初の 1 回は head / base とも捨てる。ディスクのキャッシュに載っていない冷えた起動は
    桁が変わることがあり、ラウンド 1 だけが外れ値になるため
  - base の runtime.exe は bench_test.exe と同じアーティファクトから取る。起動レポートに対応する
    前のコミットや、アーティファクトに runtime.exe が無いときは、起動だけ比べずに head を測る
  - 起動の失敗では CI を落とさない (起動・終了そのものは ci.yml の別のステップが確かめている)。
    確保回数の予算 (STARTUP_ALLOC_BUDGETS) を超えたときは bench と同じく終了コード 3

時間の変化では CI を落とさない (報告だけ)。ヒープ確保の予算 (回数は決定的に数えられる)
を超えたときだけ失敗にする。

使い方:
    python .github/scripts/bench-run.py --exe runtime/build/x64/Master/bench_test.exe \\
        --compiler MSVC --config Master --out-dir bench-out --summary $GITHUB_STEP_SUMMARY
    python .github/scripts/bench-run.py --exe ... --compiler MSVC --config Debug --smoke
    手元で 2 つの exe を比べる:
    python .github/scripts/bench-run.py --exe new.exe --base-exe old.exe --compiler MSVC --config Release

<out-dir> の中身:
    result.json   結果 (アーティファクト bench-result-<構成> として上げるもの)
    raw/          exe が書いた生の JSON (head-<r>.json / base-<r>.json)
    _base/        --base auto で落とした base の exe (上げなくてよい)

--base auto が読む環境変数: GH_TOKEN, GITHUB_REPOSITORY, GITHUB_REF_NAME, GITHUB_SHA, GITHUB_RUN_ID
結果に書く環境変数: GITHUB_RUN_NUMBER, GITHUB_RUN_ATTEMPT, ImageOS, ImageVersion, RUNNER_ENVIRONMENT

終了コード: 0 = 成功、1 = head の exe が失敗した・結果が 0 件・結果を書けなかった、
2 = 引数の誤り、3 = ヒープ確保の予算を超えたベンチマークがある (スモークでは確かめない)。
"""

import argparse
import glob
import json
import os
import re
import shutil
import stat
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bench_common as bc  # noqa: E402

# 1 回の exe 実行の上限 (既定)。全ベンチマークで数十秒の想定なので、10 分かかるなら固まっている
EXE_TIMEOUT = 600
# gh の 1 回の呼び出しの上限 (exe の取得を含む)
GH_TIMEOUT = 300
# runtime.exe を 1 回起動して終わるまでの上限。普段は 1 秒程度
STARTUP_TIMEOUT = 120
# runtime.exe に回させるフレーム数。1 フレーム目を終えたところが計測の終点なので 1 で足りる
STARTUP_FRAMES = 1
# 失敗したときにログへ出す行数
LOG_TAIL_LINES = 60


def warning(message):
    bc.annotate("warning", message)


def notice(message):
    bc.annotate("notice", message)


def error(message):
    bc.annotate("error", message)


def decode(data):
    # exe のログは std::wcout 経由なので、コードページ次第で UTF-8 でないことがある
    return (data or b"").decode("utf-8", errors="replace")


def tail(text, lines=LOG_TAIL_LINES):
    rows = text.rstrip().splitlines()
    return "\n".join(rows[-lines:])


def print_group(title, text):
    if not text.strip():
        return
    print(f"::group::{title}")
    print(text)
    print("::endgroup::", flush=True)


def env_int(name):
    try:
        return int(os.environ.get(name, ""))
    except ValueError:
        return None


# ---------------------------------------------------------------------------
# gh (base の exe の取得)
# ---------------------------------------------------------------------------

def gh_failed(what, detail, soft):
    """gh の失敗を注釈にする。

    soft=False: base を使えなくなる失敗 (一覧・取得)。::warning:: で「base を用意できない」
    soft=True : base は使えるまま続く失敗 (分岐元の特定・実行番号の取得)。::notice:: で、
                what に「どうなるか」まで書いた文面を渡す ("{detail}" を理由で埋める)
    """
    if soft:
        notice(what.format(detail=detail))
    else:
        warning(f"base を用意できない ({what}): {detail}")


def gh(args, what, soft=False):
    """gh を 1 回呼び、標準出力を返す。失敗したら注釈 (gh_failed) を出して None を返す。

    base の用意は「できれば」の処理なので、どこで失敗しても例外にしない。呼び出し側は
    None を見て base なしに切り替える (soft の呼び出しは代わりの手で続ける)。gh の呼び出しは
    必ずここを通す。
    """
    cmd = ["gh"] + [str(a) for a in args]
    try:
        proc = subprocess.run(cmd, stdin=subprocess.DEVNULL, capture_output=True, timeout=GH_TIMEOUT)
    except FileNotFoundError:
        gh_failed(what, "gh が見つからない", soft)
        return None
    except subprocess.TimeoutExpired:
        gh_failed(what, f"gh が {GH_TIMEOUT} 秒で終わらない", soft)
        return None
    except OSError as e:
        gh_failed(what, f"gh を起動できない ({e})", soft)
        return None
    if proc.returncode != 0:
        err = decode(proc.stderr).strip().splitlines()
        detail = err[-1] if err else "(メッセージなし)"
        gh_failed(what, f"gh の終了コード {proc.returncode}: {detail}", soft)
        return None
    return decode(proc.stdout)


def gh_json(args, what, soft=False):
    text = gh(args, what, soft)
    if text is None:
        return None
    try:
        return json.loads(text)
    except ValueError as e:
        gh_failed(what, f"gh の出力が JSON でない ({e})", soft)
        return None


def to_int(x):
    """実行 id などを int に。数字でなければ None。"""
    try:
        return int(str(x))
    except (TypeError, ValueError):
        return None


def find_named(root, name):
    """root の下から name のファイルを探す (大文字小文字は区別しない)。無ければ None。"""
    wanted = name.lower()
    for path in sorted(glob.glob(os.path.join(root, "**", "*"), recursive=True)):
        if os.path.basename(path).lower() == wanted and os.path.isfile(path):
            return path
    return None


def find_exe(root):
    hits = sorted(glob.glob(os.path.join(root, "**", bc.BENCH_EXE_NAME), recursive=True))
    if not hits:
        # 名前が違っても exe が 1 つだけならそれを使う
        exes = sorted(glob.glob(os.path.join(root, "**", "*.exe"), recursive=True))
        hits = exes if len(exes) == 1 else []
    return hits[0] if hits else None


def resolve_base(names, out_dir):
    """アーティファクト bench-exe-<構成> から base の exe を探して落とす。

    names は探す名前の候補。構成名は小文字 (bench-exe-msvc-master) が正だが、ワークフロー
    の式には小文字化の関数が無く bench-exe-MSVC-Master で上げることもあるので両方を探し、
    名前は大文字小文字を区別せずに比べる。戻り値は (exe のパス, base 情報) か (None, None)。
    """
    repo = os.environ.get("GITHUB_REPOSITORY")
    sha = os.environ.get("GITHUB_SHA")
    ref = os.environ.get("GITHUB_REF_NAME")
    run_id = to_int(os.environ.get("GITHUB_RUN_ID"))
    if not (repo and sha and ref):
        print("GitHub Actions の外なので base を探さない (--base-exe で指定できる)")
        return None, None

    name = names[0]
    wanted = {n.lower() for n in names}
    artifacts = {}
    for query in dict.fromkeys(names):
        data = gh_json(["api", f"repos/{repo}/actions/artifacts?name={query}&per_page=100"], "アーティファクトの一覧")
        if not isinstance(data, dict):
            return None, None
        for a in data.get("artifacts") or []:
            artifacts[a.get("id")] = a
    candidates = []
    for a in artifacts.values():
        wr = a.get("workflow_run") or {}
        if (a.get("name") or "").lower() not in wanted or a.get("expired"):
            continue
        if wr.get("head_branch") != "master" or to_int(wr.get("id")) is None or not wr.get("head_sha"):
            continue
        if run_id is not None and to_int(wr["id"]) == run_id:
            continue
        # フォークからの実行はブランチ名が master でも別のコード。比較の基準にしない
        if wr.get("head_repository_id") and wr.get("repository_id") \
                and wr["head_repository_id"] != wr["repository_id"]:
            continue
        candidates.append(a)
    # 新しい実行から順に並べる。実行 id は実行を作った順に増え、再実行しても変わらない。
    # アーティファクトの created_at は再実行で新しくなるので、同じ実行の中の並びにだけ使う
    candidates.sort(key=lambda a: (to_int(a["workflow_run"]["id"]), a.get("created_at") or ""), reverse=True)
    if not candidates:
        warning(f"base にする {name} が無い (master の CI がまだ上げていないか、期限切れ)。比較せずに計測する")
        return None, None

    pick = None
    if ref == "master":
        kind = "previous-master"
        # 今回より前に作られた master の実行だけを候補にする。古い master の実行を再実行した
        # とき、それより新しい master を「直前」に選ぶと比較が逆向きになる (新しいコミットでの
        # 高速化が今回の悪化に見え、履歴の点と Discord の通知が誤る)
        pick = next((a for a in candidates
                     if a["workflow_run"]["head_sha"] != sha
                     and (run_id is None or to_int(a["workflow_run"]["id"]) < run_id)), None)
        if pick is None:
            warning(f"base にする {name} が無い (この実行より前の、別のコミットの master の結果が無い)。"
                    "比較せずに計測する")
            return None, None
    else:
        # ブランチは分岐元と比べる。特定できなくても最新の master と比べられるので notice にとどめる
        text = gh(["api", f"repos/{repo}/compare/master...{sha}", "--jq", ".merge_base_commit.sha"],
                  "分岐元を特定できない ({detail})。最新の master と比べる", soft=True)
        merge_base = text.strip().lower() if text else ""
        if text is not None and not re.fullmatch(r"[0-9a-f]{40}", merge_base):
            notice(f"分岐元を特定できない (応答が SHA でない: {merge_base[:60]!r})。最新の master と比べる")
            merge_base = ""
        if merge_base:
            pick = next((a for a in candidates if a["workflow_run"]["head_sha"] == merge_base), None)
        kind = "merge-base"
        if pick is None:
            # 分岐元の exe が期限切れなど。最新の master と比べる (差にはブランチ外の変更も混ざる)
            if merge_base:
                print(f"分岐元 {bc.short_sha(merge_base)} の {name} が無い (期限切れか、計測を始める前の"
                      "コミット)。最新の master と比べる")
            pick = candidates[0]
            kind = "latest-master"

    wr = pick["workflow_run"]
    dest = os.path.join(out_dir, "_base", str(wr["id"]))
    shutil.rmtree(dest, ignore_errors=True)
    os.makedirs(dest, exist_ok=True)
    if gh(["run", "download", str(wr["id"]), "-R", repo, "-n", pick["name"], "-D", dest], "base の exe の取得") is None:
        return None, None
    exe = find_exe(dest)
    if exe is None:
        warning(f"base を用意できない: {pick['name']} の中に {bc.BENCH_EXE_NAME} が無い")
        return None, None
    if os.name != "nt":
        # 手元 (Linux) の試験用。アーティファクトは実行属性を保たない
        os.chmod(exe, os.stat(exe).st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)

    # 実行番号は表示 (#123) にしか使わないので、取れなくても base はそのまま使う
    run_number = None
    run = gh_json(["api", f"repos/{repo}/actions/runs/{wr['id']}"],
                  "base の実行番号を取れない ({detail})。比較は続ける", soft=True)
    if isinstance(run, dict) and isinstance(run.get("run_number"), int) and not isinstance(run["run_number"], bool):
        run_number = run["run_number"]
    elif isinstance(run, dict):
        notice("base の実行番号を取れない (応答に run_number が無い)。比較は続ける")
    info = {"sha": wr["head_sha"], "run_id": to_int(wr["id"]), "run_number": run_number, "kind": kind}
    print(f"base: {bc.base_kind_label(kind)} {bc.short_sha(wr['head_sha'])} (run {wr['id']})")
    return exe, info


# ---------------------------------------------------------------------------
# exe の実行
# ---------------------------------------------------------------------------

def load_raw(path):
    """exe が書いた JSON を読む。(raw, 失敗理由)。"""
    try:
        raw = bc.read_json(path)
    except FileNotFoundError:
        return None, "--out の JSON が書かれていない"
    except (OSError, ValueError) as e:
        return None, f"JSON を読めない ({e})"
    if not isinstance(raw, dict) or raw.get("schema") != bc.RAW_SCHEMA:
        schema = raw.get("schema") if isinstance(raw, dict) else None
        return None, f"スキーマが {bc.RAW_SCHEMA} でない ({schema!r})"
    if not isinstance(raw.get("benchmarks"), list):
        return None, "benchmarks が配列でない"
    return raw, None


def describe_exit(code):
    if code is None:
        return "終了コードなし"
    if code < 0 or code > 255:
        # Windows の例外終了 (0xC0000005 など) は 16 進の方が調べやすい
        return f"終了コード {code} (0x{code & 0xFFFFFFFF:08X})"
    return f"終了コード {code}"


def run_exe(exe, out_path, smoke, extra, timeout=EXE_TIMEOUT):
    """1 回実行して raw JSON を読む。(raw, 終了コード, 失敗理由, ログ, 秒)。"""
    try:
        os.remove(out_path)
    except FileNotFoundError:
        pass
    cmd = [exe, "--out", out_path] + (["--smoke"] if smoke else []) + extra
    t0 = time.monotonic()
    try:
        proc = subprocess.run(cmd, stdin=subprocess.DEVNULL, capture_output=True, timeout=timeout)
    except subprocess.TimeoutExpired as e:
        log = "--- stderr ---\n" + tail(decode(e.stderr)) + "\n--- stdout ---\n" + tail(decode(e.stdout))
        return None, None, f"{timeout:g} 秒で終わらない", log, time.monotonic() - t0
    except OSError as e:
        return None, None, f"起動できない ({e})", "", time.monotonic() - t0
    elapsed = time.monotonic() - t0
    # stderr は進捗、stdout はエンジンのログ
    log = "--- stderr ---\n" + tail(decode(proc.stderr)) + "\n--- stdout ---\n" + tail(decode(proc.stdout))
    if proc.returncode not in (0, 3):
        return None, proc.returncode, describe_exit(proc.returncode), log, elapsed
    raw, reason = load_raw(out_path)
    if raw is None:
        return None, proc.returncode, reason, log, elapsed
    return raw, proc.returncode, None, log, elapsed


def run_startup(exe, report_path, timeout=STARTUP_TIMEOUT):
    """runtime.exe を 1 回起動し、起動レポートを読む。(parse_startup_report の結果, 失敗理由, ログ)。

    作業ディレクトリは exe の置き場所 (Editor の Core.RuntimeSession、ci.yml の起動確認と同じ)。
    """
    try:
        os.remove(report_path)
    except FileNotFoundError:
        pass
    cmd = [exe, f"--exit-after-frames={STARTUP_FRAMES}", f"--startup-report={report_path}"]
    try:
        proc = subprocess.run(cmd, cwd=os.path.dirname(exe), stdin=subprocess.DEVNULL,
                              capture_output=True, timeout=timeout)
    except subprocess.TimeoutExpired:
        return None, f"{timeout:g} 秒で終わらない", ""
    except OSError as e:
        return None, f"起動できない ({e})", ""
    log = "--- stderr ---\n" + tail(decode(proc.stderr)) + "\n--- stdout ---\n" + tail(decode(proc.stdout))
    if proc.returncode != 0:
        return None, describe_exit(proc.returncode), log
    try:
        report = bc.parse_startup_report(bc.read_json(report_path))
    except FileNotFoundError:
        return None, "起動レポートが書かれていない (--startup-report に対応していない exe)", log
    except (OSError, ValueError) as e:
        return None, f"起動レポートを読めない ({e})", log
    return report, None, log


class StartupRunner:
    """head / base の runtime.exe を起動して、ラウンドごとの startup/* の項目を作る。

    どちらかが失敗したら、その側の起動の計測をそこでやめる (bench_test の計測は続ける)。
    base だけが欠けたラウンドは対にならないので、比較から自然に外れる。
    """

    def __init__(self, exes, launches, raw_dir, smoke):
        self.exes = dict(exes)
        self.launches = launches
        self.raw_dir = raw_dir
        self.smoke = smoke
        self.problems = []

    def enabled(self, side):
        return self.exes.get(side) is not None

    def _disable(self, side, reason, log):
        self.exes[side] = None
        message = f"{side} の runtime.exe の起動を測れない ({reason})。以降は測らない"
        if side == "head":
            self.problems.append(f"⚠️ {message}")
            warning(message)
        else:
            # 起動レポートに対応する前の base (master) では普通に起きる
            self.problems.append(f"ℹ️ {message}。起動は比べずに head だけ載せている")
            notice(message)
        print_group(f"{side} の runtime.exe の出力 (末尾)", log)

    def warm_up(self):
        """冷えた起動を 1 回ずつ捨てる。base が起動レポートに対応しているかもここで分かる。"""
        for side in ("head", "base"):
            if not self.enabled(side):
                continue
            report, reason, log = run_startup(self.exes[side], os.path.join(self.raw_dir, f"startup-{side}-warmup.json"))
            if report is None:
                self._disable(side, f"準備の起動: {reason}", log)

    def measure(self, side, r):
        """1 ラウンドぶん起動し、startup/* の項目の列を返す。測れなければ空。"""
        if not self.enabled(side):
            return []
        reports = []
        t0 = time.monotonic()
        for i in range(self.launches):
            path = os.path.join(self.raw_dir, f"startup-{side}-{r}-{i}.json")
            report, reason, log = run_startup(self.exes[side], path)
            if report is None:
                self._disable(side, f"ラウンド {r + 1} の {i + 1} 回目: {reason}", log)
                return []
            reports.append(report)
        entries = bc.startup_benchmarks(reports, check_budget=not self.smoke)
        print(f"    {side}: runtime.exe を {len(reports)} 回起動, {time.monotonic() - t0:.1f} 秒", flush=True)
        return entries


# ---------------------------------------------------------------------------
# 集計
# ---------------------------------------------------------------------------

def collect(raws):
    """ベンチマーク名 → ラウンドごとの raw 項目 (そのラウンドに無ければ None)。名前は初出順。"""
    order = []
    per = {}
    for i, raw in enumerate(raws):
        for b in raw.get("benchmarks") or []:
            name = b.get("name") if isinstance(b, dict) else None
            if not isinstance(name, str) or not name:
                continue
            if name not in per:
                per[name] = [None] * len(raws)
                order.append(name)
            per[name][i] = b
    return order, per


def round_medians(entries):
    return [bc.median(bc.finite(e.get("samples_ns"))) if e and bc.finite(e.get("samples_ns")) else None
            for e in entries]


def side_stats(entries):
    present = [e for e in entries if e]
    samples = [bc.finite(e.get("samples_ns")) for e in present]
    cycles = [c for e in present for c in bc.finite(e.get("cycles_per_op"))]
    return bc.summarize_side(samples, cycles)


def rounded_side(s):
    out = {"rounds": [bc.sig_round(x, 6) for x in s["rounds"]]}
    for k in ("median", "q1", "q3", "p10", "p90", "min", "cycles"):
        out[k] = bc.sig_round(s[k], 6)
    out["cv"] = bc.dec_round(s["cv"], 5)
    return out


def num(x):
    return float(x) if bc.is_num(x) else None


def alloc_stats(head_entries, base_entries, smoke):
    """確保回数は決定的なので head の最初のラウンドの値を使い、全ラウンドで一致するかを見る。"""
    present = [e for e in head_entries if e]
    first = present[0]
    keys = ("allocs_per_op", "alloc_bytes_per_op", "frees_per_op")
    signature = [tuple(bc.dec_round(e.get(k), 6) for k in keys) for e in present]
    stable = all(s == signature[0] for s in signature) and all(e.get("alloc_stable") is not False for e in present)
    if smoke:
        # Debug のスモークは予算を確かめない (exe も確かめない)
        budget_ok = None
    elif any(e.get("budget_ok") is False for e in present):
        budget_ok = False
    elif any(e.get("budget_ok") is True for e in present):
        budget_ok = True
    else:
        budget_ok = None
    base_first = next((e for e in base_entries if e), None)
    base_allocs = num(base_first.get("allocs_per_op")) if base_first else None
    base_bytes = num(base_first.get("alloc_bytes_per_op")) if base_first else None
    allocs = num(first.get("allocs_per_op"))
    return {
        "allocs": bc.sig_round(allocs, 6),
        "bytes": bc.sig_round(num(first.get("alloc_bytes_per_op")), 6),
        "frees": bc.sig_round(num(first.get("frees_per_op")), 6),
        "stable": stable,
        "budget": first.get("alloc_budget"),
        "budget_ok": budget_ok,
        "base_allocs": bc.sig_round(base_allocs, 6),
        "base_bytes": bc.sig_round(base_bytes, 6),
        "verdict": bc.alloc_verdict(allocs, base_allocs) if base_first else None,
    }


def build_benchmarks(head_raws, base_raws, threshold, smoke):
    order, head_per = collect(head_raws)
    _, base_per = collect(base_raws)
    out = []
    for name in order:
        head_entries = head_per[name]
        first = next(e for e in head_entries if e)
        threads = first.get("threads") if isinstance(first.get("threads"), int) else 1
        base_entries = base_per.get(name) or []
        has_base = any(base_entries)
        entry = {
            "name": name,
            "group": first.get("group") or name.split("/", 1)[0],
            "title": first.get("title") or "",
            "per": first.get("per") or "op",
            "threads": threads,
            "alloc_budget": first.get("alloc_budget"),
            "head": rounded_side(side_stats(head_entries)),
            "base": rounded_side(side_stats(base_entries)) if has_base else None,
            "paired": None,
        }
        if has_base:
            # ラウンド番号で対にする (同じラウンド = 同じ VM の同じ時間帯)
            p = bc.paired_analysis(name, round_medians(head_entries), round_medians(base_entries),
                                   threshold, threads)
            entry["paired"] = {
                "ratio": bc.dec_round(p["ratio"], 6),
                "ci_low": bc.dec_round(p["ci_low"], 6),
                "ci_high": bc.dec_round(p["ci_high"], 6),
                "change": bc.dec_round(p["change"], 6),
                "verdict": p["verdict"],
                "pairs": p["pairs"],
            }
        entry["alloc"] = alloc_stats(head_entries, base_entries, smoke)
        out.append(entry)
    return out


def summarize(benchmarks):
    s = {v: 0 for v in bc.VERDICTS}
    ratios = []
    for b in benchmarks:
        p = b.get("paired")
        if p:
            s[p["verdict"]] += 1
            if p["verdict"] != "insufficient" and p.get("ratio") and b.get("group") not in bc.GEOMEAN_EXCLUDED_GROUPS:
                ratios.append(p["ratio"])
    s["geomean_change"] = bc.dec_round(bc.geomean_change(ratios), 6)
    s["alloc_regressed"] = sum(1 for b in benchmarks if (b.get("alloc") or {}).get("verdict") == "regressed")
    s["budget_violations"] = sum(1 for b in benchmarks if (b.get("alloc") or {}).get("budget_ok") is False)
    return s


# ---------------------------------------------------------------------------
# コミットと環境
# ---------------------------------------------------------------------------

def git_out(args):
    try:
        proc = subprocess.run(["git"] + args, stdin=subprocess.DEVNULL, capture_output=True, timeout=60)
    except (OSError, subprocess.TimeoutExpired):
        return None
    if proc.returncode != 0:
        return None
    return decode(proc.stdout).strip()


def commit_info():
    sha_env = os.environ.get("GITHUB_SHA")
    text = git_out(["log", "-1", "--format=%H%x1f%s%x1f%cI", sha_env or "HEAD"])
    sha, subject, date = sha_env, None, None
    if text and text.count("\x1f") >= 2:
        sha, subject, date = text.split("\x1f", 2)
    ref = os.environ.get("GITHUB_REF_NAME") or git_out(["rev-parse", "--abbrev-ref", "HEAD"])
    return {
        "sha": sha,
        "subject": subject,
        "date": date,
        "ref": ref,
        "run_id": env_int("GITHUB_RUN_ID"),
        "run_number": env_int("GITHUB_RUN_NUMBER"),
        "run_attempt": env_int("GITHUB_RUN_ATTEMPT"),
    }


def env_info(raw):
    ctx = (raw or {}).get("context") or {}
    image_os = os.environ.get("ImageOS")
    image_version = os.environ.get("ImageVersion")
    # ホスト名やユーザー名は記録しない (公開ページに出るため)。CPU とイメージだけで足りる
    return {
        "cpu": (ctx.get("cpu") or "").strip() or None,
        "logical_cpus": ctx.get("logical_cpus"),
        "hypervisor": ctx.get("hypervisor"),
        "image": f"{image_os}/{image_version}" if image_os and image_version else None,
        "runner": os.environ.get("RUNNER_ENVIRONMENT") or None,
        "compiler": ctx.get("compiler"),
    }


# ---------------------------------------------------------------------------
# main
# ---------------------------------------------------------------------------

def main():
    for stream in (sys.stdout, sys.stderr):
        if hasattr(stream, "reconfigure"):
            stream.reconfigure(encoding="utf-8", errors="replace")

    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--exe", required=True, help="計測する bench_test.exe (head)")
    ap.add_argument("--compiler", required=True, choices=bc.COMPILERS, help="コンパイラ (構成名に使う)")
    ap.add_argument("--config", required=True, choices=bc.CONFIGS, help="ビルド構成 (構成名に使う)")
    ap.add_argument("--out-dir", default="bench-out", help="結果を書くディレクトリ (既定: bench-out)")
    ap.add_argument("--smoke", action="store_true", help="各ベンチマークを 1 回だけ動かす (Debug 向け。base と比べない)")
    ap.add_argument("--rounds", type=int, help="ラウンド数 (既定: base ありで 10、なしで 5、スモークで 1)")
    ap.add_argument("--startup-exe", help="起動を測る runtime.exe (head)。省くと起動は測らない")
    ap.add_argument("--startup-launches", type=int, default=3,
                    help="1 ラウンドで runtime.exe を起動する回数 (既定: 3。スモークでは 1)")
    ap.add_argument("--base-startup-exe", help="--base-exe と組で使う、比較対象の runtime.exe")
    group = ap.add_mutually_exclusive_group()
    group.add_argument("--base-exe", help="比較対象の exe を直接指定する")
    group.add_argument("--base", choices=("auto", "none"), default="auto",
                       help="auto: master のアーティファクトから探す (既定)、none: 比べない")
    ap.add_argument("--threshold", type=float, default=bc.DEFAULT_THRESHOLD,
                    help="悪化・改善と判定する変化の大きさ (既定: 0.05 = 5 %%。複数スレッドは 2 倍)")
    ap.add_argument("--filter", help="名前にこの文字列を含むベンチマークだけ回す (exe へそのまま渡す)")
    ap.add_argument("--min-benchmarks", type=int, default=1,
                    help="結果がこの件数未満なら失敗にする (0 件の緑を緑と誤読しないため。既定: 1)")
    ap.add_argument("--timeout", type=float, default=EXE_TIMEOUT,
                    help=f"exe 1 回の実行の上限秒数 (既定: {EXE_TIMEOUT})")
    ap.add_argument("--summary", help="Markdown を追記するファイル ($GITHUB_STEP_SUMMARY)")
    args = ap.parse_args()

    if args.rounds is not None and args.rounds < 1:
        ap.error("--rounds は 1 以上")
    if not (0 < args.threshold < 1):
        ap.error("--threshold は 0 と 1 の間")
    if args.startup_launches < 1:
        ap.error("--startup-launches は 1 以上")

    variant = bc.variant_id(args.compiler, args.config)
    head_exe = os.path.abspath(args.exe)
    if not os.path.isfile(head_exe):
        error(f"bench_test が無い: {args.exe} (ビルドに失敗したか、出力先が違う)")
        return 1
    out_dir = os.path.abspath(args.out_dir)
    raw_dir = os.path.join(out_dir, "raw")
    os.makedirs(raw_dir, exist_ok=True)
    for old in glob.glob(os.path.join(raw_dir, "*.json")):
        os.remove(old)

    # base を決める。スモークは Debug で動くかを見るだけなので比べない
    base_exe, base_info = None, None
    if args.smoke:
        pass
    elif args.base_exe:
        if os.path.isfile(args.base_exe):
            base_exe = os.path.abspath(args.base_exe)
            base_info = {"sha": None, "run_id": None, "run_number": None, "kind": "explicit"}
        else:
            warning(f"--base-exe が無いので比較せずに計測する: {args.base_exe}")
    elif args.base == "auto":
        base_exe, base_info = resolve_base([f"bench-exe-{variant}", f"bench-exe-{args.compiler}-{args.config}"], out_dir)

    # 起動の計測 (runtime.exe)。base の runtime.exe は bench_test.exe と同じアーティファクトに入っている
    startup = None
    if args.startup_exe:
        head_startup = os.path.abspath(args.startup_exe)
        if not os.path.isfile(head_startup):
            warning(f"runtime.exe が無いので起動は測らない: {args.startup_exe}")
        else:
            base_startup = None
            if args.base_startup_exe:
                base_startup = os.path.abspath(args.base_startup_exe) if os.path.isfile(args.base_startup_exe) else None
            elif base_exe and base_info and base_info.get("kind") != "explicit":
                base_startup = find_named(os.path.dirname(base_exe), bc.RUNTIME_EXE_NAME)
                if base_startup is None:
                    notice("base のアーティファクトに runtime.exe が無い (起動の計測を始める前のコミット)。"
                           "起動は比べずに head だけ測る")
            if base_exe is None:
                base_startup = None
            startup = StartupRunner({"head": head_startup, "base": base_startup},
                                    1 if args.smoke else args.startup_launches, raw_dir, args.smoke)

    if args.smoke:
        rounds = 1
    elif args.rounds is not None:
        rounds = args.rounds
    else:
        rounds = 10 if base_exe else 5
    extra = ["--filter", args.filter] if args.filter else []

    label = bc.variant_label(variant)
    print(f"{label}: {rounds} ラウンド" + (" (スモーク)" if args.smoke else "")
          + (f" / base: {bc.base_kind_label(base_info['kind'])}" if base_info else " / base なし"), flush=True)

    if startup and not args.smoke:
        startup.warm_up()

    head_raws, base_raws = [], []
    head_error = None
    budget_exit = False
    for r in range(rounds):
        order = ("head", "base") if r % 2 == 0 else ("base", "head")
        for side in order:
            if side == "base" and base_exe is None:
                continue
            exe = head_exe if side == "head" else base_exe
            out_path = os.path.join(raw_dir, f"{side}-{r}.json")
            raw, code, reason, log, elapsed = run_exe(exe, out_path, args.smoke, extra, args.timeout)
            if side == "head":
                if raw is None:
                    head_error = f"ラウンド {r + 1}/{rounds}: {reason}"
                    error(f"head の bench_test が失敗した ({head_error})")
                    print_group("head の出力 (末尾)", log)
                    break
                if code == 3 and not args.smoke:
                    budget_exit = True
                head_raws.append(raw)
                if r == 0:
                    # 1 回目だけ進捗を残す (毎回出すとログが読めなくなる)
                    print_group("head の出力 (1 ラウンド目)", log)
            else:
                # base の予算超過 (終了コード 3) は base 側のコードの話なので、計測としては使う
                if raw is None:
                    warning(f"base の bench_test が失敗したので比較をやめる (ラウンド {r + 1}/{rounds}: {reason})")
                    print_group("base の出力 (末尾)", log)
                    base_exe, base_info, base_raws = None, None, []
                    continue
                base_raws.append(raw)
            n = len(raw.get("benchmarks") or [])
            print(f"[{r + 1}/{rounds}] {side}: {n} 件, {elapsed:.1f} 秒"
                  + (" (予算超過あり)" if code == 3 and side == "head" else ""), flush=True)
            if startup:
                # 同じラウンドの raw に足す。以降の集計は bench_test のベンチマークと区別しない
                raw.setdefault("benchmarks", []).extend(startup.measure(side, r))
        if head_error:
            break

    benchmarks = build_benchmarks(head_raws, base_raws, args.threshold, args.smoke) if head_raws else []
    result = {
        "schema": bc.RUN_SCHEMA,
        "variant": variant,
        "compiler": args.compiler,
        "config": args.config,
        "smoke": bool(args.smoke),
        "repo": os.environ.get("GITHUB_REPOSITORY") or None,
        "commit": commit_info(),
        "base": base_info if base_raws else None,
        "env": env_info(head_raws[0] if head_raws else None),
        "rounds": len(head_raws),
        "threshold": args.threshold,
        "benchmarks": benchmarks,
        "summary": summarize(benchmarks),
    }

    exit_code = 0
    try:
        bc.write_json(os.path.join(out_dir, "result.json"), result)
    except OSError as e:
        error(f"result.json を書けない: {e}")
        exit_code = 1

    markdown = bc.variant_markdown(result)
    if startup and startup.problems:
        markdown += "".join(f"{p}\n\n" for p in startup.problems)
    if head_error:
        markdown += f"❌ head の bench_test が失敗した ({head_error})。集まった分だけ載せている。\n\n"
    print(markdown)
    if args.summary:
        try:
            with open(args.summary, "a", encoding="utf-8") as f:
                f.write(markdown)
        except OSError as e:
            warning(f"サマリーを書けない: {e}")

    if head_error:
        return 1
    if len(benchmarks) < args.min_benchmarks:
        error(f"ベンチマークの結果が {len(benchmarks)} 件しかない (最低 {args.min_benchmarks} 件)。"
              "フィルタの誤りか、exe が何も登録していない")
        return 1
    if exit_code:
        return exit_code

    violations = [b for b in benchmarks if (b.get("alloc") or {}).get("budget_ok") is False]
    for b in violations:
        a = b["alloc"]
        error(f"ヒープ確保の予算超過: {b['name']} ({bc.fmt_count(a.get('allocs'))} 回/op、"
              f"予算 {bc.fmt_count(a.get('budget'))}) [{label}]")
    if budget_exit and not violations:
        # exe は超過と言っているのに、結果の JSON に budget_ok=false が無い
        error(f"bench_test がヒープ確保の予算超過 (終了コード 3) を返した [{label}]")
    if violations or budget_exit:
        return 3
    return 0


if __name__ == "__main__":
    sys.exit(main())
