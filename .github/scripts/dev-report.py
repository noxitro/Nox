#!/usr/bin/env python3
"""Nox の開発レポート (日次 / 週次) を作り、AI レビューを添えて Discord に投稿する。

以前は別リポジトリ (noxitro/discord-dev-report) で GitHub API と GitHub Models を
使っていたが、次の理由でこちらへ移した。
  - GitHub Models は 2026-07-30 に提供終了した
  - 別リポジトリだと PAT の期限管理と private の Actions 枠が要る。ここなら
    チェックアウト済みの git を読むだけで済み、公開リポジトリなので実行も無料
  - 差分を API 経由で取っていたため 1 ファイル 500 文字で切っており、ほとんど
    見られていなかった。手元の git なら差分を丸ごと渡せる
  - レビューの基準に AGENTS.md (このリポジトリの規約) をそのまま渡せる

日次 (daily):
  前回レポートした master の地点から、今の HEAD までを対象にする。前回の地点は
  ワークフローが actions/cache に保存した状態ファイル (--state) から読む。日付で
  区切らないのは、(1) 前日のコミットを翌日に push すると取りこぼす、(2) 実行が
  失敗した日の分が抜ける、の 2 つを避けるため。状態が無いとき (初回・キャッシュ
  切れ・履歴の書き換え後) は JST の前日 1 日分に戻る。
  対象のコミットが無い日は投稿しない (流れてきた時点で見るものがある、にする)。
週次 (weekly):
  JST で直近 7 日 (実行日の 0 時まで) を集計し、活動統計と週の総括を投稿する。

AI レビュー (設定されている API キーの分だけ行い、並べて投稿する):
  GEMINI_API_KEY     Google Gemini API。無料枠で使える (既定モデル gemini-3.8-flash)。
                     無料枠では送った内容が Google の製品改善に使われる。公開
                     リポジトリのコードなので問題にならない。
  ANTHROPIC_API_KEY  Claude API。有料。キーを登録したときだけ呼ぶ (既定 claude-opus-5)。
                     1 回あたりの概算費用をレポートに出すので、無料の Gemini と
                     並べて費用対効果を比べられる。
  どちらも無ければ、コミット一覧と変更統計だけを投稿する。

使い方 (手元で試す):
  python3 .github/scripts/dev-report.py --mode daily --dry-run
  python3 .github/scripts/dev-report.py --mode daily --dry-run --base <rev> --head <rev>

終了コード: 投稿できて AI レビューも失敗しなければ 0。どれかが失敗したら 1
(日次では状態を進めないので、次回に同じ範囲をもう一度レビューする)。
"""

import argparse
import datetime as dt
import json
import os
import re
import subprocess
import sys
import time
import urllib.error
import urllib.request

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import discord_webhook  # noqa: E402

JST = dt.timezone(dt.timedelta(hours=9), "JST")

# AI に渡す差分の上限 (文字数)。日次の中央値は約 8 万文字、上位 10% で約 32 万文字
# (2026-06〜09 の実測)。40 万文字 ≒ 11 万トークン程度で、Gemini の無料枠でも
# 1 回で送れる量に収める。超えた分は優先度の低いファイルから省き、省いたことを
# レポートに明記する。
MAX_DIFF_CHARS = int(os.environ.get("NOX_REPORT_MAX_DIFF_CHARS") or 400_000)
# 1 ファイルの差分の上限。これを超えるファイルは先頭だけを渡す。
MAX_FILE_DIFF_CHARS = 60_000
# 一覧に並べるコミットの上限。超えた分は件数だけ示す (比較リンクで全部見られる)。
MAX_LISTED_COMMITS = 40

GEMINI_DEFAULT_MODEL = "gemini-3.8-flash"
CLAUDE_DEFAULT_MODEL = "claude-opus-5"
# 100 万トークンあたりの USD (入力, 出力)。概算の表示にだけ使う。
CLAUDE_PRICES = {
    "claude-fable-5-1": (10.0, 50.0),
    "claude-opus-5-5": (4.0, 20.0),
    "claude-opus-5": (5.0, 25.0),
    "claude-opus-4-8": (5.0, 25.0),
    "claude-sonnet-5": (2.0, 10.0),
    "claude-haiku-4-5": (1.0, 5.0),
}
# 拒否されたときに別モデルで再実行させる (server-side fallback) のに対応するモデル
CLAUDE_FALLBACK_MODELS = {"claude-opus-5", "claude-fable-5-1"}

# 差分を渡す優先度。小さいほど先に入れる。どれにも当たらないものは 2。
PRIORITY_RULES = [
    (0, re.compile(r"\.(h|hpp|hh|inl|c|cc|cpp|cxx|ixx|cs|xaml|py|ps1|psm1|sh|hlsl|fx|glsl)$", re.I)),
    (0, re.compile(r"^\.github/")),
    (1, re.compile(r"\.(props|targets|vcxproj|csproj|slnx|sln|json|ya?ml|xml|cmake|gitattributes|gitignore|editorconfig)$", re.I)),
    (3, re.compile(r"\.(filters|lock|svg|css|sty|tex|map)$|(^|/)packages\.lock\.json$|^docs/doxygen/", re.I)),
]


# ---------------------------------------------------------------------------
# git
# ---------------------------------------------------------------------------

def git(*args, check=True):
    r = subprocess.run(["git", *args], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if check and r.returncode != 0:
        raise RuntimeError(f"git {' '.join(args)} が失敗した: {r.stderr.decode('utf-8', 'replace').strip()}")
    return r.stdout.decode("utf-8", "replace")


def rev(name):
    out = git("rev-parse", "--verify", "--quiet", name + "^{commit}", check=False).strip()
    return out or None


def first_parent_before(head, when):
    """head の first-parent を遡り、when より前にコミットされた最初のコミットを返す (無ければ None)。"""
    out = git("rev-list", "-1", "--first-parent", f"--before={when.isoformat()}", head).strip()
    return out or None


def is_ancestor(a, b):
    return subprocess.run(["git", "merge-base", "--is-ancestor", a, b]).returncode == 0


def list_commits(base, head):
    """base..head のコミット (マージを除く) を新しい順に返す。base が None なら head までの全部。"""
    fmt = "%H%x1f%h%x1f%an%x1f%cI%x1f%s%x1e"
    rng = f"{base}..{head}" if base else head
    out = git("log", "--no-merges", f"--format={fmt}", rng)
    commits = []
    for rec in out.split("\x1e"):
        rec = rec.strip("\n")
        if not rec:
            continue
        sha, short, author, date, subject = rec.split("\x1f")
        commits.append({"sha": sha, "short": short, "author": author,
                        "date": dt.datetime.fromisoformat(date), "subject": subject})
    return commits


def count_merges(base, head):
    rng = f"{base}..{head}" if base else head
    return len(git("rev-list", "--merges", rng).split())


def diff_base(base):
    # base が無い (リポジトリの先頭から) ときは空ツリーと比べる
    return base or git("hash-object", "-t", "tree", "/dev/null").strip()


def changed_files(base, head):
    """[(status, path, old_path, added, deleted, binary)] を git の出力順で返す。"""
    b = diff_base(base)
    raw = git("diff", "--raw", "-z", "-M", "--no-ext-diff", b, head).split("\0")
    entries = []
    i = 0
    while i < len(raw):
        meta = raw[i]
        if not meta.startswith(":"):
            i += 1
            continue
        status = meta.split(" ")[-1]
        if status[0] in "RC":
            entries.append([status[0], raw[i + 2], raw[i + 1]])
            i += 3
        else:
            entries.append([status[0], raw[i + 1], raw[i + 1]])
            i += 2
    num = git("diff", "--numstat", "-z", "-M", "--no-ext-diff", b, head).split("\0")
    stats = []
    i = 0
    while i < len(num):
        rec = num[i]
        if not rec:
            i += 1
            continue
        added, deleted, path = rec.split("\t", 2)
        if path == "":
            # 移動: "added\tdeleted\t\0old\0new\0"
            path = num[i + 2]
            i += 3
        else:
            i += 1
        stats.append((added, deleted, path))
    result = []
    for (status, path, old), (added, deleted, _) in zip(entries, stats):
        binary = added == "-"
        result.append({
            "status": status, "path": path, "old_path": old,
            "added": 0 if binary else int(added), "deleted": 0 if binary else int(deleted),
            "binary": binary,
        })
    return result


def file_patches(base, head, files):
    """ファイルごとの差分テキストを返す。git diff の出力を 'diff --git ' で区切り、
    changed_files と同じ順に並んでいる前提で対応させる。CR は落とす (AI には不要)。"""
    b = diff_base(base)
    out = git("diff", "-M", "-U5", "--no-color", "--no-ext-diff", b, head)
    parts = re.split(r"(?m)^(?=diff --git )", out)
    parts = [p for p in parts if p.startswith("diff --git ")]
    if len(parts) != len(files):
        # 対応が崩れたら安全側に倒し、ファイル単位で取り直す
        return [git("diff", "-M", "-U5", "--no-color", "--no-ext-diff", b, head, "--", f["path"], f["old_path"])
                .replace("\r\n", "\n") for f in files]
    return [p.replace("\r\n", "\n") for p in parts]


# ---------------------------------------------------------------------------
# 範囲の決定
# ---------------------------------------------------------------------------

def resolve_daily_range(head, state_path, now):
    """(base, head, 説明) を返す。base が None なら head までの全部。"""
    state = None
    if state_path and os.path.exists(state_path):
        try:
            with open(state_path, encoding="utf-8") as f:
                state = json.load(f)
        except (OSError, ValueError) as e:
            print(f"::warning::状態ファイルを読めなかった ({e})。前日分にする")
    if state and state.get("head"):
        base = rev(state["head"])
        if base and is_ancestor(base, head):
            return base, head, f"前回のレポート ({state['head'][:10]}) 以降"
        print(f"::warning::前回の地点 {state['head'][:10]} が今の履歴に無い (履歴の書き換え?)。前日分にする")

    today = now.astimezone(JST).replace(hour=0, minute=0, second=0, microsecond=0)
    start = today - dt.timedelta(days=1)
    end_head = first_parent_before(head, today)
    if end_head is None:
        return None, None, "前日分"
    return first_parent_before(head, start), end_head, f"{start:%Y-%m-%d} (JST) の 1 日分"


def resolve_weekly_range(head, now):
    end = now.astimezone(JST).replace(hour=0, minute=0, second=0, microsecond=0)
    start = end - dt.timedelta(days=7)
    end_head = first_parent_before(head, end)
    if end_head is None:
        return None, None, start, end
    return first_parent_before(head, start), end_head, start, end


# ---------------------------------------------------------------------------
# AI に渡す差分の組み立て
# ---------------------------------------------------------------------------

def priority(path):
    for prio, pattern in PRIORITY_RULES:
        if pattern.search(path):
            return prio
    return 2


def build_diff_payload(files, patches):
    """上限に収まるよう差分を選ぶ。(本文, 省略の説明の行リスト) を返す。"""
    omitted = []
    candidates = []
    for f, patch in zip(files, patches):
        if f["status"] == "D":
            omitted.append(f"削除: {f['path']} (-{f['deleted']} 行、内容は省略)")
            continue
        if f["binary"]:
            omitted.append(f"バイナリ: {f['path']}")
            continue
        if len(patch) > MAX_FILE_DIFF_CHARS:
            cut = len(patch) - MAX_FILE_DIFF_CHARS
            patch = patch[:MAX_FILE_DIFF_CHARS] + f"\n… (このファイルの差分は残り {cut:,} 文字を省略)\n"
            omitted.append(f"一部のみ: {f['path']} (差分が大きいので先頭 {MAX_FILE_DIFF_CHARS:,} 文字だけ)")
        candidates.append((priority(f["path"]), len(patch), f["path"], patch))

    # 優先度の高い順、同じ優先度なら小さい順に詰める (大きい 1 本より小さい多数を優先)
    chosen = set()
    total = 0
    for prio, size, path, patch in sorted(candidates, key=lambda c: (c[0], c[1])):
        if total + size > MAX_DIFF_CHARS:
            omitted.append(f"上限超過で省略: {path} ({size:,} 文字)")
            continue
        chosen.add(path)
        total += size
    body = "".join(patch for _, _, path, patch in candidates if path in chosen)
    return body, omitted


def load_rules():
    """レビューの基準にする AGENTS.md。規約の正典はこのファイルなので、複製せずに実行時に読む。"""
    root = git("rev-parse", "--show-toplevel").strip()
    try:
        with open(os.path.join(root, "AGENTS.md"), encoding="utf-8-sig") as f:
            return f.read()
    except OSError:
        return "(AGENTS.md を読めなかった)"


SYSTEM_PROMPT = """あなたは Nox ゲームエンジン (Windows 向け C++ ランタイム + C#/WPF エディタ) の
シニアレビュアーです。以下はこのリポジトリの規約 (AGENTS.md) で、レビューの判断基準です。
特に「最優先ルール」(CPU/メモリ性能、ヒープ確保の撤廃、Runtime で例外を使わない) と
コーディング規約・ファイル形式・WPF テーマ規約への違反は必ず指摘してください。

<agents_md>
{rules}
</agents_md>
"""

DAILY_INSTRUCTIONS = """上のコミットの差分をレビューしてください。

- 差分に実際に現れているコードだけを根拠にしてください。見えていない部分を推測で批判しないこと。
  差分の一部を省略した場合は「省略したもの」に書いてあります。
- 本物の不具合 (クラッシュ、未定義動作、リーク、競合、ロジックの誤り) と規約違反を最優先にし、
  好みの問題や細かすぎる指摘は省いてください。指摘が無ければ無理に作らないこと。
- 重大度: 🔴 不具合・規約違反 / 🟡 設計・性能・保守性の懸念 / ⚪ 軽微
- 出力は日本語で、Discord に貼るので次の形式にしてください (見出し記号 # は使わない)。全体で 3,000 文字以内。

**総評**
2〜3 文。

**指摘** (重大な順、最大 10 件。無ければ「特になし」)
- 🔴 `ファイル名:行付近` — 何が問題か → どう直すか

**良い点**
- 1〜3 項目

**評価**: NN/100 (一言で理由)
"""

WEEKLY_INSTRUCTIONS = """上は 1 週間分のコミットと変更統計です (差分の中身は含みません)。
開発の進み方を総括してください。

- コミットメッセージと変更統計から読み取れることだけを根拠にしてください。
- 出力は日本語で、Discord に貼るので次の形式にしてください (見出し記号 # は使わない)。全体で 2,500 文字以内。

**今週の成果**
- 主な変更を 3〜6 項目

**気になる点**
- 技術的負債・リスク・規約上の懸念 (コミットの粒度やメッセージの質を含む)。無ければ「特になし」

**来週の注目点**
- 1〜3 項目

**評価**: NN/100 (一言で理由)
"""


# ---------------------------------------------------------------------------
# AI レビュー
# ---------------------------------------------------------------------------

class Review:
    def __init__(self, provider, model):
        self.provider = provider
        self.model = model
        self.text = ""
        self.error = None
        self.notes = []
        self.input_tokens = None
        self.output_tokens = None
        self.cost_usd = None

    def footer(self):
        parts = [self.model]
        if self.input_tokens is not None:
            parts.append(f"入力 {self.input_tokens:,} / 出力 {self.output_tokens:,} トークン")
        if self.cost_usd is not None:
            parts.append(f"概算 ${self.cost_usd:.3f}")
        elif self.provider == "Gemini":
            parts.append("無料枠")
        return " · ".join(parts)


def http_json(url, body, headers, timeout=600):
    req = urllib.request.Request(url, data=json.dumps(body).encode("utf-8"), method="POST",
                                 headers={"Content-Type": "application/json", **headers})
    with urllib.request.urlopen(req, timeout=timeout) as resp:
        return json.loads(resp.read().decode("utf-8"))


def gemini_retry_delay(detail, attempt):
    """429 / 503 の応答にある RetryInfo.retryDelay ("37s" など) を読む。無ければ指数バックオフ。"""
    try:
        for d in json.loads(detail).get("error", {}).get("details", []):
            m = re.fullmatch(r"(\d+(?:\.\d+)?)s", str(d.get("retryDelay", "")))
            if m:
                return min(float(m.group(1)) + 1, 120)
    except (ValueError, AttributeError):
        pass
    return min(15 * (2 ** attempt), 120)


def review_with_gemini(system, user):
    model = os.environ.get("NOX_REVIEW_GEMINI_MODEL") or GEMINI_DEFAULT_MODEL
    r = Review("Gemini", model)
    base = (os.environ.get("GEMINI_API_BASE") or "https://generativelanguage.googleapis.com").rstrip("/")
    url = f"{base}/v1beta/models/{model}:generateContent"
    body = {
        "systemInstruction": {"parts": [{"text": system}]},
        "contents": [{"role": "user", "parts": [{"text": user}]}],
        # 思考に使うトークンも出力の上限に含まれるので、本文 3,000 文字に対して十分に取る
        "generationConfig": {"maxOutputTokens": 32768},
    }
    headers = {"x-goog-api-key": os.environ["GEMINI_API_KEY"]}
    data = None
    attempts = 4
    for attempt in range(attempts):
        try:
            data = http_json(url, body, headers)
            break
        except urllib.error.HTTPError as e:
            detail = e.read().decode("utf-8", "replace")
            if e.code in (429, 500, 503, 504) and attempt + 1 < attempts:
                delay = gemini_retry_delay(detail, attempt)
                print(f"Gemini: HTTP {e.code}。{delay:.0f} 秒待って再試行する")
                time.sleep(delay)
                continue
            r.error = f"HTTP {e.code}: {detail[:400]}"
            return r
        except (urllib.error.URLError, TimeoutError) as e:
            if attempt + 1 < attempts:
                time.sleep(15 * (2 ** attempt))
                continue
            r.error = f"接続に失敗した: {getattr(e, 'reason', e)}"
            return r

    usage = data.get("usageMetadata", {})
    r.input_tokens = usage.get("promptTokenCount")
    r.output_tokens = (usage.get("candidatesTokenCount") or 0) + (usage.get("thoughtsTokenCount") or 0)
    if r.input_tokens is None:
        r.input_tokens = r.output_tokens = None

    block = data.get("promptFeedback", {}).get("blockReason")
    candidates = data.get("candidates") or []
    if block or not candidates:
        r.error = f"応答が返らなかった (blockReason={block})"
        return r
    cand = candidates[0]
    parts = cand.get("content", {}).get("parts", [])
    r.text = "".join(p.get("text", "") for p in parts if not p.get("thought")).strip()
    reason = cand.get("finishReason")
    if reason == "MAX_TOKENS":
        r.notes.append("出力が上限で途中まで")
    elif reason not in (None, "STOP"):
        r.notes.append(f"finishReason={reason}")
    if not r.text:
        r.error = f"本文が空だった (finishReason={reason})"
    return r


def review_with_claude(system, user):
    model = os.environ.get("NOX_REVIEW_CLAUDE_MODEL") or CLAUDE_DEFAULT_MODEL
    r = Review("Claude", model)
    try:
        import anthropic
    except ImportError:
        r.error = "anthropic パッケージが入っていない (pip install 'anthropic>=1,<2')"
        return r

    params = {
        "model": model,
        "max_tokens": 16000,
        "system": system,
        "messages": [{"role": "user", "content": user}],
    }
    if not model.startswith("claude-haiku"):
        # Haiku 4.5 は adaptive thinking と effort を受け付けない
        params["thinking"] = {"type": "adaptive"}
        params["output_config"] = {"effort": os.environ.get("NOX_REVIEW_CLAUDE_EFFORT") or "high"}
    if model in CLAUDE_FALLBACK_MODELS:
        # 安全分類器に拒否されたら、推奨の別モデルでサーバ側が再実行する
        params["betas"] = ["server-side-fallback-2026-07-01"]
        params["extra_body"] = {"fallbacks": "default"}

    client = anthropic.Anthropic(max_retries=4)
    try:
        if "betas" in params:
            resp = client.beta.messages.create(**params)
        else:
            resp = client.messages.create(**params)
    except anthropic.APIStatusError as e:
        r.error = f"HTTP {e.status_code}: {str(e.message)[:400]}"
        return r
    except anthropic.APIConnectionError as e:
        r.error = f"接続に失敗した: {e}"
        return r

    r.input_tokens = resp.usage.input_tokens
    r.output_tokens = resp.usage.output_tokens
    price = CLAUDE_PRICES.get(model)
    if price:
        r.cost_usd = (r.input_tokens * price[0] + r.output_tokens * price[1]) / 1_000_000
    if resp.model != model:
        r.notes.append(f"{resp.model} が応答 (fallback)")
    if resp.stop_reason == "refusal":
        r.error = "モデルがレビューを拒否した (stop_reason=refusal)"
        return r
    if resp.stop_reason == "max_tokens":
        r.notes.append("出力が上限で途中まで")
    r.text = "".join(b.text for b in resp.content if b.type == "text").strip()
    if not r.text:
        r.error = f"本文が空だった (stop_reason={resp.stop_reason})"
    return r


def run_reviews(system, user):
    reviews = []
    if os.environ.get("GEMINI_API_KEY"):
        reviews.append(review_with_gemini(system, user))
    if os.environ.get("ANTHROPIC_API_KEY"):
        reviews.append(review_with_claude(system, user))
    for r in reviews:
        status = f"失敗: {r.error}" if r.error else f"{len(r.text)} 文字"
        print(f"{r.provider} ({r.model}): {status} / {r.footer()}")
    return reviews


def review_embeds(reviews):
    embeds = []
    for r in reviews:
        title = f"🤖 AI レビュー — {r.provider}"
        footer = r.footer() + ("" if not r.notes else " · " + " · ".join(r.notes))
        if r.error:
            embeds += discord_webhook.text_embeds(f"⚠️ AI レビュー失敗 — {r.provider}", r.error,
                                                  discord_webhook.COLORS["red"], footer=footer)
        else:
            embeds += discord_webhook.text_embeds(title, r.text, discord_webhook.COLORS["blue"], footer=footer)
    return embeds


# ---------------------------------------------------------------------------
# レポート
# ---------------------------------------------------------------------------

def repo_url():
    server = os.environ.get("GITHUB_SERVER_URL", "https://github.com")
    return f"{server}/{os.environ.get('GITHUB_REPOSITORY', 'noxitro/Nox')}"


def commit_lines(commits):
    url = repo_url()
    lines = [f"[`{c['short']}`]({url}/commit/{c['sha']}) {discord_escape(c['subject'][:90])} — {discord_escape(c['author'])}"
             for c in commits[:MAX_LISTED_COMMITS]]
    if len(commits) > MAX_LISTED_COMMITS:
        lines.append(f"…ほか {len(commits) - MAX_LISTED_COMMITS} 件")
    return lines


def discord_escape(s):
    return re.sub(r"([\\`*_~|>\[\]])", r"\\\1", s)


def stat_line(files):
    added = sum(f["added"] for f in files)
    deleted = sum(f["deleted"] for f in files)
    return f"{len(files)} ファイル / +{added:,} −{deleted:,} 行"


def area_stats(files, limit=8):
    """変更行数をトップレベルの 2 階層 (runtime/core など) ごとに集計する。"""
    areas = {}
    for f in files:
        parts = f["path"].split("/")
        key = "/".join(parts[:2]) if len(parts) > 2 else parts[0]
        a = areas.setdefault(key, [0, 0, 0])
        a[0] += 1
        a[1] += f["added"]
        a[2] += f["deleted"]
    top = sorted(areas.items(), key=lambda kv: -(kv[1][1] + kv[1][2]))[:limit]
    return [f"`{k}` {v[0]} ファイル +{v[1]:,} −{v[2]:,}" for k, v in top]


def date_span(commits):
    dates = sorted({c["date"].astimezone(JST).strftime("%Y-%m-%d") for c in commits})
    return dates[0] if len(dates) == 1 else f"{dates[0]} 〜 {dates[-1]}"


def daily(args, now):
    head = rev(args.head)
    if args.base:
        base, desc = rev(args.base), f"指定範囲 {args.base}..{args.head}"
    else:
        base, head, desc = resolve_daily_range(head, args.state, now)
    if head is None or base == head:
        print(f"対象: {desc}。新しいコミットが無いので投稿しない")
        return 0, head
    commits = list_commits(base, head)
    merges = count_merges(base, head)
    if not commits:
        print(f"対象: {desc}。マージ以外のコミットが無いので投稿しない")
        return 0, head

    files = changed_files(base, head)
    patches = file_patches(base, head, files)
    diff_text, omitted = build_diff_payload(files, patches)
    print(f"対象: {desc} / {(base or '(root)')[:10]}..{head[:10]} / コミット {len(commits)} 件 / "
          f"{stat_line(files)} / AI に渡す差分 {len(diff_text):,} 文字 / 省略 {len(omitted)} 件")

    user = "\n".join([
        f"対象: master の {(base or '(root)')[:10]}..{head[:10]} ({date_span(commits)}, コミット {len(commits)} 件"
        + (f", マージ {merges} 件" if merges else "") + ")",
        "",
        "## コミット一覧 (新しい順)",
        *[f"- {c['short']} {c['subject']} ({c['author']})" for c in commits],
        "",
        "## 変更ファイル",
        *[f"- {f['status']} {f['path']}" + (f" (← {f['old_path']})" if f["old_path"] != f["path"] else "")
          + (" [binary]" if f["binary"] else f" +{f['added']} -{f['deleted']}") for f in files],
        "",
        "## 省略したもの",
        *([f"- {o}" for o in omitted] or ["- なし"]),
        "",
        "## 差分",
        "<diff>",
        diff_text,
        "</diff>",
        "",
        DAILY_INSTRUCTIONS,
    ])
    reviews = run_reviews(SYSTEM_PROMPT.format(rules=load_rules()), user)

    compare = f"{repo_url()}/compare/{base}...{head}" if base else f"{repo_url()}/commits/{head}"
    body = [f"**{stat_line(files)}**" + (f" · マージ {merges} 件" if merges else ""), "", *commit_lines(commits)]
    if omitted:
        body += ["", f"AI に渡していない差分: {len(omitted)} 件 (削除・バイナリ・上限超過など)"]
    if not reviews:
        body += ["", "AI レビューは未設定 (GEMINI_API_KEY / ANTHROPIC_API_KEY)"]
    embeds = discord_webhook.text_embeds(f"📅 デイリー開発レポート — {date_span(commits)}", "\n".join(body),
                                         discord_webhook.COLORS["green"], url=compare,
                                         footer=f"{os.environ.get('GITHUB_REPOSITORY', 'noxitro/Nox')} · {desc}")
    embeds += review_embeds(reviews)
    return publish(args, embeds, reviews), head


def weekly(args, now):
    head = rev(args.head)
    base, head, start, end = resolve_weekly_range(head, now)
    period = f"{start:%Y-%m-%d} 〜 {(end - dt.timedelta(days=1)):%Y-%m-%d}"
    commits = list_commits(base, head) if head and base != head else []
    files = changed_files(base, head) if commits else []
    merges = count_merges(base, head) if commits else 0

    per_day = {}
    authors = {}
    for c in commits:
        day = c["date"].astimezone(JST).date()
        per_day[day] = per_day.get(day, 0) + 1
        authors[c["author"]] = authors.get(c["author"], 0) + 1

    body = [f"**コミット {len(commits)} 件**" + (f" · マージ {merges} 件" if merges else "")
            + (f" · {stat_line(files)}" if files else "")]
    if commits:
        body += ["", "**日別**", *[f"{d:%m/%d} ({'月火水木金土日'[d.weekday()]}): {n} 件" for d, n in sorted(per_day.items())]]
        body += ["", "**作者**", *[f"{discord_escape(a)}: {n} 件" for a, n in sorted(authors.items(), key=lambda kv: -kv[1])]]
        body += ["", "**変更の多い場所**", *area_stats(files)]
    else:
        body += ["", "今週のコミットはありませんでした。"]

    reviews = []
    if commits:
        user = "\n".join([
            f"期間: {period} (JST), コミット {len(commits)} 件" + (f", マージ {merges} 件" if merges else ""),
            "",
            "## コミット一覧 (新しい順)",
            *[f"- {c['date'].astimezone(JST):%m/%d} {c['short']} {c['subject']} ({c['author']})" for c in commits],
            "",
            "## 変更の多い場所",
            *area_stats(files, limit=20),
            "",
            f"## 合計: {stat_line(files)}",
            "",
            WEEKLY_INSTRUCTIONS,
        ])
        reviews = run_reviews(SYSTEM_PROMPT.format(rules=load_rules()), user)

    url = f"{repo_url()}/compare/{base}...{head}" if base and head and base != head else f"{repo_url()}/commits"
    embeds = discord_webhook.text_embeds(f"📊 ウィークリー開発レポート — {period}", "\n".join(body),
                                         discord_webhook.COLORS["blue"], url=url,
                                         footer=os.environ.get("GITHUB_REPOSITORY", "noxitro/Nox"))
    embeds += review_embeds(reviews)
    print(f"対象: {period} / コミット {len(commits)} 件")
    return publish(args, embeds, reviews)


def publish(args, embeds, reviews):
    """投稿して終了コードを返す。dry-run なら表示だけ。"""
    webhook = os.environ.get("DISCORD_WEBHOOK_URL", "")
    if args.dry_run:
        discord_webhook.send(webhook, embeds, username="Nox Dev Report", dry_run=True)
        write_summary(embeds)
    elif not webhook:
        print("::error::DISCORD_WEBHOOK_URL が未設定なので投稿できない")
        return 1
    else:
        count = discord_webhook.send(webhook, embeds, username="Nox Dev Report")
        print(f"Discord に {count} 件のメッセージを送った")
    failed = [r for r in reviews if r.error]
    for r in failed:
        print(f"::error::{r.provider} のレビューに失敗した: {r.error}")
    return 1 if failed else 0


def write_summary(embeds):
    """dry-run の結果をジョブのサマリーに出す (Actions の画面で確認できるように)。"""
    path = os.environ.get("GITHUB_STEP_SUMMARY")
    if not path:
        return
    with open(path, "a", encoding="utf-8") as f:
        f.write("## Dev report (dry-run)\n\n")
        for e in embeds:
            f.write(f"### {e['title']}\n\n{e.get('description', '')}\n\n")
            if "footer" in e:
                f.write(f"_{e['footer']['text']}_\n\n")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--mode", choices=["daily", "weekly"], default="daily")
    ap.add_argument("--head", default="HEAD", help="レポートの終点 (既定: HEAD)")
    ap.add_argument("--base", help="日次の起点を直接指定する (状態ファイルより優先)")
    ap.add_argument("--state", help="日次: 前回の地点を読む/書く JSON ファイル")
    ap.add_argument("--now", help="現在時刻の上書き (ISO 8601、試験用)")
    ap.add_argument("--dry-run", action="store_true", help="投稿せず、内容を表示するだけ")
    args = ap.parse_args()

    for stream in (sys.stdout, sys.stderr):
        if hasattr(stream, "reconfigure"):
            stream.reconfigure(encoding="utf-8", errors="replace")

    now = dt.datetime.fromisoformat(args.now) if args.now else dt.datetime.now(dt.timezone.utc)
    if now.tzinfo is None:
        now = now.replace(tzinfo=JST)

    if args.mode == "weekly":
        return weekly(args, now)

    code, reported_head = daily(args, now)
    # 投稿とレビューが成功したときだけ地点を進める。失敗したら次回に同じ範囲をやり直す。
    if code == 0 and args.state and not args.dry_run and not args.base and reported_head:
        os.makedirs(os.path.dirname(os.path.abspath(args.state)), exist_ok=True)
        with open(args.state, "w", encoding="utf-8") as f:
            json.dump({"head": reported_head, "reported_at": now.isoformat()}, f)
        out = os.environ.get("GITHUB_OUTPUT")
        if out:
            with open(out, "a", encoding="utf-8") as f:
                f.write("state_updated=true\n")
        print(f"状態を更新した: {reported_head[:10]}")
    return code


if __name__ == "__main__":
    sys.exit(main())
