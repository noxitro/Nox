# core_test

`core` / `reflection` / `reflection_generated_test` に依存する GoogleTest スイート。

## 配置

テスト対象のモジュール配下に置いてある（`runtime/core/test/`）。`kernel` 側も
同じ規則で `runtime/kernel/test/`。ユニットテストを対象コードと同居させるのは
Chromium のスタイルガイドと Pitchfork Layout の Merged Test Placement に沿った形。

かつては隣に `runtime/core/test_support/` があり、`core.vcxproj` が
旧 `NOX_ASSERT` ベースのセルフテスト本体を core 本体へ混ぜてビルドしていた
（`runtime.exe` の起動時に一部が走っていた）。起動時テストは廃止し、
中身はすべてこのディレクトリへ移してある。リフレクション生成器に見せる
テスト用型（`test_types.h` 系）も同様にここにある。

## kernel_test と分けてある理由

依存の向きが逆になるため。`kernel` は最下層で、`core` は
`kernel` + `reflection` の上に乗っている。`kernel_test` に core のテストを足すと、
kernel の単体テストがエンジン全部とコード生成器を引きずることになる。

- `kernel_test` … `kernel.vcxproj` だけを参照する。生成器は動かない。
- `core_test`   … `kernel` / `reflection` / `core` / `reflection_generated_test` を参照する。
  リフレクションの getter / setter は **生成コードを実際に走らせないと検証できない**ので、
  こちらはビルド時に `ReflectionGenerator.exe` が動く。

どちらも `runtime/runtime.slnx` の `tests` フォルダに入っている
（かつて分けていた `runtime_test.slnx` を統合した経緯は `kernel/test/README.md` を参照）。

## 中身

| ファイル | 内容 |
|---|---|
| `main.cpp` | エントリポイント。`nox::memory::Initialize` → `nox::reflection::Initialize` の順で初期化してから `RUN_ALL_TESTS()` |
| `test_new_delete.cpp` | gtest の DLL とヒープを揃えるための標準 `operator new` / `delete`（`kernel/test/README.md` の説明と同じ理由。これが無いと `main` 到達前に落ちる） |
| `core_self_test.cpp` | 同ディレクトリの `NOX_ASSERT` ベースのセルフテスト（`delegate_test.cpp` / `entity_command_buffer_test.cpp` / `entity_ecs_test.cpp` / `job_system_test.cpp` / `test_reflection.cpp`）を gtest のケースとして走らせる |
| `reflection_variable_test.cpp` | `nox::reflection::VariableInfo` の getter / setter / アドレス取得の回帰テスト |
| `updater_graph_layering_test.cpp` | `UpdaterGraph` の衝突判定とレイヤリングの検証。時間に依存しない |
| `updater_graph_benchmark.cpp` | 並列化が実際に効くかの実測。全ケース `DISABLED_` で CI では走らない |
| `updater_worker_count_test.cpp` | `--serial-updater` / `--updater-workers=N` の解析規則の検証 |

### UpdaterGraph のテストについて

`updater_graph_layering_test.cpp` は `nox::legacy::ConflictsUpdaterNodeAccess` /
`nox::legacy::BuildUpdaterLayerIndices` / `nox::legacy::UpdaterGraph::Rebuild` を直接叩く。
いずれも public かつ `World` 非依存なので、`World::Init()` が private でも検証できる
（`updater_graph_legacy.h` にもその意図が書いてある）。

EntityLogic の更新メソッドは通常リフレクション生成コードが購読するが、ここでは
`nox::legacy::EntityLogicMethodTable` の手書き特殊化（`entity_logic_legacy.h` が用意している
エスケープハッチ）を使っている。おかげでテスト専用の型を `core/test/test_types.h` へ
足す必要がなく、テストが生成器の出力に依存しない。

`updater_graph_benchmark.cpp` は実行時間に依存するので CI に載せない。手で測るときは:

```cmd
build\runtime\x64\Release\core_test.exe --gtest_also_run_disabled_tests --gtest_filter=UpdaterGraphBenchmark.*
```

ワーカー数を振って中央値を表で出す。`runtime.exe` 側で同じことをするには
`--serial-updater`（0本）か `--updater-workers=N` を渡す。

その解析は `nox::legacy::ResolveUpdaterWorkerCount`（`world_legacy.h`。`World` のメンバではなく
自由関数）が担う。「引数列 + 既定値」だけを見る純粋関数なので、
`World` を組み立てず、プロセスの実引数にも論理プロセッサ数にも依存せずに
`updater_worker_count_test.cpp` から全分岐を踏める。
テスト都合で `World` の公開範囲は広げていない。

## リフレクションのテスト用型について

`reflection_variable_test.cpp` が使う型は
`runtime/core/test/reflection_variable_test_types.h` にある。

リフレクション生成器は解析の起点 (`reflect.cpp`) から辿れる型しか見ない。
テスト専用の型は集約ヘッダ `core/test/test_types.h` に載せ、テスト用の生成プロジェクト
`reflection_generated_test` の `reflect.cpp` と `pch.h` からだけ include する (全構成)。

本体の `reflection_generated` (`runtime.exe` がリンクする) からは見せない。
見せると `runtime.exe` がテスト用の System / Logic を購読し、毎フレームの更新に乗せてしまう。
`core_test` は本体の代わりに `reflection_generated_test` をリンクする。
詳細は `runtime/reflection_generated_test/README.md`。

## ビルドと実行

`core_test` と `reflection_generated_test` は `runtime.slnx` の既定のビルドから外してある
(`<Build Project="false" />`)。ソリューションを建てた後、`reflection_generated_test` → `core_test` の順に建てる
(CI の Build test projects と同じ手順。プロパティの渡し方は `.github/workflows/ci.yml` を参照)。
Master を含む全構成で建てられる。
Visual Studio では `reflection_generated_test` を先に右クリックでビルドする (依存先としては建たないことがある。`runtime/reflection_generated_test/README.md`)。

```cmd
build\runtime\x64\Debug\core_test.exe
```

`main` が `nox::reflection::Initialize()` を呼ぶので、
`reflection_generated_test` がリンクされていないと生成コードの登録関数が無く、
`FindClassInfo` が全部 `nullptr` を返して落ちる。
