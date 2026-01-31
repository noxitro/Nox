# Kernel Test Suite

このディレクトリには、kernel ライブラリの単体テストが含まれています。

## 重要事項

**テストプロジェクトは通常のソリューション（runtime.sln/runtime.slnx）には含まれていません。**

テスト専用の `runtime_test.slnx` を使用してビルドします。これにより、通常の開発作業時にテストプロジェクトの読み込みによる影響を避けることができます。

## テストフレームワーク

Google Test を使用しています。依存関係は vcpkg を通じて管理されています。

## テストのビルド

### Visual Studio でのビルド

1. `runtime/runtime_test.slnx` を開く（**注意**: runtime.slnx ではありません）
2. `kernel_test` プロジェクトを選択
3. ビルド実行（vcpkg が自動的に Google Test をインストールします）

### コマンドラインでのビルド

```cmd
cd runtime
msbuild runtime_test.slnx /p:Configuration=Debug /p:Platform=x64
```

## テストの実行

### Visual Studio から実行

テストエクスプローラーから実行、または `kernel_test.exe` を直接実行できます。

### コマンドラインから実行

```cmd
runtime\build\runtime_test\x64\Debug\kernel_test.exe
```

### テストオプション

Google Test は様々なコマンドラインオプションをサポートしています：

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
3. テストコードを記述：

```cpp
#include "stdafx.h"
#include "../your_header.h"

TEST(TestSuiteName, TestName)
{
    // テストコード
    EXPECT_EQ(expected, actual);
}
```

## CI での実行

GitHub Actions ワークフローが自動的にテストをビルド・実行します：

- トリガー: すべてのコミット、PR、手動実行
- 構成: Debug, Release, Master
- テスト結果はアーティファクトとしてアップロードされます

## 現在のテストカバレッジ

- `basic_test.cpp`: 基本型とサイズ検証、文字列ユーティリティ

## 参考資料

- [Google Test Documentation](https://google.github.io/googletest/)
- [vcpkg Documentation](https://vcpkg.io/)
