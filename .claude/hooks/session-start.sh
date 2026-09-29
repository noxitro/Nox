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
#   2. gitleaks を固定バージョンで入れる (secret-scan.yml と同じ版・同じ検証手順)。
#      失敗してもセッションは止めない。gitleaks.sh は無ければ警告だけ出して
#      組み込みパターンで検査を続ける。
#
# 手元 (Windows) では走らせない。フックの導入は README の手順で 1 回だけ行う。
set -uo pipefail

if [ "${CLAUDE_CODE_REMOTE:-}" != "true" ]; then
  exit 0
fi

cd "${CLAUDE_PROJECT_DIR:-$(git rev-parse --show-toplevel)}" || exit 0

# --- 1. git フック ----------------------------------------------------------
sh tools/git-hooks/install.sh || echo "session-start: install.sh が失敗した。フックは無効のまま" >&2

# --- 2. gitleaks ------------------------------------------------------------
GITLEAKS_VERSION=8.30.1

if command -v gitleaks >/dev/null 2>&1 && gitleaks version 2>/dev/null | grep -q "^v\{0,1\}${GITLEAKS_VERSION}\$"; then
  echo "session-start: gitleaks ${GITLEAKS_VERSION} は導入済み"
  exit 0
fi

case "$(uname -s)-$(uname -m)" in
  Linux-x86_64) archive="gitleaks_${GITLEAKS_VERSION}_linux_x64.tar.gz" ;;
  Linux-aarch64) archive="gitleaks_${GITLEAKS_VERSION}_linux_arm64.tar.gz" ;;
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

base="https://github.com/gitleaks/gitleaks/releases/download/v${GITLEAKS_VERSION}"
tmp=$(mktemp -d) || exit 0
trap 'rm -rf "$tmp"' EXIT
(
  set -e
  cd "$tmp"
  curl -fsSL --retry 3 "$base/$archive" -o "$archive"
  # リリースに付属する checksums.txt と突き合わせる (secret-scan.yml と同じ)
  curl -fsSL --retry 3 "$base/gitleaks_${GITLEAKS_VERSION}_checksums.txt" -o sums.txt
  grep " $archive\$" sums.txt | sha256sum -c - >/dev/null
  tar -xzf "$archive" gitleaks
  install -m 0755 gitleaks "$dest/gitleaks"
) || {
  echo "session-start: gitleaks の取得に失敗した。組み込みパターンのみで検査する" >&2
  exit 0
}
echo "session-start: gitleaks $("$dest/gitleaks" version 2>/dev/null) を $dest に入れた"
