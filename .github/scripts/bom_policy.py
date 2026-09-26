"""どのファイルに UTF-8 BOM を付けるかの決まり。check-file-format.py と strip-bom.py が使う。

リポジトリのテキストファイルは BOM なしの UTF-8 に統一してある (AGENTS.md の「ファイル形式」)。
BOM が要るのは、BOM が無いとシステムのコードページ (日本語 Windows では Shift-JIS) で
読まれてしまう次のものだけ。.editorconfig の charset もこれと揃えること。
"""

import fnmatch

# BOM が必要なもの
REQUIRED = [
    # Windows PowerShell 5.1 は BOM の無いスクリプトを Shift-JIS として読む
    "*.ps1",
    "*.psm1",
    "*.psd1",
    # Visual Studio のテンプレートのうち日本語を含むもの。パラメーター置換で読み込まれる
    "tools/vs-templates/nox_module/root.h",
    "tools/vs-templates/nox_module/NoxModule.vcxproj.filters",
]

# どちらでもよいもの (移行中)
ANY = [
    # リポジトリに入っている MakeAllIncludeHeader.exe が古く、ビルドのたびに BOM 付きで
    # 書き出す。exe を作り直したら (生成器のソースは BOM なしで書くよう直してある) 消す
    "runtime/app/all_include.h",
    "runtime/app/all_include.cpp",
]

REQUIRED_POLICY = "required"
FORBIDDEN_POLICY = "forbidden"
ANY_POLICY = "any"


def policy(path):
    """path (リポジトリのルートからの / 区切りのパス) の BOM の決まりを返す。"""
    if any(fnmatch.fnmatch(path, p) for p in REQUIRED):
        return REQUIRED_POLICY
    if any(fnmatch.fnmatch(path, p) for p in ANY):
        return ANY_POLICY
    return FORBIDDEN_POLICY
