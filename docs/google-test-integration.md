# Google Test Integration Summary

## 概要

このPRでは、kernel.vcxproj プロジェクトに Google Test を導入し、GitHub Actions で自動テストを実行する CI を構築しました。

**テストプロジェクトは `runtime/runtime.slnx` の `/tests/` フォルダに含まれています。**
以前はテスト専用の `runtime_test.slnx` に分けていましたが、gtest は `vcpkg.json` の
依存に入っていてどのビルドでも等しく復元されるため、分ける理由がありませんでした。
ソリューションが 2 つあると片方だけを直して追従漏れが起きるので統合しています。

## 実装内容

### 1. 依存関係管理 (vcpkg)

- **vcpkg.json**: Google Test (gtest) を依存関係として定義
- **vcpkg-configuration.json**: vcpkg のベースライン設定

これにより、ビルド時に自動的に Google Test がインストールされます。

### 2. テストプロジェクトの配置

テストプロジェクトは**テスト対象のモジュール配下の `test/`** に置く。

| プロジェクト | 場所 | 依存 |
|---|---|---|
| `kernel_test` | `runtime/kernel/test/` | `kernel` のみ |
| `core_test` | `runtime/core/test/` | `kernel` / `reflection` / `core` / `reflection_generated` |
| `<モジュール名>_test` | `runtime/modules/<モジュール名>/test/` | `core_test` と同じ組 + モジュール本体 + それが依存するモジュール |

ユニットテストを対象コードと同居させるのは Chromium のスタイルガイドおよび
Pitchfork Layout の Merged Test Placement に沿った形。かつて core 内部に
`runtime/core/test_support/` があり、`core.vcxproj` が旧セルフテスト本体を
core 本体へ混ぜてビルドしていたが、起動時テストの廃止に伴い
`runtime/core/test/` へ統合した。

プロジェクト名は `_test` 終わりで統一する。ReflectionGenerator が
このサフィックスでテスト実行ファイルを判別し、「全部入りヘッダ」の
include 生成から除外している（`bin/source/ReflectionGenerator/Entry.cs`）。

#### モジュールのテスト

モジュールはこれから増えていくので、テストプロジェクトは手で作らずに生成する。

```powershell
pwsh tools/module-test/new-module-test.ps1 <モジュール名>
```

- 元になるファイルは `tools/module-test/template/`。
- gtest まわりの設定 (`GTEST_LINKED_AS_SHARED_LIBRARY` / `gtest.lib` / `gtest.dll` のコピー /
  Console サブシステム) は `runtime/property_sheet/nox_test.props` に集めてあり、
  生成したプロジェクトはこれを読む。gtest の扱いを変えるときはここだけ直せばよい。
- `main.cpp` は `core_test` と同じく `nox::memory` と `nox::reflection` を初期化してから走る。
- `runtime.slnx` の `/tests/` へ `<Build Project="false" />` 付きで登録する (理由は下の CI の項)。

**依存関係**: static library はリンク時に依存先を連れてこないので、テストの実行ファイル側で
依存するライブラリを全部参照する必要がある。スクリプトは次をまとめて参照に入れる。

- `kernel` / `reflection` / `core` / `reflection_generated` (どのモジュールも core の上に載るので常に)
- テスト対象のモジュール
- そのモジュールが依存している他のモジュール。`runtime.slnx` の `BuildDependency`
  (VS の「プロジェクトの依存関係」) と vcxproj の `ProjectReference` を推移的に辿って拾う

生成した**後で**モジュールに依存を足した場合は、テスト側にも同じ `ProjectReference` を
手で足す (VS の「参照の追加」)。足し忘れると未解決の外部シンボル (LNK2019) で気付ける。

#### kernel_test

**場所**: `runtime/kernel/test/`

**構成ファイル**:
- `kernel_test.vcxproj`: Visual Studio プロジェクトファイル
- `kernel_test.vcxproj.filters`: ソースファイルのフィルタリング設定
- `pch.h` / `pch.cpp`: プリコンパイル済みヘッダー
- `main.cpp`: テストエントリポイント（Google Test の初期化）
- `README.md`: テストの使い方とドキュメント

**テストファイル**:
- `basic_test.cpp`: 基本型のサイズ検証、ポインタ型のテスト
- `math_test.cpp`: Vec2/Vec3 のテスト（構築、演算）

### 3. ソリューションへの登録

- **`runtime/runtime.slnx`**: `/tests/` フォルダに `kernel_test` / `core_test` / 各モジュールのテストを置く
  - それぞれのテスト対象プロジェクトを `BuildDependency` に持つ
  - 出力は `runtime/build/runtime/x64/<Configuration>/` に落ちる
    （`OutDir` が `build\$(SolutionName)\...` のため、ランタイム本体と同じ場所）

### 4. GitHub Actions CI

**ファイル**: `.github/workflows/ci.yml`

**追加された機能**:
1. vcpkg のセットアップ（Google Test のインストールのため）
2. `Build & test` マトリクスジョブで `runtime.slnx` をビルド
3. テスト実行ステップ
   - `runtime.slnx` にある `*_test.vcxproj` を全部拾ってビルド・実行する
     (テストを足しても `ci.yml` の追記は要らない)
   - 独立したジョブではなく、各マトリクスジョブの中で実行する
     （同じ構成を 2 回建てないため。かつては Build ジョブと Test ジョブに
     分かれており、Debug のフルビルドが 1 push につき 2 回走っていた）
   - MSVC / ClangCL の両方、Debug / Release の両方で実行
   - Master では実行しない（`test_types.h` が `#if !NOX_MASTER` で外れ、
     テスト型のリフレクションが生成されないため core_test が成立しない）
4. テスト結果のアップロード（XML 形式。アーティファクト名は
   `gtest-results-<コンパイラ>-<構成>`）

## トリガー

CI は以下の場合に自動実行されます：

1. **コミット時**: すべてのブランチへの push
   （`**.md` / `docs/**` だけの変更は除く）
2. **手動実行**: GitHub UI の「Run workflow」

`pull_request` トリガーは使っていません。同一リポジトリのブランチへの PR は
push 側と合わせて毎回 2 倍走り、ref が違うので concurrency でも畳めないためです。
3. **手動実行**: GitHub Actions の UI から workflow_dispatch で手動実行可能

## テストの実行方法

### ローカルでの実行

#### Visual Studio から
1. `runtime/runtime.slnx` を開く
2. `tests` フォルダの `kernel_test` プロジェクトをビルド
3. 実行可能ファイルを直接実行、またはテストエクスプローラーから実行

#### コマンドライン
```cmd
cd runtime
msbuild runtime.slnx /p:Configuration=Debug /p:Platform=x64
build\runtime\x64\Debug\kernel_test.exe
```

### CI での実行

GitHub にプッシュすると自動的に実行されます。
テスト結果は GitHub Actions の Artifacts としてダウンロード可能です
（`gtest-results-<コンパイラ>-<構成>`）。

## テストの追加方法

1. `runtime/kernel/test/` に新しい `*_test.cpp` ファイルを作成
2. `kernel_test.vcxproj` に `<ClCompile Include="...">` を追加
3. Google Test のマクロを使用してテストを記述：

```cpp
#include "pch.h"
#include "../your_header.h"

TEST(TestSuiteName, TestName)
{
    // Arrange
    int expected = 42;
    
    // Act
    int actual = YourFunction();
    
    // Assert
    EXPECT_EQ(expected, actual);
}
```

## 技術的詳細

### プロジェクト構成
- **タイプ**: Application (.exe)
- **プラットフォーム**: x64
- **構成**: Debug, Release, Master
- **ツールセット**: v145 (Visual Studio 2022)
- **依存関係**: kernel.lib

### ビルド設定
- プリコンパイル済みヘッダー: pch.h
- インクルードパス: `$(ProjectDir);$(ProjectDir)..`
- vcpkg 統合: 有効

### CI 設定
- OS: windows-latest
- ビルドツール: MSBuild
- パッケージマネージャー: vcpkg
- テストフレームワーク: Google Test
- 結果出力: XML (JUnit 形式)

## 今後の拡張

- [ ] テストカバレッジレポートの追加
- [ ] より多くの kernel 機能のテスト追加
- [ ] パフォーマンステストの追加
- [ ] メモリリークテストの追加

## 参考資料

- [Google Test Documentation](https://google.github.io/googletest/)
- [vcpkg Documentation](https://vcpkg.io/)
- [GitHub Actions Documentation](https://docs.github.com/actions)
