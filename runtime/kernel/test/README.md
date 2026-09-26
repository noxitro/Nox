# Kernel Test Suite

このディレクトリには、kernel ライブラリの単体テストが含まれています。

## 重要事項

**テストプロジェクトは `runtime.slnx` の `tests` フォルダに入っています。**

以前はテスト専用の `runtime_test.slnx` に分けていましたが、統合しました。
分けていた理由（gtest が無い環境でソリューション全体が壊れる）は成立しません。
gtest はリポジトリルートの `vcpkg.json` の依存に入っていて、`runtime.slnx` を
建てれば vcpkg のマニフェストモードが他の依存と一緒に復元するためです。
ソリューションが 2 つあると、プロジェクト追加を片方だけに入れて
追従漏れが起きる方が実害が大きいので統合しています。

## テストフレームワーク

Google Test を使用しています。依存関係はリポジトリルートの `vcpkg.json` の
`dependencies` に `gtest` として宣言されており、vcpkg のマニフェストモードで
インストールされます。

インストール先は `vcpkg_installed/x64-windows/` です。
このトリプレットでは gtest は **DLL（共有ライブラリ）** としてビルドされるため、

- `kernel_test.vcxproj` は `GTEST_LINKED_AS_SHARED_LIBRARY` を定義します。
- `gtest.lib`（インポートライブラリ）をリンクします。
- PostBuildEvent で `gtest.dll` を出力ディレクトリにコピーします。
  コピー元は Debug が `$(NoxVcpkgInstalledDir)debug\bin\`、
  Release / Master が `$(NoxVcpkgInstalledDir)bin\` です。

### `NoxVcpkgInstalledDir` の上書き

`runtime/property_sheet/nox_common.props` が `NoxVcpkgInstalledDir` を定義しています。
インストール済みツリーの場所は次の優先順位で決まります。

1. 環境変数 `NOX_VCPKG_INSTALLED_DIR`
2. 追跡外の `runtime\Directory.Build.user.props` で設定した `NoxVcpkgInstalledRoot`
3. 既定（リポジトリ直下の `vcpkg_installed`）

指すのはトリプレットディレクトリの親（`x64-windows` の 1 つ上）です。
git worktree で 1 つのインストール済みツリーを共有したい場合は 1 か 2 を使います。

## `operator new` の差し替えについて（重要）

kernel は `memory/new_delete.h` でグローバルの `operator new` / `operator delete` を
`nox::memory::Allocate` / `Deallocate` に差し替えています。

一方 gtest は上記のとおり DLL としてリンクされ、DLL 側は CRT 標準の
`operator new` / `delete` を使います。exe 側だけ独自アロケータになると、
gtest.dll が確保したメモリを exe が解放する（またはその逆）経路で
ヘッダ付きブロックと生ブロックが混ざり、ヒープが壊れてプロセスがハングします。

そのため `test_new_delete.cpp` でテスト実行ファイル用に標準の
`operator new` / `delete` を定義しています。リンカはライブラリ
（`kernel.lib` の `new_delete.obj`）より先にオブジェクトファイル側の定義を採用するため、
exe と gtest.dll が同じ CRT ヒープを共有する状態になります。

**この結果 `kernel_test` は `nox::memory` のグローバルアロケータ自体は検証しません。**
`nox::memory::Allocate` / `Deallocate` を直接呼ぶ経路（`memory/pmr.cpp`,
`memory/stl_allocate_adapter.cpp`）は差し替えの影響を受けないため、
そちらは通常どおり動作します。

将来 gtest を静的リンク（`x64-windows-static-md` トリプレット等）に切り替えられれば
この回避は不要になり、エンジンのアロケータごとテストできるようになります。

## プリコンパイル済みヘッダー

- `pch.h` が唯一のプリコンパイル済みヘッダーで、`<gtest/gtest.h>` を含みます。
- `pch.cpp` だけが `PrecompiledHeader=Create`、他の `.cpp` はすべて `Use` です。
- したがって、このディレクトリの `.cpp` は必ず先頭で `#include "pch.h"` してください。

## テストのビルド

### Visual Studio でのビルド

1. `runtime/runtime.slnx` を開く
2. `tests` フォルダの `kernel_test` プロジェクトを選択
3. ビルド実行

### コマンドラインでのビルド

```cmd
cd runtime
msbuild runtime.slnx -p:Configuration=Debug -p:Platform=x64 -m
```

vcpkg のインストール先を差し替える場合:

```cmd
set NOX_VCPKG_INSTALLED_DIR=E:\path\to\vcpkg_installed
msbuild runtime.slnx -p:Configuration=Debug -p:Platform=x64 -m
```

## テストの実行

出力先は `runtime\build\runtime\x64\Debug\` です
（`OutDir` は `$(SolutionDir)build\$(SolutionName)\$(Platform)\$(Configuration)\`）。
`gtest.dll` は PostBuildEvent で同じディレクトリにコピー済みなので、
そのまま実行できます。

```cmd
runtime\build\runtime\x64\Debug\kernel_test.exe
```

終了コードは、全テスト成功で 0、失敗があれば 1 です。

### テストオプション

```cmd
# すべてのテストを実行
kernel_test.exe

# 特定のテストのみ実行
kernel_test.exe --gtest_filter=KernelBasicTest.*

# テスト結果を XML で出力
kernel_test.exe --gtest_output=xml:test_results.xml

# ヘルプを表示
kernel_test.exe --help
```

## テストの追加

新しいテストファイルを追加する場合：

1. `kernel/test/` ディレクトリに新しい `.cpp` ファイルを作成
2. `kernel_test.vcxproj` に `<ClCompile Include="...">` を追加
   （`kernel_test.vcxproj.filters` にも追加する）
3. テストコードを記述：

```cpp
#include "pch.h"
#include "../your_header.h"

TEST(TestSuiteName, TestName)
{
    // テストコード
    EXPECT_EQ(actual, expected);
}
```

エントリポイントは `main.cpp` の `InitGoogleTest` + `RUN_ALL_TESTS` です
（`gtest_main` はリンクしていません）。テストファイル側に `main` は不要です。

## CI での扱い

`.github/workflows/ci.yml` の `test` ジョブが `runtime.slnx` を Debug で建て、
`kernel_test.exe` と `core_test.exe` の両方を実行します。
どちらか 1 本でも失敗すればジョブが落ちます。

## core のテストについて

`core` / `reflection` に依存するテストは `kernel_test` には入れません
（kernel の単体テストがエンジン全部とコード生成器を引きずってしまうため）。
`runtime/core/test/` に別プロジェクトがあります。

## 現在のテストカバレッジ

- `basic_test.cpp`: 基本型のサイズ検証、ポインタ型、リフレクションの
  関数属性 / フィールド属性（static 判定）
- `math_test.cpp`: `nox::Vec2` / `nox::Vec3` の構築・加減算・スカラー倍
- `file_win64_test.cpp`: UTF-8 パスでのバイナリ読み書き、追記モード

## 参考資料

- [Google Test Documentation](https://google.github.io/googletest/)
- [vcpkg Documentation](https://vcpkg.io/)
