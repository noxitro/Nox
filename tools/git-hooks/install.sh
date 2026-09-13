#!/bin/sh
# フックを有効にする。リポジトリごとに 1 回だけ実行すればよい。
# core.hooksPath を使うので、フック自体がリポジトリで共有される。
set -eu
cd "$(git rev-parse --show-toplevel)"
chmod +x tools/git-hooks/pre-commit tools/git-hooks/pre-push tools/git-hooks/scan.sh 2>/dev/null || true
git config core.hooksPath tools/git-hooks
echo "core.hooksPath = tools/git-hooks を設定した。"
echo "無効化するには: git config --unset core.hooksPath"
