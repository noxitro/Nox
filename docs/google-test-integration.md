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
| `core_test` | `runtime/core/test/` | `kernel` / `reflection` / `core` / `reflection_generated_test` |

ユニットテストを対象コードと同居させるのは Chromium のスタイルガイドおよび
Pitchfork Layout の Merged Test Placement に沿った形。かつて core 内部に
`runtime/core/test_support/` があり、`core.vcxproj` が旧セルフテスト本体を
core 本体へ混ぜてビルドしていたが、起動時テストの廃止に伴い
`runtime/core/test/` へ統合した。

プロジェクト名は `_test` 終わりで統一する。ReflectionGenerator が
このサフィックスでテスト実行ファイルを判別し、「全部入りヘッダ」の
include 生成から除外している（`bin/source/ReflectionGenerator/Entry.cs`）。

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

- **`runtime/runtime.slnx`**: `/tests/` フォルダに `kernel_test` / `core_test` と、
  `core_test` がリンクする `reflection_generated_test` を置く
  - それぞれのテスト対象プロジェクトを `BuildDependency` に持つ
  - 3 つとも `<Build Project="false" />` が付いていて、ソリューションのビルドでは建たない。
    ソリューションを建てた後に vcxproj を直接建てる（「テストの実行方法」）
  - 出力は `runtime/build/runtime/x64/<Configuration>/` に落ちる
    （`OutDir` が `$(SolutionDir)build\$(SolutionName)\...` のため、ランタイム本体と同じ場所。
    vcxproj を直接建てるときも `SolutionDir` / `SolutionName` を渡してここに揃える）

### 4. GitHub Actions CI

**ファイル**: `.github/workflows/ci.yml`

**追加された機能**:
1. vcpkg のセットアップ（Google Test のインストールのため）
2. `Build & test` マトリクスジョブで `runtime.slnx` をビルドし、続く「Build test projects」で
   `kernel_test` → `reflection_generated_test` → `core_test` の vcxproj を直接建てる
3. テスト実行ステップ
   - 独立したジョブではなく、各マトリクスジョブの中で実行する
     （同じ構成を 2 回建てないため。かつては Build ジョブと Test ジョブに
     分かれており、Debug のフルビルドが 1 push につき 2 回走っていた）
   - MSVC / ClangCL の両方、Debug / Release / Master のすべてで実行
     （テスト用の型のリフレクションは `reflection_generated_test` が全構成で生成する。
     `runtime.exe` がリンクする `reflection_generated` には入らない）
4. テスト結果のアップロード（XML 形式。アーティファクト名は
   `gtest-results-<コンパイラ>-<構成>`）

## トリガー

CI は以下の場合に自動実行されます：

1. **コミット時**: すべてのブランチへの push（`ci-media/**` を除く）
2. **手動実行**: GitHub UI の「Run workflow」（workflow_dispatch）

`paths-ignore` は置いていないので、文書だけの変更でも run は起きます。
plan ジョブが、文書など（`.md`・`docs/`・ルートの `.gitignore` と `LICENSE`・`tools/bench-site/`）を
除いた木から `ci_key` を求め、同じ `ci_key` の木が前に CI gate まで通っていれば
（今の ref か main の記録。main では main 自身の記録だけ）、Windows のジョブを全部飛ばして
CI gate だけを緑にします。run ごと起こさない形にすると、文書だけの push をした PR で
必須チェック（CI gate）が報告されず、マージできないまま残るためです。
詳細は `.github/workflows/ci.yml` 冒頭のコメントの「文書」と plan ジョブの説明を参照。

`pull_request` トリガーは使っていません。同一リポジトリのブランチへの PR は
push 側と合わせて毎回 2 倍走り、ref が違うので concurrency でも畳めないためです。

## テストの実行方法

### ローカルでの実行

#### Visual Studio から
1. `runtime/runtime.slnx` を開く
2. `tests` フォルダの `kernel_test` プロジェクトをビルド
3. 実行可能ファイルを直接実行、またはテストエクスプローラーから実行

#### コマンドライン

テストプロジェクトはソリューションのビルドでは建たないので、CI の「Build test projects」と同じく、
ソリューションを建てた後に `kernel_test` → `reflection_generated_test` → `core_test` の順で
vcxproj を直接建てます。`SolutionDir` は `\` 区切りのまま、末尾に `\` を付けて渡します。

```cmd
cd runtime
msbuild runtime.slnx -p:Configuration=Debug -p:Platform=x64 -m
set TEST_PROPS=-p:Configuration=Debug -p:Platform=x64 -p:SolutionDir=%CD%\ -p:SolutionName=runtime -p:SolutionPath=%CD%\runtime.slnx -p:BuildProjectReferences=false -m
msbuild kernel\test\kernel_test.vcxproj %TEST_PROPS%
msbuild reflection_generated_test\reflection_generated_test.vcxproj %TEST_PROPS%
msbuild core\test\core_test.vcxproj %TEST_PROPS%
build\runtime\x64\Debug\kernel_test.exe
build\runtime\x64\Debug\core_test.exe
```

各プロパティの意味と引用符の注意は `runtime/kernel/test/README.md` の「コマンドラインでのビルド」、
手元でビルドするときの前提（生成器の準備、ワークツリーでのビルド、vcpkg の共有）は
`docs/local-build.md` を参照。

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
- **ツールセット**: v145 (Visual Studio 2026)
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
