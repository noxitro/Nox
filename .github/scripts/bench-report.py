#!/usr/bin/env python3
"""ベンチマークの結果を履歴にまとめ、結果ページ (静的サイト) とサマリーを作る。

.github/workflows/bench-publish.yml (ubuntu) から呼ばれる。やること:
  1. 今回の CI の結果 (--results-dir の下の */result.json。bench-run.py が書いたもの) を読む
  2. これまでの履歴を集める。どれか 1 つが欠けても残りで補えるよう、複数の出どころを
     合わせる。同じ構成・同じコミットは 1 点にまとめる (実行番号・再実行回数が大きい方が
     勝つ) ので、同じものを何度合わせても結果は変わらない。
       - 公開中のページの data/history.js (--pages-url)
       - 前回の master の実行が残した履歴 (--history-file。アーティファクトから戻したもの)
       - 直近の master の CI が上げた bench-result-* (--backfill。gh で取る)
  3. publish (master) なら今回の結果を履歴に足す。preview (ほかのブランチ) なら履歴には
     足さず、latest に「候補」として入れ、master の履歴と並べて見られるようにする
  4. --site-src のページ一式と data/history.js・history.json・latest.js を --out-site に書く
  5. ジョブのサマリー・ステップの出力 (悪化の件数など)・Discord 用の本文を書く

公開中の履歴を取れないとき (404 以外の HTTP エラー・タイムアウト・壊れた内容) は、publish
ではサイトを書かずに終了コード 1 で止める。欠けた履歴で上書き公開すると過去の点が消えるため。
preview はどこにも書き戻さないので止めない。::warning:: を出し、取れた分の履歴だけでプレビュー
を作る (サマリーにも、公開中の履歴が入っていないと書く)。404 は「まだ一度も公開していない」と
みなし、空の履歴から始める。

publish で今回のコミットが履歴の最新のコミットより古い (古い master の実行を再実行した) ときは、
latest を古いコミットへ戻さず、前に公開した latest をそのまま使う。

使い方:
    python3 .github/scripts/bench-report.py --results-dir bench-results \\
        --site-src tools/bench-site --out-site _site \\
        --pages-url https://noxitro.github.io/Nox/ --history-file prev/history.js \\
        --backfill 30 --mode publish \\
        --summary $GITHUB_STEP_SUMMARY --output $GITHUB_OUTPUT --discord-md bench-discord.md

読む環境変数: GITHUB_REPOSITORY, GITHUB_SERVER_URL, GITHUB_RUN_ID, GH_TOKEN (--backfill の gh)

ステップの出力 (--output): regressed, improved, budget_violations, alloc_regressed,
has_results (今回の計測結果があるか), discord (Discord 用の本文を書いたか)

終了コード: 0 = 成功、1 = 履歴を取れない・読めない (publish のみ)・サイトを書けない、
2 = 引数の誤り。
"""

import argparse
import datetime as dt
import glob
import http.client
import json
import os
import shutil
import subprocess
import sys
import tempfile
import time
import urllib.error
import urllib.request

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bench_common as bc  # noqa: E402

FETCH_TIMEOUT = 30
GH_TIMEOUT = 300
DISCORD_LIMIT = 3500
# Discord では 1 構成あたりこの件数まで載せる (全構成の見出しが必ず見えるように)
DISCORD_ITEMS_PER_VARIANT = 8
# GitHub が 1 ステップで表示する注釈は 10 件程度なので、それ以上はまとめて 1 件にする
MAX_ANNOTATIONS = 10
# サイトへ写さないもの (開発用の道具と説明、手元で作ったデモデータ)
SKIP_FILES = {"make-demo-data.py", "README.md"}
SKIP_DIRS = {"data", "__pycache__", "node_modules"}
BENCH_META = ("title", "group", "per", "threads", "alloc_budget")
BENCH_COLUMNS = ("c", "e", "m", "q1", "q3", "r", "rl", "rh", "v", "bs", "a", "ab")
ENV_KEYS = ("cpu", "logical_cpus", "image", "runner", "compiler")
COMMIT_KEYS = ("sha", "subject", "date", "run_id", "run_number")


class ReportError(Exception):
    """サイトを書かずに止めるべき失敗 (履歴を取れない・読めない)。"""


def warning(message):
    bc.annotate("warning", message)


def notice(message):
    bc.annotate("notice", message)


def run_rank(commit):
    """同じ (構成, コミット) が複数あるときの優先順位。新しい実行・新しい再実行が勝つ。"""
    commit = commit or {}
    num = commit.get("run_number") if isinstance(commit.get("run_number"), int) else 0
    attempt = commit.get("run_attempt") if isinstance(commit.get("run_attempt"), int) else 0
    return (num, attempt)


# ---------------------------------------------------------------------------
# 今回の結果
# ---------------------------------------------------------------------------

def valid_run(r):
    return (isinstance(r, dict) and r.get("schema") == bc.RUN_SCHEMA
            and isinstance(r.get("variant"), str)
            and isinstance(r.get("benchmarks"), list)
            and isinstance(r.get("commit"), dict) and isinstance(r["commit"].get("sha"), str))


def is_history_result(r):
    return not r.get("smoke") and r.get("variant") in bc.HISTORY_VARIANTS


def read_results(paths, label):
    """result.json を読む。壊れたものは警告して飛ばす (1 構成の失敗で全体を止めない)。"""
    out = []
    for path in paths:
        try:
            r = bc.read_json(path)
        except (OSError, ValueError) as e:
            warning(f"{label}の result.json を読めない: {os.path.basename(os.path.dirname(path))} ({e})")
            continue
        if not valid_run(r):
            warning(f"{label}の result.json が {bc.RUN_SCHEMA} の形でない: {os.path.basename(os.path.dirname(path))}")
            continue
        out.append(r)
    return out


def load_current(results_dir):
    """(履歴に入れる構成の結果, それ以外 (スモークなど)) を返す。"""
    if not results_dir or not os.path.isdir(results_dir):
        print("今回の結果は無い (サイトだけ作り直す)")
        return [], []
    paths = sorted(glob.glob(os.path.join(results_dir, "**", "result.json"), recursive=True))
    main_results = {}
    others = []
    for r in read_results(paths, "今回"):
        if not is_history_result(r):
            others.append(r)
            continue
        prev = main_results.get(r["variant"])
        if prev is None or run_rank(r["commit"]) >= run_rank(prev["commit"]):
            main_results[r["variant"]] = r
    current = [main_results[v] for v in bc.HISTORY_VARIANTS if v in main_results]
    others.sort(key=lambda r: r.get("variant") or "")
    print(f"今回の結果: {len(current)} 構成" + (f" (ほかにスモークなど {len(others)} 件)" if others else ""))
    return current, others


# ---------------------------------------------------------------------------
# 点 (1 構成 × 1 コミット) の集まり
# ---------------------------------------------------------------------------

class Store:
    """(構成, コミット) → 点。同じキーは rank の大きい方、同じ rank なら後から入れた方が勝つ。"""

    def __init__(self):
        self.points = {}

    def has(self, variant, sha):
        return (variant, sha) in self.points

    def put(self, point):
        key = (point["variant"], point["sha"])
        old = self.points.get(key)
        if old is not None and point["rank"] < old["rank"]:
            return False
        self.points[key] = point
        return old is None


def point_from_result(r):
    c = r["commit"]
    env = r.get("env") or {}
    base_sha = (r.get("base") or {}).get("sha")
    benches = {}
    for b in r.get("benchmarks") or []:
        if not isinstance(b, dict) or not isinstance(b.get("name"), str):
            continue
        name = b["name"]
        head = b.get("head") or {}
        m = bc.sig_round(head.get("median"))
        if m is None:
            continue
        paired = b.get("paired") or {}
        alloc = b.get("alloc") or {}
        benches[name] = {
            "title": b.get("title"),
            "group": b.get("group"),
            "per": b.get("per"),
            "threads": b.get("threads"),
            "alloc_budget": b.get("alloc_budget"),
            "m": m,
            "q1": bc.sig_round(head.get("q1")),
            "q3": bc.sig_round(head.get("q3")),
            "r": bc.dec_round(paired.get("ratio")),
            "rl": bc.dec_round(paired.get("ci_low")),
            "rh": bc.dec_round(paired.get("ci_high")),
            "v": bc.VERDICT_CODES.get(paired.get("verdict")),
            "bs": base_sha if b.get("base") else None,
            "a": bc.sig_round(alloc.get("allocs")),
            "ab": bc.sig_round(alloc.get("bytes")),
        }
    return {
        "variant": r["variant"],
        "sha": c["sha"],
        "rank": run_rank(c),
        "commit": {k: c.get(k) for k in COMMIT_KEYS},
        "env": {k: env.get(k) for k in ENV_KEYS},
        "benchmarks": benches,
    }


def _col(col, key, i):
    values = col.get(key)
    if not isinstance(values, list) or i >= len(values):
        return None
    return values[i]


def points_from_history(hist, source):
    """列形式の履歴を点に戻す。形が壊れていれば ReportError。"""
    if not isinstance(hist, dict) or hist.get("schema") != bc.HISTORY_SCHEMA:
        schema = hist.get("schema") if isinstance(hist, dict) else None
        raise ReportError(f"{source}: スキーマが {bc.HISTORY_SCHEMA} でない ({schema!r})")
    try:
        commits = hist["commits"]
        points = []
        for variant, vdata in (hist.get("variants") or {}).items():
            if variant not in bc.HISTORY_VARIANTS:
                continue
            envs = vdata.get("envs") or []
            by_commit = {}
            for name, col in (vdata.get("benchmarks") or {}).items():
                meta = {k: col.get(k) for k in BENCH_META}
                for i, ci in enumerate(col["c"]):
                    if not isinstance(ci, int) or not 0 <= ci < len(commits):
                        raise ReportError(f"{source}: {variant} / {name} のコミット番号が範囲外 ({ci!r})")
                    point = by_commit.get(ci)
                    if point is None:
                        info = commits[ci]
                        ei = _col(col, "e", i)
                        env = envs[ei] if isinstance(ei, int) and 0 <= ei < len(envs) else {}
                        point = {
                            "variant": variant,
                            "sha": info["sha"],
                            # 履歴には再実行回数を残していないので 0 とみなす (同じ実行の結果が来たらそちらが勝つ)
                            "rank": run_rank({"run_number": info.get("run_number")}),
                            "commit": {k: info.get(k) for k in COMMIT_KEYS},
                            "env": {k: env.get(k) for k in ENV_KEYS},
                            "benchmarks": {},
                        }
                        by_commit[ci] = point
                    bs = _col(col, "bs", i)
                    data = dict(meta)
                    for key in ("m", "q1", "q3", "r", "rl", "rh", "v", "a", "ab"):
                        data[key] = _col(col, key, i)
                    data["bs"] = commits[bs]["sha"] if isinstance(bs, int) and 0 <= bs < len(commits) else None
                    point["benchmarks"][name] = data
            points.extend(by_commit.values())
        return points
    except (KeyError, IndexError, TypeError, AttributeError) as e:
        raise ReportError(f"{source}: 履歴の形が壊れている ({type(e).__name__}: {e})")


def build_history(store, repo, max_commits, generated):
    """点の集まりから列形式の履歴 (nox-bench-history/1) を作る。"""
    # コミットの並び: 実行番号の昇順 (同じなら日時)。同じコミットが複数の実行で測られて
    # いたら一番古い実行の位置に置く (再計測でグラフの位置が動かないように)
    commits = {}
    for p in store.points.values():
        key = (p["commit"].get("run_number") or 0, p["commit"].get("date") or "", p["sha"])
        if p["sha"] not in commits or key < commits[p["sha"]][0]:
            commits[p["sha"]] = (key, p["commit"])
    ordered = sorted(commits.values(), key=lambda x: x[0])
    dropped = max(0, len(ordered) - max_commits)
    if dropped:
        print(f"古いコミット {dropped} 件を履歴から外した (上限 {max_commits})")
    ordered = ordered[dropped:]
    index = {info["sha"]: i for i, (_, info) in enumerate(ordered)}

    variants = {}
    for variant in bc.HISTORY_VARIANTS:
        pts = sorted((p for (v, sha), p in store.points.items() if v == variant and sha in index),
                     key=lambda p: index[p["sha"]])
        if not pts:
            continue
        # ベンチマークの並び: 新しい点の並び (C++ 側の登録順) を優先し、消えたものは後ろへ
        names = []
        seen = set()
        for p in reversed(pts):
            for name in p["benchmarks"]:
                if name not in seen:
                    seen.add(name)
                    names.append(name)
        cols = {name: dict({k: None for k in BENCH_META}, **{k: [] for k in BENCH_COLUMNS}) for name in names}
        envs = []
        env_index = {}
        for p in pts:
            ekey = json.dumps(p["env"], sort_keys=True, ensure_ascii=False)
            if ekey not in env_index:
                env_index[ekey] = len(envs)
                envs.append(p["env"])
            ci = index[p["sha"]]
            for name, d in p["benchmarks"].items():
                col = cols[name]
                col["c"].append(ci)
                col["e"].append(env_index[ekey])
                for key in ("m", "q1", "q3", "a", "ab"):
                    col[key].append(bc.sig_round(d.get(key)))
                for key in ("r", "rl", "rh"):
                    col[key].append(bc.dec_round(d.get(key)))
                col["v"].append(d.get("v") if d.get("v") in (1, -1, 0, 2) else None)
                col["bs"].append(index.get(d.get("bs"), -1) if d.get("bs") else -1)
                # 表示用の情報は新しい点のものを使う (点は古い順なので上書きしていけばよい)。
                # 予算は「外した」も新しい情報なので null でも上書きする
                for key in ("title", "group", "per", "threads"):
                    if d.get(key) is not None:
                        col[key] = d[key]
                col["alloc_budget"] = d.get("alloc_budget")
        variants[variant] = {"label": bc.variant_label(variant), "envs": envs, "benchmarks": cols}

    return {
        "schema": bc.HISTORY_SCHEMA,
        "repo": repo,
        "generated": generated,
        "commits": [dict(info) for _, info in ordered],
        "variants": variants,
    }, dropped


# ---------------------------------------------------------------------------
# 履歴の取得
# ---------------------------------------------------------------------------

def fetch_text(url):
    """HTTP で取る。404 なら None。ほかの失敗は ReportError。"""
    if url.startswith(("http://", "https://")):
        # Pages の CDN は数分キャッシュする。直前の公開を取りこぼさないよう毎回違う URL にする
        url += ("&" if "?" in url else "?") + f"t={int(time.time())}"
    req = urllib.request.Request(url, headers={"User-Agent": "nox-ci bench-report", "Cache-Control": "no-cache"})
    try:
        with urllib.request.urlopen(req, timeout=FETCH_TIMEOUT) as resp:
            data = resp.read()
    except urllib.error.HTTPError as e:
        if e.code == 404:
            return None
        raise ReportError(f"{url}: HTTP {e.code} {e.reason}")
    except (urllib.error.URLError, OSError, http.client.HTTPException, ValueError) as e:
        raise ReportError(f"{url}: {e}")
    try:
        return data.decode("utf-8")
    except UnicodeDecodeError as e:
        raise ReportError(f"{url}: UTF-8 として読めない ({e})")


def parse_data(text, prefix, source):
    """data/*.js (window.X = ...;) か素の JSON を読む。"""
    try:
        if text.lstrip("﻿").lstrip().startswith("window."):
            return bc.parse_js(text, prefix)
        return json.loads(text.lstrip("﻿"))
    except ValueError as e:
        raise ReportError(f"{source}: 読めない ({e})")


def pages_data_url(pages_url, name):
    base = pages_url if pages_url.endswith("/") else pages_url + "/"
    return base + "data/" + name


def fetch_pages_history(pages_url):
    """公開中のページの履歴。まだ公開していない (404) なら None。"""
    url = pages_data_url(pages_url, "history.js")
    text = fetch_text(url)
    if text is None:
        notice(f"公開中の履歴が無い (404)。空の履歴から始める: {url}")
        return None
    return parse_data(text, bc.HISTORY_JS_PREFIX, "公開中の history.js")


def fetch_pages_latest(pages_url):
    """公開中のページの latest。無ければ None。"""
    text = fetch_text(pages_data_url(pages_url, "latest.js"))
    if text is None:
        return None
    latest = parse_data(text, bc.LATEST_JS_PREFIX, "公開中の latest.js")
    if latest is not None and (not isinstance(latest, dict) or latest.get("schema") != bc.LATEST_SCHEMA):
        raise ReportError(f"公開中の latest.js のスキーマが {bc.LATEST_SCHEMA} でない")
    return latest


def read_history_file(path):
    """(履歴, 隣の latest.js) を読む。ファイルが無ければ (None, None)。"""
    if not os.path.isfile(path):
        notice(f"--history-file が無いので飛ばす: {path}")
        return None, None
    try:
        with open(path, encoding="utf-8") as f:
            text = f.read()
    except (OSError, UnicodeDecodeError) as e:
        raise ReportError(f"--history-file を読めない: {path} ({e})")
    history = parse_data(text, bc.HISTORY_JS_PREFIX, f"--history-file {path}")
    latest = None
    sibling = os.path.join(os.path.dirname(path), "latest.js")
    if os.path.isfile(sibling):
        try:
            with open(sibling, encoding="utf-8") as f:
                latest = parse_data(f.read(), bc.LATEST_JS_PREFIX, sibling)
        except (OSError, UnicodeDecodeError, ReportError) as e:
            # latest は「今回の結果が無いときの表示」にしか使わないので、読めなくても止めない
            warning(f"{sibling} を読めないので使わない ({e})")
            latest = None
    return history, latest


# ---------------------------------------------------------------------------
# backfill (直近の master の CI の結果を gh で集める)
# ---------------------------------------------------------------------------

def gh(args, what):
    """gh を 1 回呼び、標準出力を返す。失敗したら ::warning:: を出して None。

    backfill は「取れれば足す」処理なので、失敗してもサイトづくりは止めない。
    """
    cmd = ["gh"] + [str(a) for a in args]
    try:
        proc = subprocess.run(cmd, stdin=subprocess.DEVNULL, capture_output=True, timeout=GH_TIMEOUT)
    except FileNotFoundError:
        warning(f"backfill できない ({what}): gh が見つからない")
        return None
    except subprocess.TimeoutExpired:
        warning(f"backfill できない ({what}): gh が {GH_TIMEOUT} 秒で終わらない")
        return None
    except OSError as e:
        warning(f"backfill できない ({what}): gh を起動できない ({e})")
        return None
    if proc.returncode != 0:
        err = (proc.stderr or b"").decode("utf-8", errors="replace").strip().splitlines()
        warning(f"backfill できない ({what}): gh の終了コード {proc.returncode}: {err[-1] if err else ''}")
        return None
    return (proc.stdout or b"").decode("utf-8", errors="replace")


def gh_json(args, what):
    text = gh(args, what)
    if text is None:
        return None
    try:
        return json.loads(text)
    except ValueError as e:
        warning(f"backfill できない ({what}): gh の出力が JSON でない ({e})")
        return None


def list_master_runs(repo, workflow, count):
    runs = []
    page = 1
    per_page = min(count, 100)
    while len(runs) < count:
        path = f"repos/{repo}/actions/workflows/{workflow}/runs?branch=master&status=completed&per_page={per_page}"
        if count > 100:
            path += f"&page={page}"
        data = gh_json(["api", path], "master の実行の一覧")
        if not isinstance(data, dict):
            break
        batch = data.get("workflow_runs") or []
        runs.extend(batch)
        if len(batch) < per_page or count <= 100:
            break
        page += 1
    return runs[:count]


def backfill(store, repo, count, workflow, current_run_id):
    """履歴に無い (構成, コミット) を、直近の master の CI のアーティファクトから埋める。

    Pages への公開が失敗した・Pages を有効にする前だった、などで抜けた点を取り戻す。
    """
    if not repo:
        warning("GITHUB_REPOSITORY (--repo) が無いので backfill しない")
        return 0
    runs = list_master_runs(repo, workflow, count)
    added = 0
    tmp = tempfile.mkdtemp(prefix="bench-backfill-")
    try:
        for run in runs:
            run_id = run.get("id")
            sha = run.get("head_sha")
            if not run_id or not sha or str(run_id) == str(current_run_id) or run.get("head_branch") != "master":
                continue
            missing = {f"bench-result-{v}" for v in bc.HISTORY_VARIANTS if not store.has(v, sha)}
            if not missing:
                continue
            arts = gh_json(["api", f"repos/{repo}/actions/runs/{run_id}/artifacts?per_page=100"],
                           f"run {run_id} のアーティファクト一覧")
            if not isinstance(arts, dict):
                continue
            # 名前は大文字小文字を区別しない (ワークフローが bench-result-MSVC-Master で上げても拾う)
            names = sorted({a["name"] for a in arts.get("artifacts") or []
                            if isinstance(a.get("name"), str) and a["name"].lower() in missing and not a.get("expired")})
            if not names:
                continue
            dest = os.path.join(tmp, str(run_id))
            args = ["run", "download", str(run_id), "-R", repo, "-D", dest]
            for name in names:
                args += ["-n", name]
            if gh(args, f"run {run_id} の結果の取得") is None:
                continue
            paths = sorted(glob.glob(os.path.join(dest, "**", "result.json"), recursive=True))
            for r in read_results(paths, f"run {run_id} "):
                if is_history_result(r) and store.put(point_from_result(r)):
                    added += 1
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    print(f"backfill: master の直近 {len(runs)} 実行を調べ、{added} 点を足した")
    return added


# ---------------------------------------------------------------------------
# サイト
# ---------------------------------------------------------------------------

def copy_site(site_src, out_site):
    """ページ一式を写す。data/ (手元のデモデータ) や開発用の道具は写さない。"""
    if not site_src or not os.path.isdir(site_src):
        warning(f"ページのソースが無いので data/ だけ書く: {site_src}")
        return 0
    copied = 0
    for root, dirs, files in os.walk(site_src):
        dirs[:] = sorted(d for d in dirs if d not in SKIP_DIRS and not d.startswith("."))
        rel_root = os.path.relpath(root, site_src)
        for name in sorted(files):
            if name in SKIP_FILES or name.startswith(".") or name.endswith(".pyc"):
                continue
            dst = os.path.normpath(os.path.join(out_site, rel_root, name))
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            shutil.copyfile(os.path.join(root, name), dst)
            copied += 1
    if not os.path.isfile(os.path.join(site_src, "index.html")):
        warning(f"{site_src} に index.html が無い")
    return copied


def write_site(site_src, out_site, history, latest):
    copied = copy_site(site_src, out_site)
    data_dir = os.path.join(out_site, "data")
    shutil.rmtree(data_dir, ignore_errors=True)
    bc.write_js(os.path.join(data_dir, "history.js"), bc.HISTORY_JS_PREFIX, history)
    bc.write_json(os.path.join(data_dir, "history.json"), history, indent=None)
    bc.write_js(os.path.join(data_dir, "latest.js"), bc.LATEST_JS_PREFIX, latest)
    size = os.path.getsize(os.path.join(data_dir, "history.js"))
    print(f"サイト: {out_site} (ページ {copied} ファイル + data/、history.js {size / 1024:.1f} KiB)")


def behind_history(commit, history):
    """commit が履歴の最新のコミットより前か。

    履歴のコミットは実行番号の順に並ぶ (build_history)。再実行では実行番号が変わらないので、
    古い master の実行を再実行すると、そのコミットはその後の master より前に来る。
    履歴に無いコミット (上限で外れた・点が無い) は実行番号で比べる。
    """
    commits = history.get("commits") or []
    sha = commit.get("sha")
    if not commits or commits[-1].get("sha") == sha:
        return False
    if any(c.get("sha") == sha for c in commits):
        return True
    number, newest = commit.get("run_number"), commits[-1].get("run_number")
    return isinstance(number, int) and isinstance(newest, int) and number < newest


def older_run(a, b):
    """コミット情報 a の実行番号が b より小さいか (どちらかが無ければ False)。"""
    na, nb = (a or {}).get("run_number"), (b or {}).get("run_number")
    return isinstance(na, int) and isinstance(nb, int) and na < nb


def build_latest(current, mode, server, repo, previous, history):
    if not current:
        # 計測の無い実行 (サイトだけ作り直す) では、前に公開した latest をそのまま使う
        return previous
    head = max(current, key=lambda r: run_rank(r["commit"]))
    c = head["commit"]
    if mode == "publish" and behind_history(c, history):
        # 古い master の実行の再実行など。latest を古いコミットへ戻さない (履歴の点は更新する)
        newest = history["commits"][-1]
        where = (f"今回のコミット {bc.short_sha(c.get('sha'))} (#{c.get('run_number')}) は履歴の最新 "
                 f"{bc.short_sha(newest.get('sha'))} (#{newest.get('run_number')}) より古い")
        prev_commit = (previous or {}).get("commit") if isinstance(previous, dict) else None
        if isinstance(previous, dict) and not previous.get("candidate") and not older_run(prev_commit, c):
            notice(f"{where}ので、前に公開した latest ({bc.short_sha((prev_commit or {}).get('sha'))}) を"
                   "そのまま使う")
            return previous
        print(f"{where}が、前に公開した latest が無い (かさらに古い) ので今回の結果を latest にする")
    run_url = f"{server}/{repo}/actions/runs/{c['run_id']}" if repo and c.get("run_id") else None
    return {
        "schema": bc.LATEST_SCHEMA,
        "candidate": mode == "preview",
        "branch": c.get("ref"),
        "commit": {k: c.get(k) for k in COMMIT_KEYS},
        "run_url": run_url,
        "variants": {r["variant"]: r for r in current},
    }


# ---------------------------------------------------------------------------
# サマリー・Discord
# ---------------------------------------------------------------------------

def budget_violations(r):
    return [b for b in r.get("benchmarks") or [] if (b.get("alloc") or {}).get("budget_ok") is False]


def regressed(r):
    return [b for b in r.get("benchmarks") or [] if (b.get("paired") or {}).get("verdict") == "regressed"]


def alloc_regressed(r):
    return [b for b in r.get("benchmarks") or [] if (b.get("alloc") or {}).get("verdict") == "regressed"]


def variant_line(r):
    s = r.get("summary") or {}
    label = bc.variant_label(r["variant"])
    cpu = bc.short_cpu((r.get("env") or {}).get("cpu"))
    if r.get("base"):
        text = (f"- **{label}**: {bc.count_text(s)} ・ 全体 {bc.fmt_pct(s.get('geomean_change'))} ・ "
                f"{cpu} ・ 比較: {bc.base_text(r)}")
    else:
        text = f"- **{label}**: 比較対象なし ({r.get('rounds', 0)} ラウンド) ・ {cpu}"
    if s.get("budget_violations"):
        text += f" ・ ❌ 予算超過 {s['budget_violations']}"
    if s.get("alloc_regressed"):
        text += f" ・ 確保増 {s['alloc_regressed']}"
    return text + "\n"


def summary_markdown(current, others, history, mode, pages_url, stats, missing=()):
    out = []
    if current:
        c = max(current, key=lambda r: run_rank(r["commit"]))["commit"]
        subject = (c.get("subject") or "").strip()
        out.append(f"## 📊 ベンチマーク (`{bc.short_sha(c.get('sha'))}` {bc.md_cell(subject)})\n\n")
    else:
        out.append("## 📊 ベンチマーク\n\n")
    if mode == "preview" and current:
        out.append(f"> プレビュー: ブランチ `{current[0]['commit'].get('ref') or '?'}` の結果。"
                   "master の履歴には入れず、並べて表示するだけ。\n\n")
    if missing:
        # preview で履歴の一部を取れなかった (publish ではここまで来ない)
        reasons = "、".join(f"{what}: {bc.md_cell(err)}" for what, err, _ in missing)
        if any(published for _, _, published in missing):
            out.append("> ⚠️ 公開中の履歴を取れなかったので、このプレビューには公開済みの履歴が入っていない"
                       f" (ほかから取れた分だけで作った)。{reasons}\n\n")
        else:
            out.append(f"> ⚠️ 次を取れなかったので使わずにプレビューを作った。{reasons}\n\n")
    if not current:
        out.append("この実行には計測結果が無い。履歴からサイトだけ作り直した。\n\n")
    for r in current:
        out.append(variant_line(r))
    if others:
        smoke = [f"{bc.variant_label(r['variant'])} {'✓' if r.get('benchmarks') else '✕'} "
                 f"{len(r.get('benchmarks') or [])} 件" for r in others]
        out.append(f"- スモーク: {'、'.join(smoke)}\n")
    if current or others:
        out.append("\n")

    # 同じベンチマークの行 (構成違い) は並べる。ベンチマーク同士は一番目立つ構成の順
    notable = [(vi, r, b) for vi, r in enumerate(current) for b in r.get("benchmarks") or [] if bc.is_notable(b)]
    group_key = {}
    for _, _, b in notable:
        key = bc.notable_sort_key(b)
        if b["name"] not in group_key or key < group_key[b["name"]]:
            group_key[b["name"]] = key
    notable.sort(key=lambda x: (group_key[x[2]["name"]], x[2]["name"], x[0]))
    if notable:
        out.append(f"### 目立った変化 ({len(notable)} 件)\n\n")
        out.append(bc.md_table(("構成",) + bc.TABLE_COLUMNS,
                               [[bc.variant_label(r["variant"])] + bc.bench_cells(b) for _, r, b in notable]))
        out.append("\n")
    elif any(r.get("base") for r in current):
        out.append("目立った変化は無い (悪化・改善・ばらつき大・確保の増減のどれも無い)。\n\n")

    if mode == "publish" and pages_url:
        out.append(f"- 結果ページ: {pages_url}\n")
    out.append("- サイトのプレビューはアーティファクト `bench-site` から (展開して index.html を開く)\n")
    out.append(f"- 履歴: {len(history['commits'])} コミット / {len(history['variants'])} 構成"
               f" (今回足した点 {stats['added_current']}、backfill {stats['added_backfill']}"
               + (f"、古い {stats['dropped']} コミットを除外" if stats["dropped"] else "") + ")\n\n")
    return "".join(out)


def discord_markdown(current, pages_url):
    """master で悪化・予算超過があったときの Discord 本文。"""
    c = max(current, key=lambda r: run_rank(r["commit"]))["commit"]
    head = f"**`{bc.short_sha(c.get('sha'))}`** {(c.get('subject') or '').strip()}\n"
    footer = f"\n結果ページ: {pages_url}\n" if pages_url else ""
    lines = []
    for r in current:
        items = []
        for b in budget_violations(r):
            a = b["alloc"]
            items.append(f"✕ `{b['name']}` 確保 {bc.fmt_count(a.get('allocs'))} 回/op (予算 {bc.fmt_count(a.get('budget'))})")
        for b in sorted(regressed(r), key=lambda b: -(b["paired"].get("change") or 0)):
            items.append(f"▲ `{b['name']}` {bc.fmt_change(b['paired'])} {bc.fmt_time((b.get('head') or {}).get('median'))}")
        for b in alloc_regressed(r):
            if (b.get("alloc") or {}).get("budget_ok") is False:
                continue
            a = b["alloc"]
            items.append(f"▲ `{b['name']}` 確保 {bc.fmt_count(a.get('base_allocs'))} → {bc.fmt_count(a.get('allocs'))} 回/op")
        if not items:
            continue
        if len(items) > DISCORD_ITEMS_PER_VARIANT:
            rest = len(items) - DISCORD_ITEMS_PER_VARIANT + 1
            items = items[:DISCORD_ITEMS_PER_VARIANT - 1] + [f"…ほか {rest} 件"]
        env = bc.short_cpu((r.get("env") or {}).get("cpu"))
        lines.append(f"\n**{bc.variant_label(r['variant'])}** — {env} ・ 比較: {bc.base_text(r)}")
        lines.extend(items)
    body = head
    budget = DISCORD_LIMIT - len(head) - len(footer) - 40
    for i, line in enumerate(lines):
        if len(line) + 1 > budget:
            rest = sum(1 for x in lines[i:] if not x.startswith("\n"))
            body += f"…ほか {rest} 件\n"
            break
        body += line + "\n"
        budget -= len(line) + 1
    return body + footer


def write_outputs(path, values):
    with open(path, "a", encoding="utf-8") as f:
        for k, v in values.items():
            f.write(f"{k}={v}\n")


# ---------------------------------------------------------------------------
# main
# ---------------------------------------------------------------------------

def main():
    for stream in (sys.stdout, sys.stderr):
        if hasattr(stream, "reconfigure"):
            stream.reconfigure(encoding="utf-8", errors="replace")

    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--results-dir", help="今回の結果 (*/result.json) があるディレクトリ。無くてもよい")
    ap.add_argument("--site-src", default="tools/bench-site", help="ページのソース (既定: tools/bench-site)")
    ap.add_argument("--out-site", default="_site", help="サイトの出力先 (既定: _site)")
    ap.add_argument("--pages-url", help="公開中のページの URL。<URL>data/history.js を取って合わせる")
    ap.add_argument("--history-file", action="append", default=[],
                    help="合わせる履歴 (history.js か history.json)。無ければ飛ばす。複数指定できる")
    ap.add_argument("--backfill", type=int, default=0,
                    help="直近 N 回の master の CI から bench-result-* を集めて埋める (gh と GH_TOKEN が要る)")
    ap.add_argument("--backfill-workflow", default="ci.yml", help="backfill で調べるワークフロー (既定: ci.yml)")
    ap.add_argument("--mode", choices=("publish", "preview"), default="preview",
                    help="publish: 今回の結果を履歴に足す (master)。preview: 足さずに候補として見せる (既定)")
    ap.add_argument("--repo", default=os.environ.get("GITHUB_REPOSITORY"), help="owner/name (既定: $GITHUB_REPOSITORY)")
    ap.add_argument("--max-commits", type=int, default=1500, help="履歴に残すコミット数の上限 (既定: 1500)")
    ap.add_argument("--summary", help="Markdown を追記するファイル ($GITHUB_STEP_SUMMARY)")
    ap.add_argument("--output", help="ステップの出力を追記するファイル ($GITHUB_OUTPUT)")
    ap.add_argument("--discord-md", help="master で悪化・予算超過があったときに Discord の本文を書くファイル")
    args = ap.parse_args()
    if args.max_commits < 1:
        ap.error("--max-commits は 1 以上")

    server = os.environ.get("GITHUB_SERVER_URL") or "https://github.com"
    current, others = load_current(args.results_dir)
    repo = args.repo or next((r.get("repo") for r in current + others if r.get("repo")), None)

    store = Store()
    previous_latest = None
    # preview で取れなかった出どころ ([(何, 理由, 公開中の履歴か)])。publish では 1 つでも
    # 取れなければ止める
    missing = []

    def lenient(what, e, published_history=False):
        """preview では履歴の取得失敗で止めない (どこにも書き戻さないので欠けても害が無い)。"""
        if args.mode == "publish":
            raise e
        warning(f"{what}を取れないので、使わずにプレビューを作る (プレビューは公開しないので止めない): {e}")
        missing.append((what.strip(), str(e), published_history))

    try:
        if args.pages_url:
            history = None
            try:
                history = fetch_pages_history(args.pages_url)
                if history is not None:
                    points = points_from_history(history, "公開中の history.js")
                    for p in points:
                        store.put(p)
                    repo = repo or history.get("repo")
                    print(f"公開中の履歴: {len(points)} 点")
            except ReportError as e:
                lenient("公開中の履歴 (history.js) ", e, published_history=True)
            try:
                previous_latest = fetch_pages_latest(args.pages_url)
            except ReportError as e:
                lenient("公開中の latest.js ", e)
        for path in args.history_file:
            try:
                history, sibling_latest = read_history_file(path)
                if history is None:
                    continue
                points = points_from_history(history, f"--history-file {path}")
            except ReportError as e:
                lenient(f"--history-file {path} ", e)
                continue
            added = sum(1 for p in points if store.put(p))
            repo = repo or history.get("repo")
            if previous_latest is None:
                previous_latest = sibling_latest
            print(f"{path}: {len(points)} 点 (うち新しい点 {added})")
    except ReportError as e:
        bc.annotate("error", f"これまでの履歴を取れないのでサイトを作らない: {e}")
        if args.summary:
            with open(args.summary, "a", encoding="utf-8") as f:
                f.write(f"## 📊 ベンチマーク\n\n❌ これまでの履歴を取れないのでサイトを作らなかった "
                        f"(欠けた履歴で上書きしないため): {bc.md_cell(e)}\n\n")
        return 1

    added_backfill = backfill(store, repo, args.backfill, args.backfill_workflow,
                              os.environ.get("GITHUB_RUN_ID")) if args.backfill > 0 else 0

    added_current = 0
    if args.mode == "publish":
        for r in current:
            store.put(point_from_result(r))
            added_current += 1

    generated = dt.datetime.now(dt.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    history, dropped = build_history(store, repo, args.max_commits, generated)
    latest = build_latest(current, args.mode, server, repo, previous_latest, history)

    try:
        write_site(args.site_src, args.out_site, history, latest)
    except OSError as e:
        bc.annotate("error", f"サイトを書けない: {e}")
        return 1

    all_results = current + others
    counts = {
        "regressed": sum(len(regressed(r)) for r in current),
        "improved": sum(1 for r in current for b in r.get("benchmarks") or []
                        if (b.get("paired") or {}).get("verdict") == "improved"),
        "budget_violations": sum(len(budget_violations(r)) for r in all_results),
        "alloc_regressed": sum(len(alloc_regressed(r)) for r in current),
    }

    stats = {"added_current": added_current, "added_backfill": added_backfill, "dropped": dropped}
    markdown = summary_markdown(current, others, history, args.mode, args.pages_url, stats, missing)
    print(markdown)
    if args.summary:
        with open(args.summary, "a", encoding="utf-8") as f:
            f.write(markdown)

    if args.mode == "publish":
        # master の悪化は失敗にしない (揺れで誤判定することもある) が、見落とさないよう注釈を出す
        notes = []
        for r in current:
            label = bc.variant_label(r["variant"])
            for b in budget_violations(r):
                a = b["alloc"]
                notes.append(("ヒープ確保の予算超過", f"{label}: {b['name']} 確保 {bc.fmt_count(a.get('allocs'))} 回/op "
                              f"(予算 {bc.fmt_count(a.get('budget'))})"))
            for b in regressed(r):
                notes.append(("ベンチマークの悪化", f"{label}: {b['name']} {bc.fmt_change(b['paired'])}"))
            for b in alloc_regressed(r):
                a = b["alloc"]
                notes.append(("ヒープ確保の増加", f"{label}: {b['name']} 確保 {bc.fmt_count(a.get('base_allocs'))} → "
                              f"{bc.fmt_count(a.get('allocs'))} 回/op"))
        if len(notes) > MAX_ANNOTATIONS:
            rest = len(notes) - (MAX_ANNOTATIONS - 1)
            notes = notes[:MAX_ANNOTATIONS - 1] + [("ベンチマーク", f"ほか {rest} 件。ジョブのサマリーを見てほしい")]
        for title, message in notes:
            bc.annotate("warning", message, title=title)

    discord = args.mode == "publish" and bool(current) and (counts["regressed"] or counts["budget_violations"])
    if args.discord_md:
        if discord:
            bc.write_text(args.discord_md, discord_markdown(current, args.pages_url))
        elif os.path.exists(args.discord_md):
            os.remove(args.discord_md)

    if args.output:
        write_outputs(args.output, dict(counts, has_results="true" if current else "false",
                                        discord="true" if discord else "false"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
