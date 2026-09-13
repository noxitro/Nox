#!/bin/sh
# 公開リポジトリに出してはいけないものを検出する共通スキャナ。
# pre-commit と pre-push の両方から呼ばれる。
#
# 標準入力から "<path>\t<blob-ish>" を 1 行ずつ受け取る。blob-ish は
# git cat-file にそのまま渡せる形 (pre-commit なら ":path"、pre-push なら
# "<sha>:path")。検出したら理由を表示して終了コード 1 を返す。
#
# 誤検出を握り潰したいときは .githooks-allow に 1 行 1 パスで書く。
set -u

ROOT=$(git rev-parse --show-toplevel)
ALLOW="$ROOT/.githooks-allow"
FOUND=0
MAX_BYTES=$((5 * 1024 * 1024))

note() { FOUND=1; printf '  [%s] %s\n    %s\n' "$1" "$2" "$3" >&2; }

allowed() {
  [ -f "$ALLOW" ] || return 1
  grep -Fxq "$1" "$ALLOW" 2>/dev/null
}

while IFS="$(printf '\t')" read -r path obj; do
  [ -n "$path" ] || continue
  allowed "$path" && continue

  # --- 1. パスによる判定: ビルド / IDE 生成物 -------------------------------
  # このリポジトリで実際に何度も混入してきたもの。.gitignore をすり抜けて
  # git add -f された場合にここで止める。
  case "$path" in
    */obj/*|obj/*|*/.vs/*|.vs/*|*/x64/Debug/*|*/x64/Release/*|*.tlog/*)
      note ARTIFACT "$path" "ビルド/IDE 生成物。ローカル絶対パスを埋め込むため公開不可" ;;
    *.pdb|*.idb|*.ilk|*.iobj|*.ipdb|*.exp|*.suo|*.user|*.cache|*.ifc|*.i)
      note ARTIFACT "$path" "ビルド副産物。.gitignore を確認すること" ;;
    *.VC.db|*.db-wal|*.db-shm|*Browse.VC.db)
      note ARTIFACT "$path" "Visual Studio のローカル DB。ソース断片とパスを含む" ;;
  esac

  # --- 2. 資格情報らしいファイル名 -------------------------------------------
  case "$path" in
    *.pem|*.key|*.pfx|*.p12|*.jks|*.keystore|*.snk|*.ppk|*.ovpn|.netrc|*.npmrc)
      note CREDENTIAL-FILE "$path" "鍵・証明書の拡張子" ;;
    .env|.env.*|*/.env|*/.env.*|id_rsa|id_dsa|id_ecdsa|id_ed25519|*/id_rsa|*/id_ed25519)
      note CREDENTIAL-FILE "$path" "秘密情報を置く定番のファイル名" ;;
  esac

  # 本体が読めないものはここまで
  git cat-file -e "$obj" 2>/dev/null || continue

  # --- 3. サイズ -------------------------------------------------------------
  size=$(git cat-file -s "$obj" 2>/dev/null || echo 0)
  if [ "$size" -gt "$MAX_BYTES" ]; then
    note SIZE "$path" "$((size / 1024 / 1024)) MB — 5MB 超。成果物の混入を疑うこと"
  fi

  # --- 4. 中身 ---------------------------------------------------------------
  body=$(git cat-file blob "$obj" 2>/dev/null) || continue

  # 高信頼な秘密情報。誤検出が少ないものだけを並べている。
  hit=$(printf '%s' "$body" | grep -a -o -E \
    -e 'BEGIN (RSA|OPENSSH|DSA|EC|PGP) PRIVATE KEY' \
    -e 'gh[pousr]_[A-Za-z0-9]{36}' \
    -e 'github_pat_[A-Za-z0-9_]{30}' \
    -e 'AKIA[0-9A-Z]{16}' \
    -e 'AIza[0-9A-Za-z_-]{35}' \
    -e 'xox[abprs]-[0-9A-Za-z-]{10}' \
    -e 'sk-ant-[A-Za-z0-9_-]{20}' \
    -e '(mongodb\+srv|mongodb|postgresql|postgres|mysql|redis|amqp)://[^:/[:space:]]+:[^@[:space:]]+@' \
    2>/dev/null | head -1)
  [ -n "$hit" ] && note SECRET "$path" "$(printf '%s' "$hit" | cut -c1-40)…"

  # 個人情報。履歴から除去済みなので、再流入をここで止める。
  hit=$(printf '%s' "$body" | grep -a -o -E \
    -e '[A-Za-z]:[\/]{1,2}Users[\/]{1,2}[A-Za-z0-9._-]+' \
    -e '[A-Za-z0-9._%+-]+@(gmail|outlook|yahoo|icloud|hotmail)\.[A-Za-z.]{2,}' \
    2>/dev/null | head -1)
  [ -n "$hit" ] && note PERSONAL "$path" "$hit — ローカル絶対パス / 個人メールの露出"
done

if [ "$FOUND" -ne 0 ]; then
  cat >&2 <<'MSG'

  --------------------------------------------------------------------
  公開リポジトリに出せないものが含まれている。
  修正するか、誤検出なら .githooks-allow にパスを 1 行で足すこと。
  どうしても通したいときだけ --no-verify を付ける。
  --------------------------------------------------------------------
MSG
  exit 1
fi
exit 0
