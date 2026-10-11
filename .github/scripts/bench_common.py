"""ベンチマーク (bench_test.exe) の結果を扱う共通処理。

bench-run.py (Windows ランナーで計測する) と bench-report.py (ubuntu で履歴とサイトを
作る) の両方から import する。統計・書式・スキーマの定数・data/*.js の読み書きを
ここに集め、2 つのスクリプトで判定や表示がずれないようにする。

統計の考え方:
  - GitHub のランナーは VM ごとに CPU が違い、同じ VM でも時間とともに速さが揺れる。
    そこで head (今のコミット) と base (比較対象) を同じ VM で交互に測り、ラウンド
    ごとの比 head / base を見る。揺れは両方に同じように乗るので、比にすると打ち消す
  - 比の対数の中央値を推定値にし、ブートストラップ (2000 回) で 95 % 信頼区間を出す。
    外れ値 (たまたま割り込みが重なったラウンド) に引きずられないよう平均は使わない
  - 乱数の種はベンチマーク名から作る。同じ入力なら何度計算しても同じ判定になり、
    再実行で判定が入れ替わって混乱するのを防ぐ
  - 判定のしきい値は既定 5 %。複数スレッドで動くもの (JobSystem など) はスケジュール
    の揺れが大きいので 2 倍にする
  - 「変化なし」は同等性の主張なので、区間がしきい値の内側 (1 − しきい値 〜 1 + しきい値)
    に収まったときだけ出す。有意でもなく、しきい値の内側とも言えないものは「ばらつき大」
"""

import json
import math
import os
import random
import re
import zlib

# ---------------------------------------------------------------------------
# スキーマと構成
# ---------------------------------------------------------------------------

RAW_SCHEMA = "nox-bench-raw/1"
RUN_SCHEMA = "nox-bench-run/1"
HISTORY_SCHEMA = "nox-bench-history/1"
LATEST_SCHEMA = "nox-bench-latest/1"

# 履歴に入れる構成 (表示順)。Debug はアサート入りで時間に意味が無いので、
# スモーク (動くかどうか) だけ走らせて履歴には入れない。
VARIANT_LABELS = {
    "msvc-master": "MSVC / Master",
    "msvc-release": "MSVC / Release",
    "clangcl-master": "ClangCL / Master",
    "clangcl-release": "ClangCL / Release",
}
HISTORY_VARIANTS = tuple(VARIANT_LABELS)

COMPILERS = ("MSVC", "ClangCL")
CONFIGS = ("Debug", "Release", "Master")
_COMPILER_NAMES = {c.lower(): c for c in COMPILERS}

# bench-run.py がアーティファクトの中で探す exe の名前
BENCH_EXE_NAME = "bench_test.exe"


def variant_id(compiler, config):
    """("MSVC", "Master") → "msvc-master" """
    return f"{compiler}-{config}".lower()


def variant_label(variant):
    """"msvc-master" → "MSVC / Master"。履歴に入れない構成 (Debug) も同じ形にする。"""
    if variant in VARIANT_LABELS:
        return VARIANT_LABELS[variant]
    compiler, _, config = (variant or "").partition("-")
    return f"{_COMPILER_NAMES.get(compiler, compiler)} / {config.capitalize()}"


# ---------------------------------------------------------------------------
# 判定の表示
# ---------------------------------------------------------------------------

VERDICTS = ("regressed", "improved", "unchanged", "noisy", "insufficient")
# 履歴 (列形式) の v 列に入れる符号。insufficient と比較なしは null にする
VERDICT_CODES = {"regressed": 1, "improved": -1, "unchanged": 0, "noisy": 2}
# 色だけに頼らない (色覚の差やテキストのみの表示でも読める) よう、必ず記号と文字を添える
VERDICT_GLYPHS = {
    "regressed": "▲",
    "improved": "▼",
    "unchanged": "≈",
    "noisy": "？",
    "insufficient": "—",
}
VERDICT_LABELS = {
    "regressed": "悪化",
    "improved": "改善",
    "unchanged": "変化なし",
    "noisy": "ばらつき大",
    # 対が 3 組未満で判定しない。結果ページ (tools/bench-site/app.js) も同じ文言を使う
    "insufficient": "回数不足",
}
NO_BASE_GLYPH = "—"
NO_BASE_LABEL = "比較なし"

# base の種類を言葉にしたもの。"explicit" は手元で --base-exe を渡したとき
BASE_KIND_LABELS = {
    "previous-main": "直前の main",
    "merge-base": "分岐元",
    "latest-main": "最新の main",
    "explicit": "指定した exe",
    # 既定ブランチを master から main へ改名する前の結果に残っている名前
    "previous-master": "直前の main",
    "latest-master": "最新の main",
}


def verdict_text(verdict):
    """"regressed" → "▲ 悪化"。None (比較なし) は "— 比較なし"。"""
    if verdict not in VERDICT_LABELS:
        return f"{NO_BASE_GLYPH} {NO_BASE_LABEL}"
    return f"{VERDICT_GLYPHS[verdict]} {VERDICT_LABELS[verdict]}"


def base_kind_label(kind):
    return BASE_KIND_LABELS.get(kind, kind or "不明")


# ---------------------------------------------------------------------------
# 統計
# ---------------------------------------------------------------------------

DEFAULT_THRESHOLD = 0.05
MIN_PAIRS = 3
BOOTSTRAP_RESAMPLES = 2000
CI_LEVEL = 0.95


def is_num(x):
    """bool と NaN / 無限大を除いた数値か。"""
    return isinstance(x, (int, float)) and not isinstance(x, bool) and math.isfinite(x)


def finite(xs):
    """数値 (有限) だけを float で取り出す。raw JSON の欠損や NaN を統計に混ぜない。"""
    return [float(x) for x in (xs or []) if is_num(x)]


def _quantile_sorted(s, q):
    n = len(s)
    if n == 0:
        return None
    if n == 1:
        return s[0]
    pos = (n - 1) * q
    lo = int(math.floor(pos))
    hi = min(lo + 1, n - 1)
    return s[lo] + (s[hi] - s[lo]) * (pos - lo)


def quantile(xs, q):
    """線形補間の分位点 (numpy の "linear"、R の type 7 と同じ)。空なら None。"""
    return _quantile_sorted(sorted(xs), q)


def median(xs):
    return quantile(xs, 0.5)


def mean(xs):
    xs = list(xs)
    return sum(xs) / len(xs) if xs else None


def pstdev(xs):
    """母標準偏差 (n で割る)。"""
    xs = list(xs)
    if not xs:
        return None
    m = sum(xs) / len(xs)
    return math.sqrt(sum((x - m) ** 2 for x in xs) / len(xs))


def cv(xs):
    """変動係数 = 母標準偏差 / 平均。平均が 0 なら 0。"""
    xs = list(xs)
    if not xs:
        return None
    m = mean(xs)
    if m == 0:
        return 0.0
    return pstdev(xs) / m


def mad(xs):
    """中央絶対偏差 (1.4826 倍などの補正はしない)。"""
    xs = list(xs)
    if not xs:
        return None
    med = median(xs)
    return median([abs(x - med) for x in xs])


def summarize_side(round_samples, cycles=None):
    """片側 (head か base) の統計。round_samples はラウンドごとの samples_ns の配列。

    rounds はラウンドごとの中央値 (対にして比べる単位)。median などの分位点は全ラウンドの
    サンプルをまとめて取る (1 回の計測のばらつきを表に出すため)。cv はラウンドの中央値の
    ばらつき (ラウンドをまたぐ揺れ、つまり VM の揺れの大きさ)。
    """
    rounds = [median(s) for s in round_samples if s]
    pooled = sorted(x for s in round_samples for x in s)
    cyc = finite(cycles)
    return {
        "rounds": rounds,
        "median": _quantile_sorted(pooled, 0.5),
        "q1": _quantile_sorted(pooled, 0.25),
        "q3": _quantile_sorted(pooled, 0.75),
        "p10": _quantile_sorted(pooled, 0.10),
        "p90": _quantile_sorted(pooled, 0.90),
        "min": pooled[0] if pooled else None,
        "cv": cv(rounds) if rounds else None,
        "cycles": median(cyc) if cyc else None,
    }


def effective_threshold(threshold, threads):
    """複数スレッドのベンチマークはしきい値を 2 倍にする (スケジュールの揺れが大きい)。"""
    return threshold * 2 if (threads or 1) > 1 else threshold


def paired_analysis(name, head_rounds, base_rounds, threshold=DEFAULT_THRESHOLD, threads=1):
    """同じラウンドの head / base を対にして比べる。

    head_rounds[i] と base_rounds[i] は同じラウンド (同じ VM で続けて測った) の中央値。
    どちらかが None や 0 以下の組は捨てる。3 組未満なら判定しない ("insufficient")。

    判定 (thr はしきい値。複数スレッドは 2 倍):
      regressed : 区間の下端 > 1 かつ 推定値 ≥ 1 + thr (有意に遅く、しきい値以上)
      improved  : 区間の上端 < 1 かつ 推定値 ≤ 1 − thr (有意に速く、しきい値以上)
      unchanged : 区間全体が (1 − thr, 1 + thr) の内側 (しきい値ほどの変化は無いと言える)
      noisy     : それ以外 (区間がしきい値の外まで伸びていて、変化の有無を言えない)
    """
    lr = []
    for h, b in zip(head_rounds, base_rounds):
        if is_num(h) and is_num(b) and h > 0 and b > 0:
            lr.append(math.log(h / b))
    n = len(lr)
    if n == 0:
        return {"ratio": None, "ci_low": None, "ci_high": None, "change": None,
                "verdict": "insufficient", "pairs": 0}
    ratio = math.exp(median(lr))
    if n < MIN_PAIRS:
        return {"ratio": ratio, "ci_low": None, "ci_high": None, "change": ratio - 1,
                "verdict": "insufficient", "pairs": n}

    # 名前から種を作り、同じ入力なら必ず同じ区間になるようにする
    rng = random.Random(zlib.crc32(name.encode("utf-8")))
    meds = []
    for _ in range(BOOTSTRAP_RESAMPLES):
        meds.append(_quantile_sorted(sorted(rng.choices(lr, k=n)), 0.5))
    meds.sort()
    alpha = (1 - CI_LEVEL) / 2
    ci_low = math.exp(_quantile_sorted(meds, alpha))
    ci_high = math.exp(_quantile_sorted(meds, 1 - alpha))

    thr = effective_threshold(threshold, threads)
    if ci_low > 1 and ratio - 1 >= thr:
        verdict = "regressed"
    elif ci_high < 1 and 1 - ratio >= thr:
        verdict = "improved"
    elif ci_low > 1 - thr and ci_high < 1 + thr:
        # 同等性の判定: 区間全体がしきい値の内側にあるときだけ「変化なし」と言う
        verdict = "unchanged"
    else:
        # 区間がしきい値の外まで伸びている (揺れが大きい、またはしきい値前後の変化で
        # 有意とまでは言えない)。「変化なし」とは言えないので分けて見せる
        verdict = "noisy"
    return {"ratio": ratio, "ci_low": ci_low, "ci_high": ci_high, "change": ratio - 1,
            "verdict": verdict, "pairs": n}


def geomean_change(ratios):
    """比の幾何平均 - 1。全体として速くなったか遅くなったかの 1 つの数字。"""
    logs = [math.log(r) for r in ratios if is_num(r) and r > 0]
    if not logs:
        return None
    return math.exp(sum(logs) / len(logs)) - 1


def alloc_verdict(head_allocs, base_allocs):
    """ヒープ確保回数 (1 op あたり) を base と比べる。確保回数は決定的なので区間は要らない。"""
    if not is_num(head_allocs) or not is_num(base_allocs):
        return None
    h = round(head_allocs, 6)
    b = round(base_allocs, 6)
    if h > b:
        return "regressed"
    if h < b:
        return "improved"
    return "unchanged"


# ---------------------------------------------------------------------------
# 丸めと書式
# ---------------------------------------------------------------------------

def _compact(v):
    # 12340.0 を 12340 と書き、JSON を少しでも小さくする
    if v == 0:
        return 0
    if v.is_integer() and abs(v) < 1e15:
        return int(v)
    return v


def sig_round(x, digits=4):
    """有効数字 digits 桁に丸める。数値でなければ None。履歴を小さく保つために使う。"""
    if not is_num(x):
        return None
    return _compact(float(f"{float(x):.{digits}g}"))


def dec_round(x, places=4):
    """小数点以下 places 桁に丸める (比や信頼区間向け)。"""
    if not is_num(x):
        return None
    return _compact(round(float(x), places))


def _sig3(v):
    """0 以上の v を有効数字 3 桁の文字列にする (末尾の 0 も残す: 1.00)。"""
    if v == 0:
        return "0"
    decimals = max(0, 2 - int(math.floor(math.log10(v))))
    return f"{v:.{decimals}f}"


_TIME_UNITS = (("s", 1e9), ("ms", 1e6), ("µs", 1e3), ("ns", 1.0))


def fmt_time(ns):
    """ナノ秒を単位付きで有効数字 3 桁にする: 0.812 ns / 12.3 ns / 1.23 µs / 45.6 ms。"""
    if not is_num(ns):
        return "—"
    if ns == 0:
        return "0 ns"
    sign = "−" if ns < 0 else ""
    # 先に 3 桁へ丸めてから単位を選ぶ (999.6 ns を "1000 ns" でなく "1.00 µs" にする)
    v = float(f"{abs(ns):.3g}")
    for unit, scale in _TIME_UNITS:
        if v >= scale or unit == "ns":
            return f"{sign}{_sig3(v / scale)} {unit}"
    return "—"


def fmt_pct(x, digits=1):
    """比率の変化 (0.034) を "+3.4 %" に。負号は U+2212 (−) を使う。"""
    if not is_num(x):
        return "—"
    v = x * 100
    if abs(v) >= 100:
        digits = 0
    s = f"{abs(v):.{digits}f}"
    if float(s) == 0:
        return f"±{s} %"
    return ("+" if v > 0 else "−") + s + " %"


def _pct_bare(x):
    s = fmt_pct(x)
    return s[:-2] if s.endswith(" %") else s


def fmt_change(paired):
    """paired の結果を "+3.4 % (+1.0 〜 +6.0)" に。区間が無ければ推定値だけ。"""
    if not paired or not is_num(paired.get("ratio")):
        return "—"
    text = fmt_pct(paired["ratio"] - 1)
    lo, hi = paired.get("ci_low"), paired.get("ci_high")
    if is_num(lo) and is_num(hi):
        text += f" ({_pct_bare(lo - 1)} 〜 {_pct_bare(hi - 1)})"
    return text


def _count_plain(v):
    """0 以上の v: 整数ならそのまま、端数は有効数字 3 桁 (末尾の 0 は落とす)、100 万以上は M。"""
    if v >= 1e6:
        return f"{_sig3(float(f'{v:.3g}') / 1e6)}M"
    if abs(v - round(v)) < 1e-9:
        return str(int(round(v)))
    s = _sig3(float(f"{v:.3g}"))
    if "." in s:
        s = s.rstrip("0").rstrip(".")
    return s


# 1 未満の回数を "1/N" と書くときの許容。1/v が最寄りの整数 N から N の 2 % より離れていれば
# (0.3 = 1/3.33 など) 分数にせず有効数字 3 桁で書く
RECIPROCAL_TOLERANCE = 0.02


def fmt_count(x):
    """1 op あたりの確保回数など。結果ページと同じ書き方にする。

    0 は "0"。0 と 1 の間は逆数の分数 ("1/512" = 512 op に 1 回)。1/v を最寄りの整数に丸め、
    ずれが 2 % を超える (または 1/1 になる) ときだけ有効数字 3 桁の小数に戻す。
    1 以上は整数ならそのまま、端数は有効数字 3 桁。
    """
    if not is_num(x):
        return "—"
    if x == 0:
        return "0"
    sign = "−" if x < 0 else ""
    v = abs(x)
    if v < 1:
        inv = 1 / v
        if math.isfinite(inv):
            n = round(inv)
            if n >= 2 and abs(inv - n) <= RECIPROCAL_TOLERANCE * n:
                return f"{sign}1/{n}"
    return sign + _count_plain(v)


def fmt_bytes(x):
    """バイト数。1 未満も分数にはしない (0.125 B は "0.125 B")。"""
    if not is_num(x):
        return "—"
    v = abs(x)
    for unit, scale in (("MiB", 1024.0 ** 2), ("KiB", 1024.0)):
        if v >= scale:
            return f"{_sig3(float(f'{v / scale:.3g}'))} {unit}"
    return f"{_count_plain(v)} B"


_CPU_NOISE = re.compile(
    r"\((?:R|TM|tm|r)\)|\bCPU\b|@\s*[\d.]+\s*GHz|\b\d+-Core\b|\bProcessor\b|\bwith Radeon.*$",
    re.IGNORECASE,
)


def short_cpu(cpu):
    """"Intel(R) Xeon(R) Platinum 8370C CPU @ 2.80GHz" → "Intel Xeon Platinum 8370C"。"""
    if not cpu:
        return "不明な CPU"
    s = re.sub(r"\s+", " ", _CPU_NOISE.sub(" ", cpu)).strip()
    return s or cpu


def short_sha(sha):
    return (sha or "")[:7] or "-------"


# ---------------------------------------------------------------------------
# JSON と data/*.js の読み書き
# ---------------------------------------------------------------------------

HISTORY_JS_PREFIX = "window.NOX_BENCH_HISTORY = "
LATEST_JS_PREFIX = "window.NOX_BENCH_LATEST = "
JS_SUFFIX = ";\n"


def sanitize(obj):
    """NaN / Infinity を null に置き換える。JSON として (ブラウザで) 読めなくなるのを防ぐ。"""
    if isinstance(obj, float) and not math.isfinite(obj):
        return None
    if isinstance(obj, dict):
        return {k: sanitize(v) for k, v in obj.items()}
    if isinstance(obj, (list, tuple)):
        return [sanitize(v) for v in obj]
    return obj


def dumps_compact(obj):
    return json.dumps(sanitize(obj), ensure_ascii=False, separators=(",", ":"), allow_nan=False)


def read_json(path):
    # utf-8-sig: 誰かが BOM 付きで保存しても読めるように
    with open(path, encoding="utf-8-sig") as f:
        return json.load(f)


def write_text(path, text):
    parent = os.path.dirname(path)
    if parent:
        os.makedirs(parent, exist_ok=True)
    # Windows でも LF で書く (成果物をバイト単位で比べられるように)
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)


def write_json(path, obj, indent=1):
    if indent is None:
        text = dumps_compact(obj)
    else:
        text = json.dumps(sanitize(obj), ensure_ascii=False, indent=indent, allow_nan=False)
    write_text(path, text + "\n")


def to_js(prefix, obj):
    """`window.NOX_BENCH_X = <json>;\\n`。<script src> で読めば file:// でも動く (fetch は不可)。"""
    return prefix + ("null" if obj is None else dumps_compact(obj)) + JS_SUFFIX


def parse_js(text, prefix):
    """to_js の逆。形が違えば ValueError。"""
    t = text.lstrip("\ufeff").strip()
    if not t.startswith(prefix.strip()):
        raise ValueError(f"先頭が {prefix.strip()!r} ではない")
    body = t[len(prefix.strip()):].strip()
    if body.endswith(";"):
        body = body[:-1]
    return json.loads(body)


def write_js(path, prefix, obj):
    write_text(path, to_js(prefix, obj))


def read_js(path, prefix):
    with open(path, encoding="utf-8") as f:
        return parse_js(f.read(), prefix)


# ---------------------------------------------------------------------------
# GitHub Actions の注釈
# ---------------------------------------------------------------------------

def escape_annotation(s):
    return str(s).replace("%", "%25").replace("\r", "%0D").replace("\n", "%0A")


def escape_property(s):
    return escape_annotation(s).replace(":", "%3A").replace(",", "%2C")


def annotate(level, message, title=None):
    """::warning:: / ::error:: / ::notice:: を 1 行出す。"""
    props = f" title={escape_property(title)}" if title else ""
    print(f"::{level}{props}::{escape_annotation(message)}", flush=True)


# ---------------------------------------------------------------------------
# Markdown (ジョブのサマリー)
# ---------------------------------------------------------------------------

TABLE_COLUMNS = ("ベンチマーク", "1 op", "中央値", "変化 (95% CI)", "判定", "確保/op")


def md_cell(s):
    return str(s).replace("|", "\\|").replace("\r", " ").replace("\n", " ")


def md_table(header, rows):
    lines = ["| " + " | ".join(header) + " |", "|" + "---|" * len(header)]
    for row in rows:
        lines.append("| " + " | ".join(md_cell(c) for c in row) + " |")
    return "\n".join(lines) + "\n"


def alloc_text(bench):
    """確保/op の欄: "0 ✓" / "2 ✕ 予算 1" / "▲ 3 (base 2)"。"""
    alloc = bench.get("alloc") or {}
    allocs = alloc.get("allocs")
    text = fmt_count(allocs)
    if alloc.get("verdict") == "regressed":
        text = f"▲ {text} (base {fmt_count(alloc.get('base_allocs'))})"
    elif alloc.get("verdict") == "improved":
        text = f"▼ {text} (base {fmt_count(alloc.get('base_allocs'))})"
    budget_ok = alloc.get("budget_ok")
    if budget_ok is True:
        text += " ✓"
    elif budget_ok is False:
        text += f" ✕ 予算 {fmt_count(alloc.get('budget'))}"
    if alloc.get("stable") is False:
        text += " (不安定)"
    return text


def bench_cells(bench):
    """TABLE_COLUMNS の順の 1 行。"""
    title = bench.get("title") or ""
    name_cell = f"{title} `{bench.get('name', '')}`" if title else f"`{bench.get('name', '')}`"
    threads = bench.get("threads") or 1
    per = bench.get("per") or "op"
    if threads > 1:
        per += f" ({threads} スレッド)"
    head = bench.get("head") or {}
    paired = bench.get("paired")
    verdict = paired.get("verdict") if paired else None
    return [name_cell, per, fmt_time(head.get("median")), fmt_change(paired), verdict_text(verdict), alloc_text(bench)]


def is_notable(bench):
    paired = bench.get("paired") or {}
    alloc = bench.get("alloc") or {}
    return (paired.get("verdict") in ("regressed", "improved", "noisy")
            or alloc.get("verdict") in ("regressed", "improved")
            or alloc.get("budget_ok") is False)


def notable_sort_key(bench):
    """目立つものほど前へ: 予算超過 → 悪化 (大きい順) → 確保増 → 改善 → 確保減 → ばらつき大。"""
    paired = bench.get("paired") or {}
    alloc = bench.get("alloc") or {}
    change = paired.get("change") if is_num(paired.get("change")) else 0.0
    if alloc.get("budget_ok") is False:
        return (0, -change)
    v = paired.get("verdict")
    if v == "regressed":
        return (1, -change)
    if alloc.get("verdict") == "regressed":
        return (2, -change)
    if v == "improved":
        return (3, change)
    if alloc.get("verdict") == "improved":
        return (4, change)
    return (5, -abs(change))


def count_text(summary):
    """"▲ 悪化 1 / ▼ 改善 0 / ≈ 変化なし 25 / ？ ばらつき大 2" (回数不足は 0 なら省く)。"""
    s = summary or {}
    parts = [f"{VERDICT_GLYPHS[v]} {VERDICT_LABELS[v]} {s.get(v, 0)}"
             for v in ("regressed", "improved", "unchanged", "noisy")]
    if s.get("insufficient"):
        parts.append(f"{VERDICT_GLYPHS['insufficient']} {VERDICT_LABELS['insufficient']} {s['insufficient']}")
    return " / ".join(parts)


def env_text(env):
    env = env or {}
    bits = [short_cpu(env.get("cpu"))]
    if env.get("logical_cpus"):
        bits.append(f"{env['logical_cpus']} 論理コア")
    if env.get("image"):
        bits.append(env["image"])
    return ", ".join(bits)


def base_text(result):
    base = result.get("base")
    if not base:
        return "比較対象なし"
    run = f" (#{base['run_number']})" if base.get("run_number") else ""
    sha = f" `{short_sha(base.get('sha'))}`" if base.get("sha") else ""
    return f"{base_kind_label(base.get('kind'))}{sha}{run}"


def variant_markdown(result, level="##"):
    """1 構成ぶんのサマリー (bench-run.py がジョブのサマリーに足す)。"""
    label = variant_label(result.get("variant"))
    benches = result.get("benchmarks") or []
    summary = result.get("summary") or {}
    out = []
    if result.get("smoke"):
        out.append(f"{level} 📊 ベンチマーク: {label} (スモーク)\n\n")
        if not benches:
            out.append("結果が無い。\n\n")
            return "".join(out)
        out.append(f"各ベンチマークを 1 回ずつ動かし、落ちないことだけを確かめた ({len(benches)} 件)。"
                   "アサート入りのビルドなので時間は参考にならない。\n\n")
        out.append(f"<details><summary>全 {len(benches)} 件</summary>\n\n")
        rows = [bench_cells(b) for b in benches]
        # 変化と判定の列は比較しないスモークでは空なので省く
        out.append(md_table(("ベンチマーク", "1 op", "時間 (参考)", "確保/op"), [[r[0], r[1], r[2], r[5]] for r in rows]))
        out.append("\n</details>\n\n")
        return "".join(out)

    out.append(f"{level} 📊 ベンチマーク: {label}\n\n")
    if not benches:
        out.append("結果が無い。\n\n")
        return "".join(out)
    thr = result.get("threshold", DEFAULT_THRESHOLD)
    if result.get("base"):
        out.append(f"{count_text(summary)} ・ 全体 (幾何平均) {fmt_pct(summary.get('geomean_change'))}\n\n")
        out.append(f"CPU: {env_text(result.get('env'))} ・ 比較対象: {base_text(result)} ・ "
                   f"同じ VM で交互に {result.get('rounds', 0)} ラウンド ・ "
                   f"しきい値 ±{thr * 100:g} % (複数スレッドは ±{thr * 200:g} %)\n\n")
    else:
        out.append(f"CPU: {env_text(result.get('env'))} ・ 比較対象なし ({result.get('rounds', 0)} ラウンド)。"
                   "変化の判定はしていない。\n\n")
    if summary.get("budget_violations"):
        out.append(f"❌ ヒープ確保の予算超過が {summary['budget_violations']} 件ある。\n\n")

    notable = sorted([b for b in benches if is_notable(b)], key=notable_sort_key)
    if notable:
        out.append(md_table(TABLE_COLUMNS, [bench_cells(b) for b in notable]))
    elif result.get("base"):
        out.append("目立った変化は無い。\n")
    out.append(f"\n<details><summary>全 {len(benches)} 件</summary>\n\n")
    out.append(md_table(TABLE_COLUMNS, [bench_cells(b) for b in benches]))
    out.append("\n</details>\n\n")
    return "".join(out)
