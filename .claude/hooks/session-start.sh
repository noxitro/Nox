#!/bin/bash
# Claude Code on the web のセッション開始時に走る (SessionStart フック)。
#
# 目的: クラウドのセッションからの push にも tools/git-hooks を効かせる。
#   clone 直後のチェックアウトは core.hooksPath が未設定で gitleaks も無いので、
#   エージェントの push は push 後の CI (Secret scan) にしか掛からなかった。
#   GitHub の push protection は提携している発行元のトークン形式しか見ないため、
#   個人メールやローカル絶対パスの流入は pre-push で止めるしかない。
#
# やること:
#   1. tools/git-hooks/install.sh で core.hooksPath を設定する。
#   2. gitleaks を固定バージョンで入れる。失敗してもセッションは止めない。
#      gitleaks.sh は無ければ警告だけ出して組み込みパターンで検査を続ける。
#
# 手元 (Windows) では走らせない。フックの導入は README の手順で 1 回だけ行う。
# settings.json 側でも同じ判定をしているが、直接呼ばれたときのためにここでも見る。
set -uo pipefail

if [ "${CLAUDE_CODE_REMOTE:-}" != "true" ]; then
  exit 0
fi

cd "${CLAUDE_PROJECT_DIR:-$(git rev-parse --show-toplevel)}" || exit 0

# --- 1. git フック ----------------------------------------------------------
sh tools/git-hooks/install.sh || echo "session-start: install.sh が失敗した。フックは無効のまま" >&2

# --- 2. gitleaks ------------------------------------------------------------
# 版は .github/workflows/secret-scan.yml の GITLEAKS_VERSION と揃える。
# SHA256 はリリースの checksums.txt から写したもので、リリース側の差し替えも検出できるよう
# 取得時に checksums.txt を読まずここに固定してある。版と SHA256 が secret-scan.yml の版の
# checksums.txt と一致しているかは、Secret scan が毎回検査する (食い違うと CI が落ちる)。
GITLEAKS_VERSION=8.30.1
GITLEAKS_SHA256_LINUX_X64=551f6fc83ea457d62a0d98237cbad105af8d557003051f41f3e7ca7b3f2470eb
GITLEAKS_SHA256_LINUX_ARM64=e4a487ee7ccd7d3a7f7ec08657610aa3606637dab924210b3aee62570fb4b080

if command -v gitleaks >/dev/null 2>&1; then
  have=$(gitleaks version 2>/dev/null)
  if [ "${have#v}" = "$GITLEAKS_VERSION" ]; then
    echo "session-start: gitleaks ${GITLEAKS_VERSION} は導入済み"
    exit 0
  fi
fi

case "$(uname -s)-$(uname -m)" in
  Linux-x86_64)
    archive="gitleaks_${GITLEAKS_VERSION}_linux_x64.tar.gz"
    sha256=$GITLEAKS_SHA256_LINUX_X64 ;;
  Linux-aarch64)
    archive="gitleaks_${GITLEAKS_VERSION}_linux_arm64.tar.gz"
    sha256=$GITLEAKS_SHA256_LINUX_ARM64 ;;
  *)
    echo "session-start: この環境 ($(uname -s)-$(uname -m)) 向けの gitleaks 取得は未対応。組み込みパターンのみで検査する" >&2
    exit 0 ;;
esac

if [ -w /usr/local/bin ]; then
  dest=/usr/local/bin
else
  dest="${HOME}/.local/bin"   # gitleaks.sh の探索先に入れてある
  mkdir -p "$dest"
fi

tmp=$(mktemp -d) || exit 0
trap 'rm -rf "$tmp"' EXIT
(
  set -e
  cd "$tmp"
  # フックは同期で走るので、応答が止まったときにセッションの開始を待たせ続けない。
  curl -fsSL --retry 3 --connect-timeout 10 --max-time 120 \
    "https://github.com/gitleaks/gitleaks/releases/download/v${GITLEAKS_VERSION}/${archive}" -o "$archive"
  printf '%s  %s\n' "$sha256" "$archive" | sha256sum -c - >/dev/null
  tar -xzf "$archive" gitleaks
  install -m 0755 gitleaks "$dest/gitleaks"
) || {
  echo "session-start: gitleaks の取得か検証に失敗した。組み込みパターンのみで検査する" >&2
  exit 0
}
echo "session-start: gitleaks $("$dest/gitleaks" version 2>/dev/null) を $dest に入れた"
