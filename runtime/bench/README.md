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
| `container` | `nox::Vector` / `StackAllocVector` / `FixedVector` | 確保 0 回の書き方がいくら得か。参照越しの vector へ積むと MSVC で遅くなる落とし穴も並べてある |
| `string` / `delegate` / `math` / `lock` | 書式化、CRC32、UTF 変換、delegate 呼び出し、Vec3、SRWLOCK | 基盤部品 |

一覧は `bench_test.exe --list` で出る。ベンチの名前 (`ecs/query_iterate/10k` など) は履歴と
結果ページをつなぐキーなので、一度決めたら変えない。

## 起動 (runtime.exe)

`bench_test.exe` とは別に、`runtime.exe` の起動の速さも同じ仕組みで比べる (グループ `startup`)。
起動は 1 回ごとにプロセスを作り直すので、`bench_test.exe` の中ではなく `bench-run.py` が外から起動する。

- `runtime.exe --startup-report=<パス>` を渡すと、起動の区切りごとに `QueryPerformanceCounter` と
  ヒープ確保の累積値を控え、終了前に JSON (`nox-startup/1`) を書く (`runtime/core/startup_profile.h`)。
  区切りは EntryPoint → memory / reflection / os の初期化 → `World::Init` → Init フェーズ → Start フェーズ →
  最初のフレーム。EntryPoint より前 (OS のローダ・DLL・静的初期化) は、プロセスの作成時刻
  (`GetProcessTimes`) との差で測る。記録は固定長の配列へ書くだけで、Master を含む全構成で有効
- `bench-run.py --startup-exe runtime.exe` が、各ラウンドで `bench_test.exe` の後に
  `runtime.exe --exit-after-frames=1` を 3 回起動する (`--startup-launches`)。1 回の起動が 1 サンプル。
  最初の冷えた起動 (ディスクのキャッシュに載っていない) は head / base とも 1 回捨てる
- 区間の定義と表示名は `.github/scripts/bench_common.py` の `STARTUP_INTERVALS`。
  見どころは `startup/reflection_init` と `startup/world_init` (エンジン自身の初期化)。
  `startup/pre_main` と `startup/init_phase` は OS とウィンドウ・デバイスの作成が大半で揺れが大きい。
  CI のランナーには GPU が無いので、デバイスまわりは実機と違う経路を通る
- 区間ごとの確保回数も載る。予算は `STARTUP_ALLOC_BUDGETS` に書く (超えたら CI が落ちる)。実測を見てから決める
- 起動は OS の仕事を含む壁時計なので、結果ページとサマリーの「全体の変化 (幾何平均)」には入れない

手元で起動だけ見るとき:

```powershell
runtime\build\runtime\x64\Release\runtime.exe --exit-after-frames=1 --startup-report=startup.json
```

## 計測のしかた

`bench.h` の `nox::bench::State::Run` がやること:

1. 準備運転 (遅延初期化や容量の伸長を計測から外す)
2. 校正 (1 サンプルが 2ms 以上になるまで op 回数を増やす。仮想マシンの
   `QueryPerformanceCounter` は 100ns 単位なので、短すぎる区間は測らない)
3. サンプル 11 回 (1 op あたりの ns と、計測スレッドの CPU サイクル)
4. 確保計数 (固定の 32 op を 2 回。`nox::memory::GetAllocationCounters` の差分)

プロセスは HIGH 優先度にし、計測スレッドを CPU 1 に固定する (`--no-pin` で外せる)。
ラムダの本体はインライン化させない関数越しに呼ぶ。Run の中へ展開されるかどうかで
同じ処理のコード生成が変わるのを防ぐため。

## CI での比較と判定

GitHub のホストランナーは実行ごとに CPU の型番が変わることがあり、同じバイナリでも
2 割ほど差が出る。そこで CI では、**比較元の `bench_test.exe` (master で建てたもの) と
今回のものを同じ VM で交互に 10 回ずつ**走らせ、回ごとの比の中央値と
ブートストラップの 95% 信頼区間で判定する (`.github/scripts/bench-run.py`)。

- 時間: 5% 以上の変化で、信頼区間が 0% をまたがないときに「悪化 / 改善」。
  信頼区間がまるごと ±5% に収まったときだけ「変化なし」とし、それ以外は「ばらつき大」
  (測り直さないと言えない) にする。複数スレッドで働くベンチはしきい値を 2 倍にする。
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
