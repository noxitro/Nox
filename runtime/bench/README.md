# runtime/bench — ベンチマーク

エンジンの実行時コスト (時間と、1 op あたりのヒープ確保回数) を測るコンソールアプリ `bench_test.exe`。
CI (`.github/workflows/ci.yml`) が push のたびに Release / Master で計測し、結果を
GitHub Pages の結果ページ (`tools/bench-site`) に積んでいく。

## 何を測るか

| グループ | 例 | 見どころ |
|---|---|---|
| `ecs` | クエリ列挙、Chunk 並列列挙、ランダムアクセス、Archetype 移動、遅延生成 | フレームごとに効く本体 |
| `job` | Dispatch + Wait の往復、1024 ジョブの分配 | ワーカーの起こし方で桁が変わる |
| `memory` | `nox::memory::Allocate` / グローバル new / CRT malloc | 確保 1 回の値段 |
| `container` | `nox::Vector` / `StackAllocVector` / `FixedVector` | 確保 0 回の書き方がいくら得か |
| `string` / `delegate` / `math` / `lock` | 書式化、CRC32、UTF 変換、delegate 呼び出し、Vec3、SRWLOCK | 基盤部品 |

一覧は `bench_test.exe --list` で出る。ベンチの名前 (`ecs/query_iterate/10k` など) は履歴と
結果ページをつなぐキーなので、一度決めたら変えない。

## 計測のしかた

`bench.h` の `nox::bench::State::Run` がやること:

1. 準備運転 (遅延初期化や容量の伸長を計測から外す)
2. 校正 (1 サンプルが 2ms 以上になるまで op 回数を増やす。仮想マシンの
   `QueryPerformanceCounter` は 100ns 単位なので、短すぎる区間は測らない)
3. サンプル 11 回 (1 op あたりの ns と、計測スレッドの CPU サイクル)
4. 確保計数 (固定の 32 op を 2 回。`nox::memory::GetAllocationCounters` の差分)

プロセスは HIGH 優先度にし、計測スレッドを CPU 1 に固定する。

## CI での比較と判定

GitHub のホストランナーは実行ごとに CPU の型番が変わることがあり、同じバイナリでも
2 割ほど差が出る。そこで CI では、**比較元の `bench_test.exe` (master で建てたもの) と
今回のものを同じ VM で交互に 10 回ずつ**走らせ、回ごとの比の中央値と
ブートストラップの 95% 信頼区間で判定する (`.github/scripts/bench-run.py`)。

- 時間: 5% 以上の変化で、信頼区間が 0% をまたがないときに「悪化 / 改善」。
  CI は落とさない (報告だけ)。
- 確保回数: 揺れないので厳密に比べる。`alloc_budget` を設けたベンチで予算を超えると
  `bench_test.exe` が終了コード 3 を返し、CI が落ちる。フレーム中に確保しないはずの経路
  (列挙・Dispatch・delegate 呼び出しなど) は予算 0 にしてある。

確保回数は全スレッドの合計で、`nox::memory::Allocate` を通ったものだけを数える
(CRT の `malloc` 直呼びや DLL 内の確保は数えない)。

## 手元で走らせる

```powershell
# runtime.slnx を Release か Master で建てたあと (テストと同じく、ソリューションとは別に建てる)
msbuild runtime\bench\bench_test.vcxproj /p:Configuration=Release /p:Platform=x64
runtime\build\runtime\x64\Release\bench_test.exe --out bench.json
runtime\build\runtime\x64\Release\bench_test.exe --out bench.json --filter ecs/
```

結果は JSON (`nox-bench-raw/1`) でファイルに書く。標準出力にはエンジンのログが出るので混ぜない。
進捗は標準エラーに出る。

## ベンチを足すとき

1. `bench_*.cpp` に `void BenchXxx(nox::bench::State& state)` を書く。準備 → `state.Run(body)` の順で、
   `body(n)` は「1 op を n 回」実行する。1 op で複数要素を処理するなら `state.SetItemsPerOp(N)`。
2. 同じファイルの `Get*Benchmarks()` の表に 1 行足す。
   - `name` は `グループ/名前[/条件]` の小文字 ASCII
   - `title` は日本語の短い説明 (結果ページにそのまま出る)
   - `per` は 1 op が何か (`entity` / `call` / `dispatch` など)
   - `alloc_budget` はフレーム中に確保してはいけない経路なら `0`、測るだけなら `nox::bench::kNoBudget`
3. 最適化で計算ごと消されないように、結果は `nox::bench::DoNotOptimize` に通す。

## 注意

- プロジェクト名は `_test` で終える。ReflectionGenerator は `runtime.slnx` の全プロジェクトを
  モジュールとみなし、`<名前>.h` を生成コードへ include する。`_test` で終わるものだけが除外される。
- ECS の型はこのプロジェクトの `.cpp` だけで定義し、`core/test/test_types.h` には入れない
  (生成されるテーブルが変わり、テストの件数が狂う)。
- `World` と `JobSystem` は大きい (数百 KB) ので、1 本のベンチにつき 1 つずつにする。
