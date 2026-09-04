# core_test

`core` / `reflection` / `reflection_generated` に依存する GoogleTest スイート。

## kernel_test と分けてある理由

依存の向きが逆になるため。`kernel` は最下層で、`core` は
`kernel` + `reflection` の上に乗っている。`kernel_test` に core のテストを足すと、
kernel の単体テストがエンジン全部とコード生成器を引きずることになる。

- `kernel_test` … `kernel.vcxproj` だけを参照する。生成器は動かない。
- `core_test`   … `kernel` / `reflection` / `core` / `reflection_generated` を参照する。
  リフレクションの getter / setter は **生成コードを実際に走らせないと検証できない**ので、
  こちらはビルド時に `ReflectionGenerator.exe` が動く。

どちらも `runtime/runtime_test.slnx` に入っている（`runtime.slnx` には入れない。
理由は `kernel/test/README.md` を参照）。

## 中身

| ファイル | 内容 |
|---|---|
| `main.cpp` | エントリポイント。`nox::memory::Initialize` → `nox::reflection::Initialize` の順で初期化してから `RUN_ALL_TESTS()` |
| `test_new_delete.cpp` | gtest の DLL とヒープを揃えるための標準 `operator new` / `delete`（`kernel/test/README.md` の説明と同じ理由。これが無いと `main` 到達前に落ちる） |
| `core_self_test.cpp` | `core/test/*.cpp` にある `NOX_ASSERT` ベースのセルフテストを gtest のケースとして走らせる |
| `reflection_variable_test.cpp` | `nox::reflection::VariableInfo` の getter / setter / アドレス取得の回帰テスト |

## リフレクションのテスト用型について

`reflection_variable_test.cpp` が使う型は
`runtime/core/test/reflection_variable_test_types.h` にある。

リフレクション生成器は解析の起点 (`reflection_generated/reflect.cpp`) から
辿れる型しか見ないため、テスト専用の型もそこから見える場所に置く必要がある。
既に同じ目的で `core/test/test_types.h` という集約ヘッダがあるので、それに乗せている
（`test_types.h` は `reflect.cpp` と `reflection_generated/pch.h` の両方から
`#if !NOX_MASTER` 付きで include されている）。

## ビルドと実行

```cmd
cd runtime
msbuild runtime_test.slnx -p:Configuration=Debug -p:Platform=x64 -m
build\runtime_test\x64\Debug\core_test.exe
```

`main` が `nox::reflection::Initialize()` を呼ぶので、
`reflection_generated` がリンクされていないと生成コードの登録関数が無く、
`FindClassInfo` が全部 `nullptr` を返して落ちる。
