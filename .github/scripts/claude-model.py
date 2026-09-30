#!/usr/bin/env python3
"""/claude コメントからモデルを選ぶ (claude.yml から呼ぶ)。

「/claude fable レビューして」のように、/claude の直後の語がモデル名なら
そのモデルで動かす。省いたときは DEFAULT_MODEL。

モデル名に近いが一致しない語 (fabel / opsu / sonet など) は打ち間違いとみなし、
Claude を起動せずに「もしかして」を返す。違うモデルで走って利用枠を無駄にしないため。
英単語の取り違え (table → fable など) を避けるため、先頭の文字が同じ語だけを候補にする。

入力 : 環境変数 BODY (コメント本文)。
出力 : GITHUB_OUTPUT に次を書く (未設定なら標準出力)。
    model      … 使うモデルの ID。打ち間違いのときは空。
    suggestion … 打ち間違いのときの返信本文。それ以外は空。

使い方 (手元での確認):
    BODY='/claude fable レビューして' python3 .github/scripts/claude-model.py
"""

import difflib
import os
import re
import sys

# 名前 → モデル ID。コメントの語は小文字にして「-」「_」「.」と先頭の「claude」を
# 除いてから引く (fable-5.1 / Fable5.1 / claude-fable-5-1 はどれも fable51 になる)。
MODELS = {
    "fable": "claude-fable-5-1",
    "opus": "claude-opus-5-5",
    "sonnet": "claude-sonnet-5-5",
    "haiku": "claude-haiku-4-5-20251001",
}
ALIASES = {
    "fable": "fable",
    "fable51": "fable",
    "opus": "opus",
    "opus55": "opus",
    "sonnet": "sonnet",
    "sonnet55": "sonnet",
    "haiku": "haiku",
    "haiku45": "haiku",
}
DEFAULT_MODEL = "opus"

TRIGGER = "/claude"
# 打ち間違いとみなす近さ (difflib の ratio)。fabel / opsu / sonet / hiaku が 0.75 以上になる
TYPO_RATIO = 0.75


def normalize(word):
    word = re.sub(r"[-_.]", "", word.lower())
    if word.startswith("claude") and len(word) > len("claude"):
        word = word[len("claude"):]
    return word


def first_word(body):
    """/claude の直後の語。/claude に続けて書いた場合は claude.yml 側で弾かれるので考えない。"""
    rest = body.lstrip()[len(TRIGGER):]
    words = rest.split()
    if not words:
        return ""
    # 「fableで」「opus?」のように続けて書いた日本語や記号は外す
    m = re.match(r"[A-Za-z0-9._-]+", words[0])
    return m.group(0).rstrip(".-_") if m else ""


def choose(body):
    """(モデル名, 打ち間違いの候補) を返す。どちらか一方だけが入る。"""
    word = first_word(body)
    key = normalize(word)
    if key in ALIASES:
        return ALIASES[key], None
    # 英数字の語だけが候補になる (first_word が日本語などを外してある)
    if key:
        base = re.sub(r"[0-9]+$", "", key)
        for name in MODELS:
            if base and base[0] == name[0] and difflib.SequenceMatcher(None, base, name).ratio() >= TYPO_RATIO:
                return None, (word, name)
    return DEFAULT_MODEL, None


def suggestion_text(word, name):
    names = " / ".join(f"`{n}`" for n in MODELS)
    return (
        f"`{word}` は `{name}` のことですか？モデル名が読み取れなかったので、Claude は起動していません。\n\n"
        f"`/claude {name} ...` のように書き直してください。指定できるモデルは {names} です "
        f"(省くと `{DEFAULT_MODEL}`)。"
    )


def main():
    name, typo = choose(os.environ.get("BODY", ""))
    outputs = {
        "model": MODELS[name] if name else "",
        "suggestion": suggestion_text(*typo) if typo else "",
    }
    lines = []
    for key, value in outputs.items():
        # 複数行の値は区切り文字つきの書式で渡す
        lines.append(f"{key}<<__NOX_EOF__\n{value}\n__NOX_EOF__\n")
    path = os.environ.get("GITHUB_OUTPUT")
    if path:
        with open(path, "a", encoding="utf-8") as f:
            f.writelines(lines)
    else:
        sys.stdout.writelines(lines)
    return 0


if __name__ == "__main__":
    sys.exit(main())
