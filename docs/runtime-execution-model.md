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

OS のメッセージを読む・GPU へ提出するなど、ワーカーへ流せない処理は `static constexpr bool kMainThreadOnly = true;` で宣言する。同一レイヤー内で該当ノードだけを呼び出しスレッドで回し、それ以外をワーカーへ配る。`k_parallel_for_each` (Chunk 並列) とは両立しないので `static_assert` で弾く。

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
| `AssetManager : SystemBase` | Service + 完了取り込みのメソッド (FrameIngress) |
| `SceneManager : SystemBase` | Service + シーンの ECS データ |
| `GarbageCollector : SystemBase` | Service (Object 基盤用。ECS の寿命とは分離) |
| `SocketScheduler : SystemBase` | Service + 受信メソッド (FrameIngress) |
| `EditorRemoteServer : SystemBase` | Service + ingress / egress のメソッド。`SocketScheduler` の後は `RunAfter` で明示 |
| `Renderer` / `DebugDraw : SystemBase` | Service + 抽出・提出のメソッド (Presentation)。Init の順序は `Depends` で自動 |
| `EngineModule::CreateEngineSystems` | Service の登録だけを行う `RegisterServices` |

今ある明示依存 3 件のうち 2 件 (Init の順序) は `Depends` で自動化され、フレーム内の順序として残るのは `EditorRemoteServer → SocketScheduler` の 1 件だけ。

## 8. 移行の順序

| 手順 | 内容 | 状態 |
|---|---|---|
| 1 | UpdaterGraph の全順序を「明示辺 + 型名順」にする。`RunAfter` / `RunBefore`。見つからない依存・循環は起動失敗。旧 `BuildExecuteNodeList` の読み飛ばしも起動失敗に | 完了 (CI 全構成 緑) |
| 2a | Service の寿命 (`OnInitialize` / `OnShutdown`、`Depends`、失敗時の逆順ロールバック)。`EngineModule::RegisterServices`。ノードのメインスレッド限定実行 | 実装中 |
| 2b | Service の属性付きメソッドとグローバル関数をノードとして登録 (リフレクション生成器の変更あり) | 未着手 |
| 3 | `AssetManager` → `SocketScheduler` → `SceneManager` → `EditorRemoteServer` の順で Service へ移す。フェーズに FrameIngress を足す | 未着手 |
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
- `SocketScheduler` のソケット受信ループは `Initialize` が起こす専用スレッド上で `World::IsKill()` まで回り続ける処理であり、フェーズではない。`PhaseRegister` に載せてはならず、他からこのフェーズへの依存も宣言できない。
- テストは abort せず結果を返す `UpdaterGraph::TryRebuild` を使う。起動経路は `Rebuild`。
- 既存の EntitySystem / EntityLogic の直列化順は登録順から型名順に変わる。データの流れと逆向きになった組は起動ログの `BY-NAME` に出るので、必要なら `RunAfter` を宣言する。
