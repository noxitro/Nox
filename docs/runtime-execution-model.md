# Runtime 実行モデル 設計書

Runtime (C++) の「毎フレーム何がどの順で走るか」と「共有状態を誰が所有するか」を決める文書。
旧 `SystemBase` と UpdaterGraph の 2 本立てになっている実行経路を、UpdaterGraph 1 本へ統合する。

実装は `work/updater-graph-unification` で段階的に進めている。各段階の状態は末尾の「移行の順序」を参照。

## 1. 目的

- 実行の仕組みを **UpdaterGraph 1 本**にまとめる。旧 `SystemBase` の `PhaseRegister` 表と UpdaterGraph が別々に順序を決めていると、両者をまたぐ依存が表現できず、片方の宣言が黙って無効になる (実例: `EditorRemoteServer` が依存を宣言していた `SocketScheduler` のフェーズは、どの表にも登録されておらず読み飛ばされていた)。
- **引数リストがアクセス宣言そのもの**、を全ノードの唯一の規則にする。宣言と実装が型で一致し、UpdaterGraph の衝突判定・コマンドバッファの割り当て・並列実行チェッカーが全部この 1 つの入力から出る。
- 依存解析から見えない処理をなくす。宣言のない共通の `Update()`、ノード同士の直接参照、`World` からの型引き (`GetSystem<T>()`) はいずれも禁止する。
- フレーム中のヒープ確保と間接呼び出しを増やさない (AGENTS.md の最優先ルール)。グラフの構築・ソート・名前解決は起動時に 1 回だけ行う。

## 2. 概念

概念は 4 つだけにする。

| 概念 | 役割 | 寿命 | 毎フレームの処理 |
|---|---|---|---|
| **ComponentData** | エンティティごとのデータ | エンティティに従う | — |
| **Service** | World に 1 つの共有状態 (外部資源、ゲーム全体の状態) | World が所有。`OnInitialize` / `OnShutdown` を private で持つ | 自分のメソッドをノードとして登録できる (§3) |
| **ノード** | UpdaterGraph 上の実行単位 | 構築時に確定 | 引数で宣言したものだけを読み書きする |
| **EntityCommands** | 遅延の構造変更 (生成・破棄・Add・Remove) | フェーズ末に反映 | 引数で受け取る |

「World に 1 つのゲームデータ」を Service と別の概念 (Resource) にはしない。今の Nox は全て World 所有で、所有者の違いがまだ存在しないため。外部資源を持たない Service をそう呼ぶだけにし、Window / Device を World の外へ出す時点で型として分ける。

### Service の規則

- **Update を持たない。** 共通の仮想 `Update()` があると、その中で何を読み書きしているかが依存解析から見えない。毎フレームの処理は §3 のノードとして、引数でアクセスを宣言して書く。
- **Initialize / Shutdown は持つ。** 初期化順は Service 同士の依存 (`Depends`) から自動で決まり、終了は逆順。初期化の失敗は戻り値で返す (例外は使わない)。起動失敗時は初期化済みの分だけ逆順に片付ける。
- 依存は `using Depends = nox::TypeList<...>;` で宣言し、`OnInitialize` は宣言した型だけを `ServiceContext::Get<T>()` で引ける。宣言 = 依存解析の唯一の入力、という規則を Service にも適用する。
- 読む側は `const Service&` しか受け取れない。書き込み権限を宣言したノードだけが非 const の API を呼べる。
- `World::TryGetService` は引数宣言を素通りする唯一の経路なので、起動時・テスト・エンジン内部専用とする。

## 3. ノードの定義は 1 つ、置き場所は 4 つ

```
ノード = 呼び出し可能なもの + 引数で宣言したアクセス
引数   = Service& / const Service& / Query<...> / EntityCommands& / (行単位の形では ComponentData)
```

| 置き場所 | 書き方 | 使いどころ |
|---|---|---|
| **グローバル関数** | `NOX_ATTR(nox::attr::UpdaterTask(phase))` を付けた関数 | 状態を持たない 1 フレーム 1 回の処理 |
| **Service のメソッド** | 属性を付けた private メソッド。自分自身は暗黙に書き込み | 入力のポーリング、ソケット受信など、Service 自身の状態を保つ処理 |
| **EntitySystem** | `OnUpdate(Position&, const Velocity&)`。行単位。ComponentData が 1 個以上必須 | 大量に湧くもの (弾・群れ) |
| **EntityLogic** | 属性を付けたメソッド。ComponentData が揃ったエンティティごとに 1 インスタンス | 少数の主要な個体 (プレイヤー・ボス・UI) |

- 4 つは実行の仕組みが違うのではなく、**同じノードの置き場所が違うだけ**。UpdaterGraph から見れば全て「読み書きを宣言したノード」で、並べ方・並列化・コマンドバッファの割り当ては同じ。
- EntitySystem は「本体が Query ループ 1 つだけのノード」の糖衣。行単位の書き方はそのまま残す。ComponentData を 1 つも取らない EntitySystem は `static_assert` で弾く (空の Query は全 Archetype に一致し、エンティティの数だけ呼ばれてしまう)。
- 他のエンティティを見る処理 (Transform 階層、衝突ペア) は、`Query<...>` と読み取り専用の `ComponentLookup<const T>` を引数に取るノードで書く。
- 例外として、Service のメソッドと Task は `nox::World&` を引数に取れる (排他アクセス)。Editor との橋渡しのような開発ツール用の逃げ道で、ゲームロジックでは使わない (§12)。
- ノード同士の直接参照は禁止。共有は ComponentData か Service を介する。
- 使い分けの目安: Service 自身の状態を保つ処理は Service のメソッドに、Service を使ってエンティティを動かす処理は EntitySystem / EntityLogic に書く。「まず EntityLogic で書き、数が増えたら同じ引数で EntitySystem へ移す」が小さな書き換えで済むよう、両者の引数規則は同じにしてある。

### 層の分け方

Engine 層と App 層で実行の仕組みは分けない。分けると 2 つの経路をまたぐ依存が解析できなくなる (物理とゲームロジックが同じ `Transform` に書く、入力のポーリングとプレイヤー操作が同じ Service を介す、など)。層は次の 3 軸で表す。

| 軸 | Engine 層 | App 層 |
|---|---|---|
| 所有と登録 | Engine のモジュールが Service を登録する | ゲーム側のモジュールが登録する |
| フレーム内の区間 (§4) | 主に FrameIngress と Presentation | 主に Update |
| 依存の向き | App 層の型を知らない (ライブラリの境界で担保) | Engine 層の Service を読み書きできる |

App 層を EntityLogic だけに制限することもしない。大量に湧くものの多くはゲーム固有で、EntityLogic だけだとエンティティごとにインスタンスを持って 1 体ずつ呼ぶことになり、性能最優先の方針とぶつかる。

## 4. 実行順の決め方

UpdaterGraph は、フェーズごとに構築時に 1 回だけグラフを組む。毎フレームはそれを辿るだけ。

1. **衝突判定** (自動): 同じ ComponentData / Service に書き込みが絡めば衝突。read 同士は並列。同じ Service / EntityLogic 型のメソッド同士はインスタンス状態を共有するので必ず衝突。
2. **明示辺**: 型に書いた `using RunAfter = nox::TypeList<...>;` / `using RunBefore = nox::TypeList<...>;`。読み書きの衝突からは導けない因果 (受信の後に処理する、など) を書くためのもので、衝突の有無に関係なく辺として張る。依存先は型で参照するので、名前の打ち間違いはコンパイルエラーになる。`RunBefore` があるので、上の層が下の層に手を入れずに「この前に走る」と書ける。
3. **全順序**: 明示辺のトポロジカル順。決まらない箇所は**完全修飾型名 (EntityLogic はさらにメソッド名) のソート順**で決める。登録順 (= リフレクション生成器の走査順 ≒ ファイル順) は一切使わない。ビルドやツールセットをまたいでも同じ順序になる。
4. **辺**: 衝突した組は全順序の前→後へ張る。明示辺は衝突がなくても張る。全ての辺が同じ向きなので DAG に循環は生まれず、レイヤリングは前方 1 走査で閉じる。
5. **起動失敗にするもの**: 存在しないノードへの明示辺、明示辺同士の循環、相手の型が同じフェーズにノードを持たない宣言 (辺が 1 本も張れず黙って無効になる)。いずれも宣言 (コード) の誤りなので、Master 構成でも理由をログに残して abort する。`NOX_ASSERT` は診断補助であり、成否の判定には使わない。
6. **可視化**: 明示辺がなく型名順だけで直列化の向きが決まった write/write・write/read の衝突組は、Master 以外で起動ログに一覧で出す (`BY-NAME`)。因果のある組なら `RunAfter` / `RunBefore` で向きを宣言すべき候補になる。

自動の依存解決が答えるのは「同時に走らせてよいか」だけで、「どちらが先か」は決めない。明示辺はその後者に答える。2 つは補い合う関係で競合しない。

### メインスレッド限定

OS のメッセージを読む・GPU へ提出するなど、ワーカーへ流せない処理は `static constexpr bool kMainThreadOnly = true;` で宣言する (EntitySystem / EntityLogic の型。Service のメソッドと Task は属性の `nox::attr::ThreadAffinity::MainThread` でメソッド単位に宣言する)。同一レイヤー内で該当ノードだけを呼び出しスレッドで回し、それ以外をワーカーへ配る。`k_parallel_for_each` (Chunk 並列) とは両立しないので `static_assert` で弾く。

## 5. フェーズ

最初は 4 つ。増やすのは「順序の barrier」か「構造変更を可視化する barrier」が要るときだけ。

| フェーズ | 中身 |
|---|---|
| FrameIngress | 入力ポーリング、ソケット受信、アセット完了の取り込み (主にメインスレッド) |
| Update | ゲームロジック |
| Presentation | 描画の抽出・提出 |
| FrameEnd | 外部への送信 (snapshot 後) |

- 各フェーズの末尾で EntityCommands を反映する。Playback 順は「ノード外バッファ → 全順序の昇順」で固定なので、どのワーカーがどのノードを先に走らせたかに依存しない。
- `Init` / `Terminate` はフェーズではなく、Service の寿命 (§6) と、開始・終了時に 1 回走るノードに分ける。
- 固定 tick は最初は入れない。必要になったら FrameIngress と Update の間に Fixed 系のフェーズを挿す。
- 実装済みは FrameIngress (手順 3a)。Presentation / FrameEnd はまだ無い。

## 6. 寿命 (ServiceGraph)

- Service の依存は `Depends` で宣言する。宣言から初期化順が決まり、終了は逆順。
- `OnInitialize` は失敗を戻り値で返す。
- 起動失敗時は初期化済みの分だけ逆順で片付けてから abort する。
- 未登録の型への `Depends`、循環は起動失敗。

Service の寿命と UpdaterGraph の実行順は別々の問いに答える別々のグラフで、互いに干渉しない。

| | 寿命の依存 | フレーム内の実行順 |
|---|---|---|
| 問い | どの Service を先に初期化し、後に破棄するか | このフレームでどの処理を先に走らせるか |
| 対象 | Service | ノード |
| いつ解く | 起動時と終了時に 1 回 | 起動時にグラフを組み、毎フレームそれを辿る |
| 仕組み | ServiceGraph | UpdaterGraph |

## 7. 今のクラスの行き先

| 今 | 行き先 |
|---|---|
| `SystemBase` / `PhaseRegister` / `system_phase_table_` / `BuildExecuteNodeList` | **削除** |
| `AssetManager : SystemBase` | Service (手順 3b で移行済み)。ノードは持たない。ロードは専用のロードスレッドで完結し、完了はロードスレッドが `Asset` の状態 (`IsReady`) へ直接書く。完了をフレームへ取り込むメソッド (FrameIngress) は、ゲームスレッド側で完了を受け取る処理が要る時点で足す。開発ビルドでは `EditorRemoteServer` に `Depends` でつながる |
| `SceneManager : SystemBase` | Service (手順 3b で移行済み)。メインウィンドウを持つだけでノードは持たない。`--exit-after-frames` の終了要求は World へ移した。シーンの ECS データはまだ無い |
| `GarbageCollector : SystemBase` | Service (Object 基盤用。ECS の寿命とは分離) |
| `SocketScheduler : SystemBase` | Service (手順 3a で移行済み)。受信は専用スレッドで回し、ノードは持たない |
| `EditorRemoteServer : SystemBase` | Service + クエリを実行する FrameIngress の排他ノード (手順 3a で移行済み)。`SocketScheduler` とは `Depends` (寿命) だけでつながる |
| `Renderer` / `DebugDraw : SystemBase` | Service + 抽出・提出のメソッド (Presentation)。Init の順序は `Depends` で自動 |
| `EngineModule::CreateEngineSystems` | Service の登録だけを行う `RegisterServices` (手順 3b の時点で、Core の `CreateEngineSystems` に残るのは `GarbageCollector` だけ) |

`EditorRemoteServer` と `SocketScheduler` の間にあった明示依存 3 件のうち、Init と Terminate の順序の 2 件は `Depends` (初期化順とその逆順の終了) に置き換わった。残る 1 件 (Update の順序) は、依存先が専用スレッドの受信ループでフェーズではなく、もともと黙って無効になっていたもの。受信データは受信スレッドから `EditorRemoteServer` の受信バッファ (mutex で保護) 越しに受け渡され、FrameIngress の排他ノードが取り出す。フレーム内の順序としての依存は残らない。

## 8. 移行の順序

| 手順 | 内容 | 状態 |
|---|---|---|
| 1 | UpdaterGraph の全順序を「明示辺 + 型名順」にする。`RunAfter` / `RunBefore`。見つからない依存・循環は起動失敗。旧 `BuildExecuteNodeList` の読み飛ばしも起動失敗に | 完了 (CI 全構成 緑) |
| 2a | Service の寿命 (`OnInitialize` / `OnShutdown`、`Depends`、失敗時の逆順ロールバック)。`EngineModule::RegisterServices`。ノードのメインスレッド限定実行 | 完了 |
| 2b | Service の属性付きメソッドとグローバル関数をノードとして登録 (リフレクション生成器の変更あり) | 完了 (CI 全構成 緑) |
| 3a | フェーズに FrameIngress を足す。`SocketScheduler` → `EditorRemoteServer` を Service へ移す。ノードの排他アクセス (`nox::World&`) | 完了 (CI 全構成 緑) |
| 3b | `AssetManager` / `SceneManager` を Service へ移す。`--exit-after-frames` を World へ移す | 実装中 |
| 4 | `Renderer` / `DebugDraw` / `GarbageCollector` を移す。フェーズに Presentation を足す | 未着手 |
| 5 | `SystemBase` と旧フェーズ表を削除 | 未着手 |
| 6 | `Query<...>` / `ComponentLookup<const T>` を引数の種類として足す | Transform 階層が要る時点で |

## 9. 決めておく必要がある点

- write/write の衝突に明示辺がないとき、ログで可視化にとどめるか (現状)、起動失敗にするか。
- `World::TryGetService` のアクセス制限を、いつ・どの形で入れるか (現状は Doxygen での注記のみ)。
- 固定 tick を入れる時期。

## 10. 手順 1 で決まったこと (実装済みの仕様)

- 記述子 (`EntitySystemTypeDescriptor` / `EntityLogicTypeDescriptor`) は定数初期化を維持する。`RunAfter` / `RunBefore` は `requires` で検出し、完全修飾型名の `std::span<const std::string_view>` として載せる。生成器の変更は不要。
- `nox::TypeList` は `entity_access.h` に置く (System と Logic の両方が読む)。
- `RunAfter` / `RunBefore` は public に書く。private に書いたエイリアスは `requires` から見えず、宣言がないのと同じになる。
- EntityLogic の `RunAfter` は型単位で、その型の同じフェーズの全メソッドに掛かる。同じ型のメソッド同士はメソッド名順。
- `command_buffer_index` は全順序の昇順で振る。
- `SocketScheduler` のソケット受信ループは `Initialize` が起こす専用スレッド上で `World::IsKill()` まで回り続ける処理であり、フェーズではない。`PhaseRegister` に載せてはならず、他からこのフェーズへの依存も宣言できない。(手順 3a で `SocketScheduler` は Service になり、受信ループの停止は自身の停止フラグになった。§12)
- テストは abort せず結果を返す `UpdaterGraph::TryRebuild` を使う。起動経路は `Rebuild`。
- 既存の EntitySystem / EntityLogic の直列化順は登録順から型名順に変わる。データの流れと逆向きになった組は起動ログの `BY-NAME` に出るので、必要なら `RunAfter` を宣言する。

## 11. 手順 2 で決まったこと (実装済みの仕様)

### 2a: Service の寿命とメインスレッド限定

- `OnInitialize(nox::ServiceContext&)` / `OnShutdown()` は private な仮想関数 (NVI) で、呼ぶのは World だけ。派生型は friend を書かない。
- 初期化は `Depends` のトポロジカル順、決まらない箇所は完全修飾型名順 (登録順は使わない)。終了は初期化の逆順。`OnInitialize` が false を返したら、それまでに初期化した分だけを逆順に終了して起動失敗。
- `Depends` は public な `using Depends = nox::TypeList<...>;`。private に書くと見えず、宣言が無いのと同じになる。未登録の型・循環・同じ型の二重登録は、どの `OnInitialize` よりも前に起動失敗。
- `ServiceContext::Get<T>()` は `Depends` に並べた型しか引けない。宣言外の型は nullptr を返し、`OnInitialize` が true を返しても起動失敗 (`UndeclaredServiceAccess`)。`ServiceContext` は World への参照を持たない。
- `kMainThreadOnly` は EntitySystem / EntityLogic の型に public に書く。そのノード (EntityLogic は型の全メソッド) をワーカーへ配らず、フェーズを回しているスレッドで実行する。依存解析 (レイヤー) には影響しない。`k_parallel_for_each` とは両立しない (`static_assert`)。

### 2b: Service のメソッドと Task

- **Service のメソッド**: `NOX_ATTR(nox::attr::ServiceMethod(phase[, nox::attr::ThreadAffinity]))` を `nox::Service` 派生型のメソッドに付ける (private のままでよい)。実行スレッドはメソッド単位で、既定は `Any`。Service の型に `kMainThreadOnly` を書くと `static_assert` で弾く (黙って無視しない)。
- **Task**: `NOX_ATTR(nox::attr::UpdaterTask(phase[, nox::attr::ThreadAffinity]))` を名前空間スコープの関数に付ける。無名名前空間・関数テンプレート・メンバ関数への付与は生成器がエラーにする。全順序のキーは完全修飾関数名。
- **シグネチャの規則 (両者共通)**: 戻り値は void。引数に取れるのは Service (参照 / ポインタ、const なら読み取り) と `nox::EntityCommands&` だけで、ComponentData・`nox::EntityId` は `static_assert` で弾く。entity を列挙せず、フェーズごとにちょうど 1 回呼ばれる (entity が 0 個でも呼ばれる)。参照で受ける Service が未登録なら呼び出しを打ち切り、ポインタなら nullptr を渡す (EntitySystem / EntityLogic と同じ)。
- **自己書き込みの暗黙付与**: Service のメソッドは、自分自身の Service への書き込みを宣言の末尾に足す。引数に書かなくても、その Service を読み書きする他のノードとは直列化され、並列実行チェッカーも宣言外の同時アクセスを拾える。同じ Service のメソッド同士はインスタンス状態を共有するので group で必ず衝突させ、メソッド名順に直列化する (起動ログの衝突理由は `service-state`。`BY-NAME` の候補には出さない)。
- **明示辺**: Service の型にも `RunAfter` / `RunBefore` を書ける。その型の同じフェーズの全メソッドに掛かる。並べた Service が属性付きメソッドを持たないか World に登録されていなければ `UnresolvedOrderTarget` で起動失敗。
- **Task に `RunAfter` / `RunBefore` が無い理由**: Task は型ではないので宣言を書く場所が無く、他のノードの `nox::TypeList` に並べることもできない。関数を型として扱う仕組みを足すより、順序が要る処理は状態の置き場所である Service のメソッドに書く方が、宣言が 1 か所で済む。Task は状態を持たないので group も無く、衝突は引数の宣言だけで決まる。
- **記述子の置き場所**: メソッド表 (`nox::ServiceMethodTable<T>`) と型ごとの記述子 (`nox::ServiceMethodTypeDescriptor`) は、寿命の記述子 (`nox::ServiceTypeDescriptor`) と別に持つ。メソッド表の特殊化は生成コードの翻訳単位にしか見えないため、登録側 (`MakeServiceTypeDescriptor`) から読むと翻訳単位ごとに別の中身の実体ができる。World は生成コードの表 `nox::GetServiceMethodTypes()` を型情報で登録済みの Service と照合し、登録されている型だけをノードにする。Task は `nox::GetUpdaterTaskDescriptors()` の表をそのまま渡す。
- **EntitySystem**: `OnUpdate` は ComponentData を 1 つ以上取る (`static_assert`)。空の Query は全 Archetype に一致し、entity の数だけ呼ばれてしまうため。1 フェーズに 1 回の処理は Service のメソッドか Task にする。

## 12. 手順 3 で決まったこと (実装済みの仕様)

### 3a: FrameIngress / SocketScheduler / EditorRemoteServer / 排他アクセス

- **FrameIngress の位置**: `SystemPhaseType` の `Start` と `Update` の間。`World::Update` が毎フレーム `ExecutePhase(FrameIngress)` → `ExecutePhase(Update)` の順に呼ぶ。`ExecutePhase` は旧表 → UpdaterGraph → `FlushEntityCommands` の順なので、FrameIngress で積んだ構造変更は Update から見える。VSync の待ちでフレームを飛ばすときは両方とも走らない。
- **SocketScheduler**: Service になり、`OnInitialize` で WinSock を初期化して受信スレッドを起こし、`OnShutdown` で止めて join する。受信ループの停止は `World::IsKill()` ではなく自身の停止フラグ (`OnShutdown` が立てる) で行う。`OnShutdown` は `World::Exit` から呼ばれるので、ゲームスレッドが止まった後に受信スレッドが止まる。WinSock の初期化に失敗しても起動は止めない (受信スレッドを起こさないだけ)。ノードは持たない。
- **受信スレッドは World に触れない**: Service は World への参照を持たないので、`Server::Update` と `IServerEventHandler::OnServerReceive` から World 引数を外した。以前は受信スレッド上でもクエリを実行していたが (ゲームスレッドとの競合があった)、受信スレッドは受信バッファに積むだけにし、クエリの実行は FrameIngress の排他ノードだけが行う。Editor からのクエリは最大 1 フレーム遅れて処理される。
- **登録の解除は同期**: `SocketScheduler::UnregisterEntity` は受信スレッドが外し終えるまで待ってから戻る。反映は受信ループの周回の頭でだけ行うので、戻った後に受信スレッドがその Server に触れることはない。`EditorRemoteServer` は `Depends` の逆順で `SocketScheduler` より先に終了するので、受信スレッドが動いている間に Server を閉じても競合しない。
- **EditorRemoteServer**: Service になり、`using Depends = nox::TypeList<nox::dev::net::SocketScheduler>;` で初期化・終了の順序を宣言する。`OnInitialize` で待ち受けを始めて受信スレッドに登録し、`OnShutdown` で登録を外して閉じる。待ち受けの失敗では起動を止めない (Editor と繋がらないだけ)。クエリの実行は `NOX_ATTR(nox::attr::ServiceMethod(nox::SystemPhaseType::FrameIngress))` を付けた `UpdateReceive(nox::World&)`。
- **エンジン内部からの参照**: クエリの実装 (`Query::Execute`) と、まだ `SystemBase` の `AssetManager` は引数で受け取れないので、`World::TryGetService` で `EditorRemoteServer` を引く (起動時・テスト・エンジン内部専用の経路)。見つからなければ `NOX_ASSERT` し、失敗のレスポンスを返す。(手順 3b で `AssetManager` は Service になり、`Depends` + `ServiceContext::Get` に置き換わった。クエリの実装は `SceneManager` も同じ形で引く)
- **即時系の構造変更**: クエリの実装は即時系 (`CreateEntity` など) を呼んでいない。ゲームスレッドの旧 Update フェーズで実行していた経路は、フェーズ実行中の制約が今と同じだった (受信スレッド上の経路はフェーズと無関係に走っていたが、上のとおり外した)。

### 排他アクセス (`nox::World&`)

- **規則**: Service のメソッドと Task は引数に `nox::World&` を 1 つ取れる (`EntityParameterKind::World`)。`const nox::World&` は取れない (読むだけでも World 全体に触れる点は同じで、区別しても並列化に使えない)。EntitySystem / EntityLogic の行単位メソッドに書くと `static_assert` で弾く。
- **衝突**: 取ったノードは `UpdaterNodeAccess::exclusive` を持ち、同じフェーズの全ノードと衝突する。全順序での位置は他のノードと同じ規則 (明示辺 → 型名順) で決まり、その位置で単独のレイヤーになる (前の全ノードの後、後の全ノードの前)。
- **実行スレッド**: 暗黙にメインスレッド限定 (フェーズを回しているスレッド)。属性の `ThreadAffinity` が `Any` でも立つ。
- **宣言の扱い**: World は Service のアクセス宣言に算入しない。Service のメソッドの自己書き込みはそのまま付く。起動ログのノード行には `exclusive` が付き、衝突理由は `exclusive` と出る。`BY-NAME` の候補には、排他だけによる衝突を出さない (出すとフェーズの全ノードとの組が並び、他の候補が埋もれる)。
- **即時系**: 排他ノードもフェーズ実行中に走るので、中から即時系の構造変更は呼べない (`DeniedDuringPhase`)。構造変更は `EntityCommands&` を使う。
- **用途の限定**: 引数でアクセスを宣言する規則 (§3) に対する開発ツール用の逃げ道で、Editor との橋渡しのように World を丸ごと触ることが本質の処理だけに使う。ゲームロジックでは使わない。何を読み書きしているかが依存解析から見えなくなり、そのノードの前後でフェーズの並列性を断ち切るため。

### 3b: SceneManager / AssetManager / --exit-after-frames

- **SceneManager**: Service になり、ノードを持たない。`OnInitialize` でメインウィンドウを作って (studio mode でなければ) 表示し、`OnShutdown` で破棄する。ウィンドウを作れなければ `OnInitialize` が false を返して起動を止める (ウィンドウが無いと、ユーザーの操作でも `--exit-after-frames` でも閉じる経路で終了できない)。`Depends` は無い。
- **SceneManager が Update を持たない理由**: 旧 Update フェーズがしていたのは `--exit-after-frames` の終了要求だけで、これは SceneManager の状態ではなく、World のフレーム数とコマンドラインで決まる。Service のメソッドにすると、フレーム数を読むために World への排他アクセス (§12 の逃げ道) か、フレーム数を持つ別の Service が要る。どちらも「フレームを数えている World が 1 か所で判定する」より宣言が増えるだけなので、判定は World に置き、SceneManager は閉じる操作 (`RequestCloseMainWindow`) だけを公開する。
- **`--exit-after-frames` の位置**: `World::Update` が Update フェーズの後・フレーム数を数える前に判定し (`nox::ShouldRequestExitAfterFrames`)、`TryGetService` で引いた SceneManager の `RequestCloseMainWindow` を 1 回だけ呼ぶ。旧 `SceneManager::Update` (Update フェーズ) と同じ位置なので、N 回目の呼び出しでフレーム数が N - 1 になる数え方は変わらない。VSync の待ちで飛ばすフレームでは判定しない (旧 Update フェーズも走らなかった)。要求を出し終えたかのフラグは World の状態。閉じる経路 (WM_CLOSE → WM_DESTROY → WM_QUIT) は変えていない。
- **studio mode の判定**: `--studio` の判定を純粋関数 `nox::ResolveStudioMode(引数列)` に切り出し、World (`IsStudioMode`) と SceneManager の両方がコマンドラインの引数列から呼ぶ。Service は World への参照を持たないので、World が読んだ値を Service へ渡す経路は作らない。同じ入力に同じ関数を当てるので、両者の判定は食い違わない。
- **ウィンドウを作るスレッド**: `OnInitialize` は `World::Init` の中、UI スレッドのメッセージループに入る前に呼ばれる。ウィンドウ生成の受け渡し (`nox::os::detail::DispatchCreateNativeWindow`) は UI スレッドの `nox::os::Update` が要求を拾うまで待つ作りだったので、UI スレッド自身から呼ばれたときはその場で実行するようにした。作るスレッドは以前と同じ UI スレッドで、メッセージを処理するループも変わらない。
- **AssetManager**: Service になり、ノードを持たない。`OnInitialize` でアセット型の表を作ってロードスレッドを起こし、`OnShutdown` でロードスレッドを止めて join し、生成したアセットを破棄する。ロードスレッドは World に触れず (未使用だった World 引数を外した)、停止は自身の停止フラグで行う。
- **AssetManager の依存**: 開発ビルドでは `CreateAsset` がコンバート要求を Editor へ送るので、`using Depends = nox::TypeList<EditorRemoteServer>;` を宣言し、`ServiceContext::Get` で受け取る (引けなければ起動失敗)。`EditorRemoteServer` は開発ビルドにしか無いので、宣言と `Get` を `#if NOX_DEVELOP` で囲む。Master では依存が無い。
- **完了の取り込み**: ロードスレッドが `Asset::Initialize` を呼び、完了は `Asset` の状態 (`IsReady`) へロードスレッドが直接書く。フレームへ取り込むノードはまだ無い。§7 の「完了取り込みのメソッド (FrameIngress)」は、ゲームスレッド側で完了を受け取る処理が要る時点で足す。
- **終了の時点**: SceneManager と AssetManager の終了は、ゲームスレッドの Terminate フェーズから `World::Exit` (ゲームスレッドが止まった後、UI スレッド) の Service の終了に移った。まだ `SystemBase` の `GarbageCollector` / `Renderer` / `DebugDraw` の Terminate フェーズより後になる。
- **`CreateEngineSystems`**: Core に残るのは `GarbageCollector` だけ。`Renderer` / `DebugDraw` は描画モジュールが登録する (手順 4)。
