# Runtime ECS の書き方 (名前空間と EntitySystem)

利用者が書く ECS の型 (Component / Service / EntitySystem / EntityLogic) の置き場所と名前、EntitySystem の書き方を決める文書。決めた理由と採らなかった案も残す。

- フレームの中の実行順 (UpdaterGraph) の決め方は、`work/updater-graph-unification` の実行モデルの設計書 (`docs/runtime-execution-model.md`、master へは未取り込み) が扱う。この文書はその規則 (引数の並びがアクセスの宣言、など) を前提にする。
- ただし EntitySystem のオプションの書き方は、この文書で置き換える。そのブランチではクラスの中に public の `using RunAfter = nox::TypeList<...>;` / `using RunBefore` を書くが、この文書では基底のテンプレート引数に並べる (§2)。Service の `RunAfter` / `RunBefore` はこの文書の対象外。
- master のコードは、まだこの文書の形に揃っていない。`runtime/core/entity_system.h` には `nox::EntitySystem<T, Options...>` の宣言と印の型 (`nox::RequireComponents` / `nox::RunAfter` / `nox::RunBefore`) があるが、Doxygen の説明は位置で決まる旧い形 (ExtraRequiredComponents / AfterSystems / BeforeSystems) のまま。オプションの解析、private への格納、private メソッドの検出は未実装で、実際に使われている System は `nox::legacy::EntitySystem`。実装を合わせるときは、この文書を正とする。
- 決まっていない点は §8 にまとめる。

## 1. 名前空間と型の名前

名前空間は「モジュール → 種類」の順にする。core モジュールは `nox` 直下に置く。

| 種類 | 名前空間 | 型の名前 | 例 |
|---|---|---|---|
| Component | `<モジュール>::components` | 名詞のまま | `nox::components::LocalPosition` |
| Service | `<モジュール>::services` | 末尾に `Service` | `nox::services::TimeService` |
| EntitySystem | `<モジュール>::systems` | 末尾に `System` | `nox::systems::MoveSystem` |
| EntityLogic | `<モジュール>::entity_logics` | 末尾に `Logic` | `nox::entity_logics::LoggingLogic` |

ほかのモジュールは `nox::render::components::MeshRenderer`、開発用は `nox::dev::components::Name`、ゲーム側は `game::components::Health` のように置く。

### 理由

- **Component の名前はぶつかりやすい。** `Position` / `Rotation` / `Name` / `Transform` のような一般的な名詞になりやすい。実際に `nox::Position` は算術の型として既にある。
- **`nox::` の補完を汚さない。** System と Logic はエンジンが大きくなると数百になる。種類ごとに分ければ、`nox::components::` と打ったときの候補が Component の一覧になる。
- **型の名前に種類を残す。** 型名は名前空間なしで表示される場面が多い (起動ログの実行グラフ、プロファイラ、Editor の依存グラフ、エラーメッセージ、`using namespace` した後のコード)。`Move` だけでは System か Logic か分からない。`nox::systems::MoveSystem` の重複は許容する。
- **順番を「モジュール → 種類」にする。** 名前空間の持ち主がそのままモジュールを表し、既存の `nox::render` / `nox::dev` と揃う。

### 名前空間は保存データの一部になる

保存と Editor の通信では、型を完全修飾名で指す。Component の名前に加えて、EntityLogic の名前もシーンに残る (EntityLogic はタグとして entity に付くため)。

- 名前空間と型名は、原則として変えない。旧名を残して読み替える属性 (まだ無い) ができるまでは、保存データに載った後には変えられない。
- EntityLogic を EntitySystem へ移し替えるときは、シーンに残ったロジックのタグの扱いも手順に含める。

### 置かない場所

- 無名名前空間: ReflectionGenerator が型名を書けない。
- `nox::detail`: 内部の実装専用。利用者が書く型は置かない。

## 2. EntitySystem の形

```cpp
namespace nox::systems
{
	class MoveSystem final : public nox::EntitySystem<MoveSystem,
		nox::RunAfter<InputSystem>,
		nox::RequireComponents<nox::components::Movable>>
	{
		NOX_ECS_DECLARE_VERIFY(MoveSystem);
	private:
		void OnUpdate(nox::components::LocalPosition& position, const nox::components::Velocity& velocity);
	};
}
```

### メソッドは決まった名前

- 定義できるのは `OnUpdate` / `OnAdd` / `OnRemove` の 3 つ。登録の一覧 (`RegisterList`) は書かない。
- 1 つも定義していなければコンパイルエラーにする (名前の書き間違いで、黙って何もしない System になるのを防ぐ)。この検査は、登録する翻訳単位でも行う。`NOX_ECS_DECLARE_VERIFY` は書かなくてもよいマクロなので、マクロ経由の検査だけに頼ると、書き忘れたときに検査が走らない。
- 3 つのアクセス指定は揃える。private にするなら 3 つとも private。private のメソッドは、`NOX_ECS_DECLARE_VERIFY` が friend として宣言する登録用の入口から検出する。マクロを書き忘れると private のメソッドが見えなくなるが、揃えておけば「全部見えない」として上の検査でエラーになる。
- 同じ名前のオーバーロードは書かない。メンバ関数のアドレスを取る式で存在を調べると、オーバーロードされた名前は「無い」と判定され、黙って無視される。検出の実装は、名前の存在と曖昧さを区別して `static_assert` で弾くこと。
- 1 つの System は 1 つの関心事だけを持つ。別の処理を足したくなったら System を分ける。

### オプションは基底のテンプレート引数

- `nox::EntitySystem<T, Options...>` の `Options` に、種類の印が付いた型を順番自由で並べる。
- 同じ種類を 2 回書いたら、つないで 1 つにする (`nox::RunAfter<A>, nox::RunAfter<B>` は `A` と `B` の両方の後)。
- 基底の並びはクラスのスコープの外なので、名前空間まで書く。System の定義を `namespace nox::systems { ... }` の中に書けば、同じ名前空間の相手は短く書ける。
- 相手の型は前方宣言でよい (名前を書くだけで、型の中身は使わない)。互いに `RunAfter` / `RunBefore` で参照し合う 2 つの System も書ける。

### 基底はオプションを持つだけ

基底の `nox::EntitySystem<T, Options...>` は、ヘッダを読む全ての翻訳単位で実体化される。

- 基底はオプションを型の別名として持つだけにする。派生クラスの補完に出ないよう private に置き、登録用の入口から読む。派生クラスに書く friend 宣言は基底の private に届かないので、基底の側でも登録用の入口を friend にする。
- 基底で行う検査は、型特性だけで済む軽いものに限る (未知のオプションが無いか、メソッドが 1 つ以上あるか)。
- オプションの解析 (種類ごとの配列への振り分け、型情報の取得) は、登録する翻訳単位でだけ行う。
- 登録する翻訳単位では、オプションに書いた相手の型のヘッダも読み、完全型にしておく。型情報 (`nox::reflection::Typeof<T>()`) は不完全型だとサイズ 0 として作られるため、翻訳単位によって完全型と不完全型が混ざると、同じ定数の中身が食い違い ODR 違反になる。

## 3. 引数とオプションの分け方

規則は 1 つだけ。**メソッドの中で使うもの (読み書きするもの) は引数、使わないものはオプション。**

| 分類 | 何を書くか | 置き場所 |
|---|---|---|
| 使うもの | Component、Service、他の entity の参照、構造変更のバッファ | 引数 |
| 使わないもの | 対象の絞り込み、実行の順序 | オプション |

使うものを引数にすると、受け取ったものしか触れないことが型で保証される。オプションで宣言だけして実物を別の経路 (World から型で引くなど) で取ると、宣言と実際の使い方がずれても気づけない。

## 4. 引数の種類

| 引数 | 渡し方 | 衝突の判定 |
|---|---|---|
| Component の参照 (`T&` / `const T&`) | 行ごと (列の先頭 + 行) | const なら読み取り、そうでなければ書き込み |
| `nox::Entity` | 行ごと (Entity の列) | 対象外 |
| Service の参照 / ポインタ | 全行で同じもの | const なら読み取り。参照で受け取るのに World に無ければ呼び出しを打ち切り、ポインタなら nullptr を渡す |
| `nox::EntityCommands&` | 全行で同じもの (このノード専用のバッファ) | 衝突の原因にしない (§6) |
| `nox::ComponentLookup<const T>` | 全行で同じもの (他の entity の Component を読む窓口) | `T` の読み取り |

- `ComponentLookup` は読み取り専用に限る。他の entity へ書き込めると、2 つの行が同じ entity に書き込みうるので、行ごとに並列で回してよいという前提が崩れる。他の entity を書き換えるときは、`EntityCommands` に積むか、並列にしないノードで行う。
- 同じ型 `T` について、`T&` と `nox::ComponentLookup<const T>` を同じメソッドで受け取らない (`static_assert` で弾く)。他の行が書き換えている途中の `T` を読むことになり、並列ならデータ競合、直列でも結果が entity を回る順番に左右される。
- `OnUpdate` は Component を 1 つ以上受け取る。Component が無いと Query が全ての Archetype に一致し、entity の数だけ呼ばれてしまう。1 フレームに 1 回の処理は Service のメソッドにする。

## 5. オプション

| オプション | 意味 | Query への効果 | 実行順への効果 |
|---|---|---|---|
| `nox::RequireComponents<Ts...>` | この型を全て持つ entity だけを対象にする | 必須の型に足す | なし (読み書きしないので衝突しない) |
| `nox::RunAfter<Ts...>` | `Ts` の後に実行する | なし | `Ts` → 自分 の辺 |
| `nox::RunBefore<Ts...>` | `Ts` の前に実行する | なし | 自分 → `Ts` の辺 |

- `RunAfter` / `RunBefore` の相手は型単位で、その型の同じフレーム区間の全メソッドに掛かる。
- 相手が World に登録されていない、または同じフレーム区間にノードを持たないときは、宣言の誤りとして起動を失敗させる (黙って無視すると順序の保証が消えたことに気づけない)。
- 将来足す候補: `nox::ExcludeComponents<Ts...>` (この型を持つ entity を除く)。絞り込みなのでオプションの側に置く。

## 6. 構造変更 (EntityCommands)

entity の生成・破棄、Component の追加・削除は、`nox::EntityCommands&` を引数で受け取って積む。フェーズの末尾に反映し、順番は「ノードの外のバッファ → ノードのバッファを全体の実行順の昇順」で固定する。どのワーカーが先に走ったかに左右されない。

### Service にしない理由

衝突の規則は「同じ Service に書き込みが絡めば衝突」なので、構造変更のバッファを Service にすると、構造変更をするノードが全て互いに衝突し、並列に動かなくなる。バッファは `nox::EntityCommands&` を宣言したノード 1 つにつき 1 本持ち、ノード同士の衝突の原因にしない。

### Chunk を並列に回す System からの構造変更 (方針。未実装)

1 つのノードのバッファに複数のワーカーが積むと、積まれる順番がワーカーのスケジュールで毎フレーム変わる。反映の順番が変わると Archetype に行が入る順番が変わり、次のフレームで entity を回る順番まで変わる。`work/updater-graph-unification` では、Chunk 並列の宣言と `nox::EntityCommands&` を同時に使えないようにしている。

両立させるときは次の形にする。

- バッファは、ノードごとに、Chunk の数ではなくワーカーの数だけ持つ (起動時に決まるので固定で確保できる)。
- 各コマンドに (Query の中での Archetype の順番, Chunk の番号, 行, その行の中での通し番号) を付ける。Chunk の番号は Archetype ごとの番号なので、Archetype の順番も含めないと印が一意にならない。
- 反映のときに、そのノードのワーカーごとのバッファを集めて印の順に並べ替えてから反映する。ノード同士の順番は、上の全体の実行順に従う。

どのワーカーがどの Chunk を処理しても、反映の順番は同じになる。代償は反映のときの並べ替えだけ。

## 7. 採らなかった案

| 案 | 採らなかった理由 |
|---|---|
| 種類ごとの名前空間を使わず `nox` 直下に置く | Component の名前が他の型とぶつかる。`nox::` の補完が System / Logic で埋まる |
| 名前空間を「種類 → モジュール」の順にする (`nox::components::render::...`) | 名前空間の持ち主がモジュールを表さなくなり、既存の `nox::render` と揃わない |
| 型名から種類の語を外す (`nox::systems::Move`) | 名前空間なしで表示される場面で種類が分からない |
| EntitySystem も `RegisterList` でメソッドを登録する | よくある形 (`OnUpdate` 1 つ) の記述が増え、メソッド名とフェーズを二重に書く。メソッドごとのオプションと複数メソッドは、関心事が 1 つの System では使わない |
| オプションを位置で決まるテンプレート引数にする (旧い設計。今は Doxygen の説明にだけ残る) | 使わない位置にも空の型を並べる必要があり、オプションを足すたびに全ての書き方が変わる |
| オプションをクラスの中の `using Options` に書く | 機能は同じだが、派生クラスの補完に名前が 1 つ増える。基底の並びの方がクラスの先頭で何者か分かる |
| オプションの種類ごとにクラスの中で public の `using RunAfter = nox::TypeList<...>;` などを書く (`work/updater-graph-unification` の形) | オプションの種類ごとに名前が補完に増える。private に書くと見えず宣言が無いのと同じになる、という落とし穴がある (同ブランチの設計書にも記録がある) |
| 他の entity へのアクセサや構造変更のバッファをオプションで宣言する | 使うものなので引数にする (§3) |
| `ExtraResource` のオプション | World に 1 つの共有状態は Service にまとめる (実行モデルの設計書でも Resource を別の概念にしない)。Service を引数で受け取れば足りる |
| 構造変更のバッファを Service にする | 構造変更をするノードが全て直列になる (§6) |

## 8. 決まっていない点

- フレームの区間 (FrameIngress / Update / Presentation) と `OnUpdate` / `OnAdd` / `OnRemove` の対応。`OnUpdate` を既定で Update にし、別の区間で動かすオプション (`InPhase<...>` など) を足すかどうか。
- EntityLogic の書き方 (`RegisterList` を残すか、型全体のオプションを基底のテンプレート引数にも持たせるか)。EntityLogic は 1 つの型が複数の区間で動く必要があるかで決める。どちらでもオプションの型と解析は EntitySystem と共通にする。
- 旧名を残す属性の形 (名前空間や型名を変えたときの読み替え)。
- 既存の型の改名。`nox::GarbageCollector` / `nox::dev::net::SocketScheduler` / render モジュールの `Renderer` などは §1 の規則 (`services` の名前空間、`Service` の末尾) に沿っていない。Service へ移すときに改名するか。保存データに載る前に決める。
- Chunk 並列の構造変更 (§6) を実装する時期。
