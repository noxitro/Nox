# reflection_generated_test

`core_test` だけがリンクする、テスト用のリフレクション生成プロジェクト。
本体 (`reflection_generated`) と同じ生成器を、テスト用の型を足した解析の起点で走らせる。

## 分けてある理由

生成器は解析の起点 (`reflect.cpp`) から辿れる型を、リフレクション情報と ECS の購読テーブルに書き出す。
購読テーブルに載った EntitySystem / EntityLogic は `World` の初期化で実体化され、毎フレームの更新に乗る。

以前は本体の `reflect.cpp` / `pch.h` がテスト用の型 (`core/test/test_types.h`) を `#if !NOX_MASTER` で読んでいた。
そのため Debug / Release の `runtime.exe` でテスト用の System / Logic が購読され、
Editor が読む RuntimeTypeDB にも `nox::test` の型が載っていた。逆に Master では生成されず、`core_test` が成り立たなかった。

今は次のように分けている。

| | 解析の起点 | リンクする実行ファイル | テスト用の型 | RuntimeTypeDB |
|---|---|---|---|---|
| `reflection_generated` | `reflection_generated/reflect.cpp` | `runtime.exe` / `bench_test.exe` | 含まない | 書く |
| `reflection_generated_test` | `reflection_generated_test/reflect.cpp` | `core_test.exe` | 全構成で含む | 書かない |

`core_test` は本体の代わりにこちらをリンクする (両方をリンクすると生成コードのシンボルが重複する)。
そのため、こちらの解析の起点もエンジンの型をすべて見せる。

## 中身

| ファイル | 内容 |
|---|---|
| `reflect.cpp` | 解析の起点。本体と同じ代表ヘッダに `core/test/test_types.h` を足したもの |
| `pch.h` | 生成コードが読む PCH。本体の `pch.h` に `test_types.h` を足したもの |
| `gen.h` / `reflection_generated_test.h` | 本体のヘッダへ回すだけ |
| `Directory.Build.targets` | 本体の `Directory.Build.targets` を読み込み、`NoxReflectionWriteTypeDB=false` にする |

`InitializeGen` / `FinalizeGen` は本体の `register.cpp` をそのままコンパイルして使う。

## ビルド

`runtime.slnx` では `core_test` と同じく既定のビルドから外してある (`<Build Project="false" />`)。
CI は `core_test` の前にこのプロジェクトを建てる (`.github/workflows/ci.yml` の Build test projects)。

生成物は `gen/` (.gitignore 済み) に出る。RuntimeTypeDB と `%TEMP%` のポインタファイルは書かない。
