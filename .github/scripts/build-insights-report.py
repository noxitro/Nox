#!/usr/bin/env python3
"""ビルド計測 (C++ Build Insights) の集計を、見やすいレポート (単一 HTML) と要約にする。

ci.yml の build-insights ジョブ (ubuntu) から呼ばれる。入力は各構成のビルドジョブが上げた
2 つのファイル (tools/build-insights/nox_build_insights.exe の出力と、ジョブが書いた付帯情報)。

    <data-dir>/bi-<構成>.json        nox_build_insights analyze の出力
    <data-dir>/bi-<構成>.meta.json   構成名・コミット・ワークスペースのパス・ビルド時間など

やること:
  1. パスを読みやすくする (リポジトリ相対、<MSVC>/、<WinSDK>/、<vcpkg>/)
  2. 比較元 (--base-dir。前回の main の同じファイル) があれば差分を計算する。
     実行時間は共有ランナーで揺れるので、差分は「フロントエンド全体に占める割合」でも見る
  3. tools/build-insights/site のページにデータを埋め込み、1 ファイルで開ける HTML を書く
  4. ジョブのサマリー (構成ごとの上位) を書く

使い方:
    python3 .github/scripts/build-insights-report.py --data-dir bi-data --base-dir bi-base \\
        --site-src tools/build-insights/site --out-html out/build-insights.html \\
        --summary "$GITHUB_STEP_SUMMARY" --report-url "<HTML の場所>"

読む環境変数: GITHUB_REPOSITORY, GITHUB_SERVER_URL, GITHUB_RUN_ID, GITHUB_SHA, GITHUB_REF_NAME

終了コード: 0 = 成功 (計測が 1 つも無いときも、その旨を要約に書いて 0)、1 = ページを書けない、2 = 引数の誤り。
"""

import argparse
import datetime as dt
import glob
import json
import os
import re
import sys

CONFIG_ORDER = ("Debug", "Release", "Master")
SUMMARY_TOP = 10
# 差分で「変わった」とみなす下限。共有ランナーの揺れ (実測で ±10% 前後) より大きくとる
DIFF_MIN_US = 20_000
DIFF_MIN_RATIO = 0.15
DIFF_MIN_SHARE_PT = 0.3
DIFF_TOP = 40

KIND_NAMES = {0: "class", 1: "function", 2: "variable", 3: "concept"}
# 特殊化の名前は "struct nox::Foo<int> " のように種類の語と末尾の空白が付いて届く
_SYMBOL_PREFIX = re.compile(r"^(?:struct|class|union|enum)\s+")


def symbol(name):
    return _SYMBOL_PREFIX.sub("", str(name).strip())


def warning(message):
    print(f"::warning::{message}")


# ---------------------------------------------------------------------------
# 読み込みと正規化
# ---------------------------------------------------------------------------

_SYSTEM_PATTERNS = [
    (re.compile(r"^.*[\\/]VC[\\/]Tools[\\/]MSVC[\\/][^\\/]+[\\/]", re.I), "<MSVC>/", "msvc"),
    (re.compile(r"^.*[\\/]VC[\\/]Auxiliary[\\/]VS[\\/]", re.I), "<MSVC>/", "msvc"),
    (re.compile(r"^.*[\\/]Windows Kits[\\/]10[\\/]Include[\\/][^\\/]+[\\/]", re.I), "<WinSDK>/", "sdk"),
    (re.compile(r"^.*[\\/]Windows Kits[\\/][^\\/]+[\\/]", re.I), "<WinSDK>/", "sdk"),
    (re.compile(r"^.*[\\/]vcpkg_installed[\\/][^\\/]+[\\/]", re.I), "<vcpkg>/", "vcpkg"),
]


class PathNormalizer:
    """ランナー上の絶対パスを、どのマシンで見ても同じ短い表記にする。"""

    def __init__(self, workspace):
        ws = (workspace or "").replace("/", "\\").rstrip("\\")
        self.workspace = ws.lower() + "\\" if ws else ""

    def __call__(self, path):
        if not path:
            return "", "other"
        p = path.replace("/", "\\")
        for pattern, prefix, cat in _SYSTEM_PATTERNS:
            m = pattern.match(p)
            if m:
                return prefix + p[m.end():].replace("\\", "/"), cat
        if self.workspace and p.lower().startswith(self.workspace):
            rel = p[len(self.workspace):].replace("\\", "/")
            cat = "generated" if rel.lower().startswith(("runtime/reflection_generated/", "runtime/build/")) else "project"
            return rel, cat
        return p.replace("\\", "/"), "other"


def read_json(path):
    try:
        with open(path, encoding="utf-8") as f:
            return json.load(f)
    except (OSError, ValueError) as e:
        warning(f"{path} を読めない: {e}")
        return None


def load_configs(data_dir):
    """<data-dir>/bi-<構成>.json と .meta.json を読み、構成名 → (data, meta) を返す。"""
    out = {}
    if not data_dir or not os.path.isdir(data_dir):
        return out
    for path in sorted(glob.glob(os.path.join(data_dir, "**", "bi-*.json"), recursive=True)):
        if path.endswith(".meta.json"):
            continue
        meta_path = path[:-len(".json")] + ".meta.json"
        data = read_json(path)
        meta = read_json(meta_path) if os.path.exists(meta_path) else {}
        if not isinstance(data, dict) or data.get("format") != 1:
            warning(f"{path} は集計の形式ではない")
            continue
        name = (meta or {}).get("config") or os.path.basename(path)[3:-5]
        out[name] = (data, meta or {})
    return out


def ordered(names):
    return sorted(names, key=lambda n: (CONFIG_ORDER.index(n) if n in CONFIG_ORDER else 99, n))


def build_config(name, data, meta):
    """ページに埋め込む形へ直す。パスは正規化した表に置き換える。"""
    norm = PathNormalizer(meta.get("workspace"))
    raw_paths = data.get("paths") or []
    paths = []
    cats = []
    for p in raw_paths:
        n, c = norm(p)
        paths.append(n)
        cats.append(c)

    totals = data.get("totals") or {}
    stats = data.get("stats") or {}
    build = data.get("build") or {}
    lost = int(stats.get("msvc_events_lost") or 0) + int(stats.get("msvc_buffers_lost") or 0)
    # 停止時の統計に出なくても、解析でイベントの欠落が見つかることがある
    dropped = data.get("analyze_result") == "FAILURE_DROPPED_EVENTS"

    headers = []
    for h in data.get("headers") or []:
        headers.append([
            h["p"], h["incl_us"], h["excl_us"], h["wctr_us"], h["passes"], h["pch_passes"],
            h["parses"], h["max_us"], h.get("parents") or [],
        ])
    headers.sort(key=lambda r: -r[1])

    units = []
    for u in data.get("units") or []:
        src, _ = norm(u.get("source"))
        units.append({
            "src": src,
            "inv": u.get("inv"),
            "start": u.get("start_us", 0),
            "fe": u.get("fe_us", 0),
            "fe_w": u.get("fe_wctr_us", 0),
            "be": u.get("be_us", -1),
            "be_w": u.get("be_wctr_us", -1),
            "pch": bool(u.get("pch")),
            "parses": u.get("parses", 0),
            "tree": u.get("tree"),
        })

    invocations = []
    for inv in data.get("invocations") or []:
        outputs = [norm(o)[0] for o in inv.get("outputs") or []]
        invocations.append([inv.get("type"), inv.get("id"), inv.get("start_us", 0), inv.get("dur_us", 0),
                            inv.get("wctr_us", 0), outputs])

    templates = []
    for t in data.get("templates") or []:
        templates.append({
            "n": symbol(t["name"]), "k": KIND_NAMES.get(t.get("kind"), "?"), "i": t["incl_us"], "e": t["excl_us"],
            "c": t["count"], "u": t["passes"], "s": [[symbol(sp[0])] + list(sp[1:]) for sp in t.get("specs") or []],
            "sc": t.get("spec_count", 0),
            "f": t.get("files") or [],
        })

    functions = []
    for f in data.get("functions") or []:
        functions.append({
            "n": f["name"], "d": f.get("decorated", ""), "t": f["dur_us"], "w": f["wctr_us"], "c": f["count"],
            "m": f["max_us"], "at": f.get("where", "cl"), "ic": f.get("inlinees", 0), "is": f.get("inline_size", 0),
            "ti": f.get("top_inlinees") or [],
        })

    # 関数のコード生成時間の合計は、切り詰め前の値を生成側から受け取る。
    # 古い形式 (この値が無い) では、残っている上位の関数から近似する
    if "codegen_cl_us" not in totals:
        totals = dict(totals)
        totals["codegen_cl_us"] = sum(f["t"] for f in functions if f["at"] == "cl")
        totals["codegen_ltcg_us"] = sum(f["t"] for f in functions if f["at"] != "cl")

    start = build.get("start_us", 0)
    end = build.get("end_us", 0)
    return {
        "name": name,
        "compiler": meta.get("compiler", "MSVC"),
        "templates": bool(meta.get("templates", True)),
        "build_seconds": meta.get("build_seconds"),
        "partial": lost > 0 or dropped,
        "lost": lost,
        "stop_result": stats.get("result"),
        "wall_us": max(0, end - start),
        "build_start_us": start,
        "processors": (data.get("trace") or {}).get("logical_processors", 0),
        "totals": totals,
        "paths": paths,
        "cats": cats,
        "headers": headers,
        "units": units,
        "invocations": invocations,
        "templates_list": templates,
        "functions": functions,
        # 上位だけに切り詰めてあるか。切り詰めてあれば、片側にだけ無い項目は「新規」「消えた」と言えない
        "templates_truncated": int(totals.get("template_names", len(templates))) > len(templates),
        "functions_truncated": int(totals.get("function_names", len(functions))) > len(functions),
    }


# ---------------------------------------------------------------------------
# 差分
# ---------------------------------------------------------------------------

def _diff_rows(cur_items, base_items, cur_total, base_total, cur_truncated=False, base_truncated=False):
    """(キー → 時間) 同士を比べ、変化の大きいものを返す。

    時間そのものに加え、全体に占める割合 (pt) も出す。ランナーが遅い日は全部が一様に
    遅くなるが、割合はあまり動かないので、本当に重くなったものを見分けやすい。

    片側が上位だけに切り詰めてあるとき、その側に無い項目は「順位が境界をまたいだだけ」の
    ことがあり、値が分からないので比べない。
    """
    rows = []
    for key in set(cur_items) | set(base_items):
        if (key not in base_items and base_truncated) or (key not in cur_items and cur_truncated):
            continue
        c = cur_items.get(key, 0)
        b = base_items.get(key, 0)
        delta = c - b
        share_c = c / cur_total * 100 if cur_total else 0
        share_b = b / base_total * 100 if base_total else 0
        share_pt = share_c - share_b
        big_abs = abs(delta) >= DIFF_MIN_US and (b == 0 or abs(delta) / b >= DIFF_MIN_RATIO)
        big_share = abs(share_pt) >= DIFF_MIN_SHARE_PT
        if not (big_abs and big_share):
            continue
        status = "new" if b == 0 else ("gone" if c == 0 else ("up" if delta > 0 else "down"))
        rows.append([key, b, c, round(share_b, 3), round(share_c, 3), status])
    rows.sort(key=lambda r: -abs(r[4] - r[3]))
    return rows[:DIFF_TOP]


def compute_diff(cur, base):
    if base is None:
        return None
    cur_fe = (cur["totals"] or {}).get("fe_us", 0)
    base_fe = (base["totals"] or {}).get("fe_us", 0)
    cur_be = (cur["totals"] or {}).get("be_us", 0)
    base_be = (base["totals"] or {}).get("be_us", 0)

    def codegen(cfg):
        t = cfg["totals"] or {}
        return t.get("codegen_cl_us", 0) + t.get("codegen_ltcg_us", 0)

    # 表示用の名前は別々の項目で同じになりうる (struct / class を外したテンプレート名、
    # 非装飾の関数名、同じソースを別のオプションで翻訳した翻訳単位)。上書きせずに足し込む
    def summed(pairs):
        out = {}
        for key, value in pairs:
            out[key] = out.get(key, 0) + value
        return out

    def header_map(cfg):
        return summed((cfg["paths"][h[0]], h[1]) for h in cfg["headers"])

    def template_map(cfg):
        return summed((t["n"], t["i"]) for t in cfg["templates_list"])

    def function_map(cfg):
        return summed((f["n"], f["t"]) for f in cfg["functions"])

    def unit_map(cfg):
        return summed((u["src"], u["fe"] + max(0, u["be"])) for u in cfg["units"])

    keys = ("fe_us", "be_us", "template_instantiations", "functions", "file_parses", "passes")
    totals = {k: [(base["totals"] or {}).get(k, 0), (cur["totals"] or {}).get(k, 0)] for k in keys}
    totals["wall_us"] = [base["wall_us"], cur["wall_us"]]
    both_templates = cur["templates"] and base["templates"]
    return {
        "reliable": not cur["partial"] and not base["partial"],
        "totals": totals,
        "headers": _diff_rows(header_map(cur), header_map(base), cur_fe, base_fe),
        "templates": _diff_rows(template_map(cur), template_map(base), cur_fe, base_fe,
                                cur["templates_truncated"], base["templates_truncated"]) if both_templates else [],
        # 関数の割合の分母は、切り詰め前のコード生成時間の合計 (cl とリンク時の和)
        "functions": _diff_rows(function_map(cur), function_map(base), codegen(cur), codegen(base),
                                cur["functions_truncated"], base["functions_truncated"]),
        "units": _diff_rows(unit_map(cur), unit_map(base), cur_fe + cur_be, base_fe + base_be),
    }


# ---------------------------------------------------------------------------
# 出力
# ---------------------------------------------------------------------------

def fmt_ms(us):
    if us is None:
        return "-"
    ms = us / 1000
    if ms >= 120_000:
        return f"{ms / 60_000:,.1f} 分"
    if ms >= 10_000:
        return f"{ms / 1000:,.1f} s"
    if ms >= 100:
        return f"{ms:,.0f} ms"
    return f"{ms:,.1f} ms"


def md_code(text, limit=90):
    """表のセルに置くコード表記。コード中では実体参照が展開されないので < > はそのまま残す。"""
    s = str(text)
    if len(s) > limit:
        s = s[: limit - 1] + "…"
    return "`" + s.replace("`", "'").replace("|", "\\|") + "`"


def summary_markdown(configs, diffs, report_url, missing):
    lines = ["## 🔎 Build Insights (C++ のビルド時間の内訳)", ""]
    if report_url:
        lines += [f"**[詳しいレポートを開く]({report_url})** — ヘッダ・インクルードツリー・テンプレート・関数・翻訳単位・前回 main との差分", ""]
    if not configs:
        lines += ["計測結果が 1 つも無い (計測に失敗したか、MSVC のビルドが走っていない)。", ""]
        return "\n".join(lines) + "\n"
    for name in missing:
        lines.append(f"- ⚠️ {name}: 計測結果が無い (ビルドジョブの Build Insights のステップを参照)")
    for cfg in configs:
        if cfg["partial"]:
            count = f" {cfg['lost']:,} 件" if cfg["lost"] else ""
            lines.append(f"- ⚠️ {cfg['name']}: イベントが{count}欠けた。時間が実際より長く出ている項目がある (差分は出さない)")
    if missing or any(c["partial"] for c in configs):
        lines.append("")

    # 構成ごとの概要
    lines += ["| 構成 | ビルド (実時間) | フロントエンド合計 | バックエンド合計 | 翻訳単位 | テンプレート展開 | 前回 main 比 |",
              "|---|---:|---:|---:|---:|---:|---|"]
    for cfg in configs:
        t = cfg["totals"]
        d = diffs.get(cfg["name"])
        if d is None:
            vs = "比較元なし"
        elif not d["reliable"]:
            vs = "比較しない (欠落あり)"
        else:
            b, c = d["totals"]["fe_us"]
            vs = f"FE {((c - b) / b * 100) if b else 0:+.1f}%"
            b, c = d["totals"]["template_instantiations"]
            if cfg["templates"] and b:
                vs += f" / 展開数 {((c - b) / b * 100):+.1f}%"
        inst = f"{t.get('template_instantiations', 0):,}" if cfg["templates"] else "(取っていない)"
        lines.append(f"| {cfg['name']} | {fmt_ms(cfg['wall_us'])} | {fmt_ms(t.get('fe_us'))} | {fmt_ms(t.get('be_us'))} | "
                     f"{t.get('passes', 0):,} | {inst} | {vs} |")
    lines.append("")

    # 重いヘッダ (構成ごと)
    for cfg in configs:
        fe = cfg["totals"].get("fe_us") or 1
        lines += [f"<details{' open' if cfg is configs[0] else ''}><summary><b>{cfg['name']}</b>: 重いヘッダ・テンプレート・関数 (上位 {SUMMARY_TOP})</summary>", ""]
        lines += ["**ヘッダ** (全翻訳単位での解析時間の合計。PCH 内のものは PCH を作るときに 1 回だけ数える)", "",
                  "| ヘッダ | 合計 | FE 比 | 取り込んだ翻訳単位 | |", "|---|---:|---:|---:|---|"]
        for h in cfg["headers"][:SUMMARY_TOP]:
            badge = "PCH 内" if h[5] and h[5] == h[4] else ""
            lines.append(f"| {md_code(cfg['paths'][h[0]])} | {fmt_ms(h[1])} | {h[1] / fe * 100:.1f}% | {h[4]:,} | {badge} |")
        lines.append("")
        if cfg["templates"] and cfg["templates_list"]:
            lines += ["**テンプレート** (primary template ごと。再帰の二重計上は除く)", "",
                      "| テンプレート | 合計 | 回数 | 翻訳単位 |", "|---|---:|---:|---:|"]
            for t in cfg["templates_list"][:SUMMARY_TOP]:
                lines.append(f"| {md_code(t['n'])} | {fmt_ms(t['i'])} | {t['c']:,} | {t['u']:,} |")
            lines.append("")
        if cfg["functions"]:
            lines += ["**関数のコード生成**", "", "| 関数 | 合計 | 回数 | 場所 |", "|---|---:|---:|---|"]
            for f in cfg["functions"][:SUMMARY_TOP]:
                where = {"cl": "cl", "ltcg": "リンク時 (LTCG)", "both": "cl + LTCG"}.get(f["at"], f["at"])
                lines.append(f"| {md_code(f['n'])} | {fmt_ms(f['t'])} | {f['c']:,} | {where} |")
            lines.append("")
        d = diffs.get(cfg["name"])
        if d and d["reliable"] and d["headers"]:
            lines += ["**前回 main から大きく変わったヘッダ**", "", "| ヘッダ | 前回 | 今回 | FE 比 |", "|---|---:|---:|---:|"]
            for r in d["headers"][:SUMMARY_TOP]:
                lines.append(f"| {md_code(r[0])} | {fmt_ms(r[1])} | {fmt_ms(r[2])} | {r[3]:.1f}% → {r[4]:.1f}% |")
            lines.append("")
        lines += ["</details>", ""]
    return "\n".join(lines) + "\n"


def write_html(site_src, out_html, payload):
    def read(name):
        with open(os.path.join(site_src, name), encoding="utf-8") as f:
            return f.read()

    html = read("index.html")
    css = read("style.css")
    js = read("app.js")
    # </script> がデータ中に現れても閉じタグとして解釈されないようにする。<!-- も、後ろに <script が
    # 続くとスクリプトの終わりの解釈が変わるので崩しておく (JSON の文字列の中の \u0021 は ! として読まれる)
    data = (json.dumps(payload, ensure_ascii=False, separators=(",", ":"))
            .replace("</", "<\\/").replace("<!--", "<\\u0021--"))
    replacements = {
        '<link rel="stylesheet" href="style.css">': f"<style>\n{css}\n</style>",
        '<script src="data.js"></script>': f"<script>window.BI_DATA = {data};</script>",
        '<script src="app.js"></script>': f"<script>\n{js}\n</script>",
    }
    for old, new in replacements.items():
        if old not in html:
            raise ValueError(f"index.html に {old} が無い")
        html = html.replace(old, new)
    os.makedirs(os.path.dirname(os.path.abspath(out_html)), exist_ok=True)
    with open(out_html, "w", encoding="utf-8", newline="\n") as f:
        f.write(html)


def main():
    for stream in (sys.stdout, sys.stderr):
        if hasattr(stream, "reconfigure"):
            stream.reconfigure(encoding="utf-8", errors="replace")

    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--data-dir", required=True, help="今回の計測 (bi-<構成>.json と .meta.json) があるディレクトリ")
    ap.add_argument("--base-dir", help="比較元 (前回の main) の同じファイルがあるディレクトリ。無くてもよい")
    ap.add_argument("--base-sha", help="比較元のコミット")
    ap.add_argument("--base-run-url", help="比較元の実行の URL")
    ap.add_argument("--site-src", default="tools/build-insights/site", help="ページのソース")
    ap.add_argument("--out-html", help="書き出す HTML")
    ap.add_argument("--summary", help="Markdown を追記するファイル ($GITHUB_STEP_SUMMARY)")
    ap.add_argument("--report-url", help="要約から張るレポートの URL")
    ap.add_argument("--skip-html", action="store_true", help="HTML は書かず、要約だけ書く (HTML を上げた後に URL 付きで要約を書くとき)")
    ap.add_argument("--expect", default="", help="計測があるはずの構成 (カンマ区切り)。欠けていれば要約で知らせる")
    args = ap.parse_args()

    current = load_configs(args.data_dir)
    base = load_configs(args.base_dir) if args.base_dir else {}

    configs = [build_config(n, *current[n]) for n in ordered(current)]
    base_configs = {n: build_config(n, *base[n]) for n in base}
    diffs = {c["name"]: compute_diff(c, base_configs.get(c["name"])) for c in configs}
    expected = [e for e in (s.strip() for s in args.expect.split(",")) if e]
    missing = [e for e in expected if e not in current]

    server = os.environ.get("GITHUB_SERVER_URL", "https://github.com")
    repo = os.environ.get("GITHUB_REPOSITORY", "")
    run_id = os.environ.get("GITHUB_RUN_ID", "")
    first_meta = next(iter(current.values()))[1] if current else {}
    sha = first_meta.get("sha") or os.environ.get("GITHUB_SHA", "")
    commit = {
        "sha": sha,
        "ref": first_meta.get("ref") or os.environ.get("GITHUB_REF_NAME", ""),
        "subject": first_meta.get("subject", ""),
        "run_url": f"{server}/{repo}/actions/runs/{run_id}" if repo and run_id else "",
        "commit_url": f"{server}/{repo}/commit/{sha}" if repo and sha else "",
        "repo": repo,
    }
    payload = {
        "generated": dt.datetime.now(dt.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "commit": commit,
        "base": {"sha": args.base_sha or "", "run_url": args.base_run_url or ""} if base_configs else None,
        "configs": configs,
        "diffs": diffs,
        "missing": missing,
    }

    if not args.skip_html:
        if not args.out_html:
            print("--out-html が要る", file=sys.stderr)
            return 2
        try:
            write_html(args.site_src, args.out_html, payload)
        except (OSError, ValueError) as e:
            print(f"::error::レポートを書けない: {e}")
            return 1
        print(f"レポートを書いた: {args.out_html} ({os.path.getsize(args.out_html) / 1024 / 1024:.1f} MiB)")

    if args.summary:
        with open(args.summary, "a", encoding="utf-8") as f:
            f.write(summary_markdown(configs, diffs, args.report_url, missing))
    return 0


if __name__ == "__main__":
    sys.exit(main())
