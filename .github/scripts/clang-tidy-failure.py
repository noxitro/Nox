#!/usr/bin/env python3
"""clang-tidy のワークフローが失敗したとき、原因の手掛かりを付けて Discord に知らせる。

.github/workflows/clang-tidy.yml の Notify failure から呼ばれる。通知には次を載せる。
  - どの段階で落ちたか (ビルド / 集計 / それ以外)
  - ブランチとコミット
  - ビルドで落ちたときは、MSBuild のログから拾ったエラー行 (重複を除いて先頭の数件)

Webhook の URL は環境変数 DISCORD_WEBHOOK_URL から読む (未設定なら投稿しない。
discord_webhook.py と同じ方針)。

使い方:
    python3 .github/scripts/clang-tidy-failure.py --stage build --log build.log --root . \\
        --run-url "$RUN_URL" --dry-run
"""

import argparse
import os
import posixpath
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import discord_webhook  # noqa: E402

STAGES = {
    "build": "ビルドに失敗した",
    "report": "集計に失敗した",
    "other": "失敗した",
}

# MSBuild のエラー行。先頭に "12>" (並列ビルドのプロジェクト番号)、末尾に
# " [D:\...\kernel.vcxproj]" (どのプロジェクトのビルド中か) が付くことがある。
#   io\file_stream_writer.cpp(9,10): fatal error : '../os/windows.h' file not found [D:\...\kernel.vcxproj]
#   LINK : fatal error LNK1104: cannot open file 'foo.lib' [D:\...\runtime.vcxproj]
#   C:\...\Microsoft.CppCommon.targets(254,5): error MSB3073: The command ... exited with code 1. [D:\...\core.vcxproj]
#   error MSB1009: Project file does not exist.
ERROR_RE = re.compile(
    r"^\s*(?:\d+>)?\s*(?:(?P<loc>(?:[A-Za-z]:)?[^:\n]*?)\s*:\s*)?"
    r"(?P<level>(?:fatal\s+)?error)\s*(?P<code>[A-Za-z]+\d+)?\s*:\s*(?P<msg>.*?)"
    r"(?:\s*\[(?P<project>[^\]]*\.[A-Za-z]+proj)\])?\s*$"
)
# ファイルの位置。"foo.cpp(12,5)" / "foo.cpp(12)" の行・桁の部分
POS_RE = re.compile(r"^(?P<path>.*?)(?P<pos>\(\d+(?:,\d+)*\))?$")
# エラー行の上限。Discord で読み切れる量にとどめる (全件はアーティファクトのログ)
DEFAULT_MAX = 10
LINE_CHARS = 300


def to_slash(path):
    return path.strip().replace("\\", "/")


def relative_to_root(path, root):
    """root 以下なら root からの相対パスを、外なら None を返す (Windows の大小の揺れは無視する)。"""
    root = to_slash(os.path.abspath(root)).rstrip("/") + "/"
    if path.lower().startswith(root.lower()):
        return path[len(root):]
    return None


def format_error(m, root):
    """エラー行を「場所: level code: 文面」の 1 行にする。
    MSBuild はソースの位置をプロジェクトのディレクトリからの相対で書くので、プロジェクトの
    場所で補ってリポジトリのルートからの相対に直す。ルートの外 (MSBuild の .targets など) は
    ファイル名だけにして、どのプロジェクトのビルド中かを添える。"""
    loc = (m.group("loc") or "").strip()
    project = to_slash(m.group("project") or "")
    suffix = ""
    if loc:
        pm = POS_RE.match(loc)
        path, pos = to_slash(pm.group("path")), pm.group("pos") or ""
        # "LINK" / "CSC" のようなツール名は位置ではないので、そのまま出してプロジェクトを添える
        is_path = bool(pos) or "/" in path or "." in posixpath.basename(path)
        if not is_path and project:
            suffix = f" ({posixpath.basename(project)})"
        elif is_path:
            if not posixpath.isabs(path) and not re.match(r"^[A-Za-z]:/", path) and project:
                path = posixpath.join(posixpath.dirname(project), path)
            path = posixpath.normpath(path)
            rel = relative_to_root(path, root)
            if rel is None:
                path = posixpath.basename(path)
                if project:
                    suffix = f" ({posixpath.basename(project)})"
            else:
                path = rel
        loc = path + pos
    elif project:
        suffix = f" ({posixpath.basename(project)})"

    head = re.sub(r"\s+", " ", m.group("level"))
    if m.group("code"):
        head += f" {m.group('code')}"
    text = f"{loc}: {head}: {m.group('msg').strip()}" if loc else f"{head}: {m.group('msg').strip()}"
    return discord_webhook.clip(text + suffix, LINE_CHARS)


def collect_errors(log_path, root):
    """ログのエラー行を出てきた順に重複なく返す。MSBuild は最後にもう一度エラーを並べ直すので、
    同じ行が 2 回ずつ出る。ログが無ければ空 (ビルドより前の手順で落ちたとき)。"""
    if not log_path or not os.path.exists(log_path):
        return []
    seen = {}
    with open(log_path, encoding="utf-8", errors="replace") as f:
        for raw in f:
            m = ERROR_RE.match(raw.rstrip("\r\n"))
            if m:
                seen.setdefault(format_error(m, root), None)
    return list(seen)


def commit_line():
    """コミットの短い SHA (リンク付き) と件名。schedule でも checkout 済みの HEAD から引ける。"""
    sha = os.environ.get("GITHUB_SHA", "")
    try:
        subject = subprocess.run(
            ["git", "log", "-1", "--format=%s", sha or "HEAD"],
            capture_output=True, text=True, encoding="utf-8", check=True,
        ).stdout.strip()
    except (OSError, subprocess.CalledProcessError):
        subject = ""
    if not sha:
        return subject or None
    server = os.environ.get("GITHUB_SERVER_URL", "https://github.com")
    repo = os.environ.get("GITHUB_REPOSITORY", "")
    return f"[`{sha[:7]}`]({server}/{repo}/commit/{sha}) {subject}".rstrip()


def build_description(stage, errors, max_errors, run_url):
    lines = []
    ref = os.environ.get("GITHUB_REF_NAME")
    if ref:
        lines.append(f"ブランチ: `{ref}`")
    commit = commit_line()
    if commit:
        lines.append(f"コミット: {commit}")
    lines.append("")

    if stage == "build":
        if errors:
            shown = errors[:max_errors]
            lines.append(f"**エラー ({len(errors)} 件{'、先頭 ' + str(len(shown)) + ' 件' if len(errors) > len(shown) else ''})**")
            lines.append("```")
            # 文面に ``` があるとコードブロックが閉じてしまうので崩す
            lines += [e.replace("```", "'''") for e in shown]
            lines.append("```")
        else:
            lines.append("ログからエラー行を拾えなかった。Build with clang-tidy のログを確認すること。")
    elif stage == "report":
        lines.append("ビルドは通ったが、集計 (clang-tidy-report.py) が失敗した。Report のログを確認すること。")
    else:
        lines.append("ビルドと集計以外の手順で失敗した。どの手順が落ちたかはログで確認すること。")

    if run_url:
        lines += ["", f"詳細: {run_url}"]
    return "\n".join(lines)


def main():
    # Windows のランナーは標準出力が cp1252 なので、日本語で落ちないようにする
    for stream in (sys.stdout, sys.stderr):
        if hasattr(stream, "reconfigure"):
            stream.reconfigure(encoding="utf-8", errors="replace")
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--stage", choices=sorted(STAGES), required=True, help="どの段階で落ちたか")
    ap.add_argument("--log", help="MSBuild のログ (無くてもよい)")
    ap.add_argument("--root", default=".", help="リポジトリのルート (パスを相対にするため)")
    ap.add_argument("--max-errors", type=int, default=DEFAULT_MAX, help="載せるエラー行の上限")
    ap.add_argument("--run-url", default="", help="この実行の URL")
    ap.add_argument("--dry-run", action="store_true", help="投稿せずに payload を表示する")
    args = ap.parse_args()

    errors = collect_errors(args.log, args.root) if args.stage == "build" else []
    repo = os.environ.get("GITHUB_REPOSITORY", "")
    title = f":x: clang-tidy: {STAGES[args.stage]}" + (f" — {repo}" if repo else "")
    description = build_description(args.stage, errors, args.max_errors, args.run_url)
    footer = None
    if os.environ.get("GITHUB_WORKFLOW"):
        footer = f"{os.environ['GITHUB_WORKFLOW']} #{os.environ.get('GITHUB_RUN_NUMBER', '')}"

    webhook = os.environ.get("DISCORD_WEBHOOK_URL", "").strip()
    if not webhook and not args.dry_run:
        print("::warning::DISCORD_WEBHOOK_URL が未設定なので通知をスキップする")
        return 0
    embeds = discord_webhook.text_embeds(title, description, discord_webhook.COLORS["red"],
                                         url=args.run_url or None, footer=footer)
    count = discord_webhook.send(webhook, embeds, username="GitHub Actions", dry_run=args.dry_run)
    print(f"Discord に {count} 件のメッセージを送った" + (" (dry-run)" if args.dry_run else ""))
    return 0


if __name__ == "__main__":
    sys.exit(main())
