# Google Test Integration Summary

## 概要

このPRでは、kernel.vcxproj プロジェクトに Google Test を導入し、GitHub Actions で自動テストを実行する CI を構築しました。

**重要**: テストプロジェクトは通常のソリューションファイル（runtime.sln/runtime.slnx）には含まれていません。テスト専用の `runtime_test.slnx` を使用してビルドします。これにより、通常の開発作業時にテストプロジェクトの読み込みによる影響を避けることができます。

## 実装内容

### 1. 依存関係管理 (vcpkg)

- **vcpkg.json**: Google Test (gtest) を依存関係として定義
- **vcpkg-configuration.json**: vcpkg のベースライン設定

これにより、ビルド時に自動的に Google Test がインストールされます。

### 2. テストプロジェクト (kernel_test)

**場所**: `runtime/kernel/test/`

**構成ファイル**:
- `kernel_test.vcxproj`: Visual Studio プロジェクトファイル
- `kernel_test.vcxproj.filters`: ソースファイルのフィルタリング設定
- `stdafx.h` / `stdafx.cpp`: プリコンパイル済みヘッダー
- `main.cpp`: テストエントリポイント（Google Test の初期化）
- `README.md`: テストの使い方とドキュメント

**テストファイル**:
- `basic_test.cpp`: 基本型のサイズ検証、ポインタ型のテスト
- `math_test.cpp`: Vec2/Vec3 のテスト（構築、演算）

### 3. テスト専用ソリューション

- **`runtime/runtime_test.slnx`**: テスト専用のソリューションファイル（新規作成）
  - kernel プロジェクトと kernel_test プロジェクトのみを含む
  - 通常のソリューション（runtime.sln/runtime.slnx）には影響しない
  - CI でのみ使用される

### 4. GitHub Actions CI

**ファイル**: `.github/workflows/cibuild_runtime.yml`

**追加された機能**:
1. vcpkg のセットアップ（Google Test のインストールのため）
2. テスト専用ソリューション（runtime_test.slnx）をビルドマトリクスに追加
3. テスト実行ステップ
   - runtime_test.slnx ビルド時のみ実行
   - MSVC ツールチェーンのみ（clang は除外）
   - すべての構成（Debug, Release, Master）で実行
4. テスト結果のアップロード（XML 形式）

**注意**: 通常のソリューション（runtime.slnx）はテストプロジェクトを含まないため、ビルドマトリクスに両方のソリューションが含まれています。

## トリガー

CI は以下の場合に自動実行されます：

1. **コミット時**: すべてのブランチへの push
2. **プルリクエスト**: すべてのブランチへの PR
3. **手動実行**: GitHub Actions の UI から workflow_dispatch で手動実行可能

## テストの実行方法

### ローカルでの実行

#### Visual Studio から
1. `runtime/runtime_test.slnx` を開く（**注意**: runtime.slnx ではありません）
2. `kernel_test` プロジェクトをビルド
3. 実行可能ファイルを直接実行、またはテストエクスプローラーから実行

#### コマンドライン
```cmd
cd runtime
msbuild runtime_test.slnx /p:Configuration=Debug /p:Platform=x64
runtime\build\runtime_test\x64\Debug\kernel_test.exe
```

### CI での実行

GitHub にプッシュまたは PR を作成すると、自動的に実行されます。
テスト結果は GitHub Actions の Artifacts としてダウンロード可能です。

## テストの追加方法

1. `runtime/kernel/test/` に新しい `*_test.cpp` ファイルを作成
2. `kernel_test.vcxproj` に `<ClCompile Include="...">` を追加
3. Google Test のマクロを使用してテストを記述：

```cpp
#include "stdafx.h"
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
- プリコンパイル済みヘッダー: stdafx.h
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
