# Runtime ECS の書き方

利用者が書く ECS の型 (Component / Service / EntitySystem / EntityLogic) の置き場所と名前、フレームの区間、EntitySystem と EntityLogic の書き方を決める文書。決めた理由と採らなかった案も残す。

- フレームの中の実行順 (UpdaterGraph) の決め方は、`work/updater-graph-unification` の実行モデルの設計書 (`docs/runtime-execution-model.md`、master へは未取り込み) が扱う。この文書はその規則 (引数の並びがアクセスの宣言、など) を前提にする。
- ただし次の点は、この文書で置き換える。
  - オプションの書き方: そのブランチではクラスの中に public の `using RunAfter = nox::TypeList<...>;` / `using RunBefore` を書くが、この文書では基底のテンプレート引数に並べる (§3、§4)。
  - フレームの区間: そのブランチは FrameIngress / Update / Presentation を誰でも選べる区間にしているが、この文書では利用者が選べるのは Update だけにする (§2)。
  - 同じ EntityLogic の型のメソッドどうしの衝突: そのブランチは必ず衝突させるが、この文書では `this` を Component へのアクセスとして扱って判定する (§4)。
- Service の書き方 (寿命、ノードになるメソッドの宣言) は、まだこの文書の対象外。`RunAfter` / `RunBefore` の相手に Service を書いたときの意味だけを §7 で決める。
- master のコードは、まだこの文書の形に揃っていない。実装を合わせるときは、この文書を正とする。
  - `runtime/core/entity_system.h` には `nox::EntitySystem<T, Options...>` の宣言と印の型 (`nox::RequireComponents` / `nox::RunAfter` / `nox::RunBefore`) があるが、Doxygen の説明は位置で決まる旧い形 (ExtraRequiredComponents / AfterSystems / BeforeSystems) のまま。
  - 基底は `nox::EntitySystemBase` (`nox` 直下) のままで、`nox::concepts::EntitySystem` もそれを参照している。`nox::detail` へは未移動。
  - オプションの解析、private への格納、private メソッドの検出は未実装。`nox::ExcludeComponents` も未実装。
  - 実際に使われている System は `nox::legacy::EntitySystem`。EntityLogic の新しい形は master にまだ無い。
  - Component の基底は `nox::detail::IComponentData` へ移してあり (§1)、core・テスト・ベンチマークの Component は `final` で `nox::Component<T>` を継承している。`nox` 直下の前方宣言は残っていない。`RunAfter` / `RunBefore` の相手を表す共通の印の基底 `nox::detail::UpdaterNodeOwnerBase` はあるが、コンセプト `nox::concepts::OrderTarget` (§1) は無く、`RunAfter` / `RunBefore` は相手を検査していない。
- 決まっていない点は §10 にまとめる。

## 1. 名前空間と型の名前

名前空間は「モジュール → 種類」の順にする。core モジュールは `nox` 直下に置く。

| 種類 | 名前空間 | 型の名前 | 例 |
|---|---|---|---|
| Component | `<モジュール>::components` | 役割を表す名詞 | `nox::components::LocalPosition` |
| Service | `<モジュール>::services` | 役割を表す名前 (種類の語は付けない) | `nox::services::Time` |
| EntitySystem | `<モジュール>::systems` | 役割を表す名前 (種類の語は付けない) | `nox::systems::Movement` |
| EntityLogic | `<モジュール>::entity_logics` | 役割を表す名前 (種類の語は付けない) | `nox::entity_logics::EnemyAI` |

ほかのモジュールは `nox::render::components::MeshRenderer`、開発用は `nox::dev::components::Name`、ゲーム側は `game::components::Health` のように置く。

- 型名に `Service` / `System` / `Logic` のような種類を表す語を付けない。種類は名前空間で表す。
- 種類やモジュールをまたいだ同じ名前は許す (`nox::components::Camera` と `nox::systems::Camera`、`nox::dev::components::Name` と `game::components::Name`)。
- 同じ名前の型の中では、修飾なしの名前は自分自身を指す (`nox::systems::Camera` の中の `Camera` は System 自身)。同じ名前の Component は `nox::components::Camera` と完全修飾で書く。

### 理由

- **Component の名前はぶつかりやすい。** `Position` / `Rotation` / `Name` / `Transform` のような一般的な名詞になりやすい。実際に `nox::Position` は算術の型として既にある。
- **`nox::` の補完を汚さない。** System と Logic はエンジンが大きくなると数百になる。種類ごとに分ければ、`nox::components::` と打ったときの候補が Component の一覧になる。
- **種類は名前空間で表し、型名には付けない。** 名前空間で種類が分かるので、型名に重ねると `nox::systems::MoveSystem` のような重複になる。名前空間なしで型名が表示されると種類が分からなくなる点は、型名を文字列で出す所を全て完全修飾名にすることで補う (下の「型名の表示と `using namespace`」)。
- **順番を「モジュール → 種類」にする。** 名前空間の持ち主がそのままモジュールを表し、既存の `nox::render` / `nox::dev` と揃う。

### 型名の表示と `using namespace`

- 型名を文字列で出す所は、全て完全修飾名にする。
  - Editor: `nox::reflection::ClassInfo::GetFullName()`。
  - 起動ログ (実行グラフのノードの一覧など)、アサートとエラーのログ、プロファイラの区間の名前: `nox::reflection::Type::GetTypeName()`。中身は `nox::util::GetTypeName<T>()` の値で、正規化された名前空間付きの名前になる (`runtime/kernel/test/basic_test.cpp` の `TypeNameIsNormalizedAcrossToolsets` が `type_name_probe::Nested::Value` のような名前を確かめている)。
  - コンパイラのエラーメッセージは、もともと名前空間付きで出るので対応は要らない。
- 種類の名前空間を 2 つ以上同時に `using namespace` しない (`components` と `systems` を両方取り込むと、`Camera` のような名前が曖昧になる)。ヘッダでは `using namespace` を使わない。短く書きたいときは名前空間の別名 (`namespace components = nox::components;`) を使う。

### 名前空間は保存データの一部になる

保存と Editor の通信では、型を完全修飾名で指す。Component の名前に加えて、EntityLogic の名前もシーンに残る (EntityLogic はタグとして entity に付くため)。

- 名前空間と型名は、原則として変えない。旧名を残して読み替える属性 (まだ無い) ができるまでは、保存データに載った後には変えられない。
- EntityLogic を EntitySystem へ移し替えるときは、シーンに残ったロジックのタグの扱いも手順に含める。

### 継承する型と、基底の置き場所

- Component / EntitySystem / EntityLogic は、テンプレートの `nox::Component<T>` / `nox::EntitySystem<T, Options...>` / `nox::EntityLogic<T, Options...>` を継承する。
- テンプレートでない基底 (`IComponentData` / `EntitySystemBase` / `EntityLogicBase`) は `nox::detail` に置く。利用者が名前を書くことはない。基底の入れ子の名前 (`Register` / `Trigger`) は、基底の名前空間に関係なく派生クラスの中からそのまま使える。ただし派生クラス自身がクラステンプレートのとき (基底がテンプレート引数に依存するとき) は、修飾なしでは見つからないので、基底の名前で修飾する。
- Service の基底 (`nox::ServiceBase`) と、利用者が継承する `nox::Service<T>` は、Service の書き方を決めるまで今の場所のまま (§10)。ただし下の共通の印の基底は継承する。
- 基底を置く `detail` の名前空間のブロックには、反射の対象外の印 (`IgnoreReflection`) を付けない。Editor のロジックの一覧などで、反射の派生クラスの列挙の起点にするため。
- Component の基底は `nox::detail::IComponentData`。Component は必ず `nox::Component<T>` を継承し、`IComponentData` を直接継承しない。テスト・ベンチマークと `nox::LocalTransform` で `nox::IComponentData` を直接継承している Component は、`IComponentData` を移すのと同じ変更で `nox::Component<T>` へ直す。名前で参照している所 (`entity_access_legacy.h`、`component_id.h`) も同じ変更で直す。`nox` 直下に `IComponentData` の前方宣言を残さない (`entity_system.h` と `world_legacy.h` にあるものも消す) (残すと `nox::IComponentData` が中身の無い別の型を指し、継承した所はコンパイルエラーに、`std::derived_from<nox::IComponentData>` のような検査は黙って常に偽になる)。
- EntitySystem / EntityLogic / Service の基底は、共通の印 `nox::detail::UpdaterNodeOwnerBase` を public に継承する。`RunAfter` / `RunBefore` の相手になれる型 (UpdaterGraph のノードを持つ型) を、コンセプト `nox::concepts::OrderTarget` (`std::derived_from<T, nox::detail::UpdaterNodeOwnerBase>`) で判定するためで、中身も仮想関数も持たない。Component は継承しない (ノードを持たないので相手にならない)。
  - 基底の型そのもの (`nox::EntitySystem<X>` など) もコンセプトを満たしてしまうが、ノードを持たないので §7 の起動の失敗の判定で検出される。
  - 空の基底は 1 本の継承の鎖にする。空の基底を複数並べると、MSVC の ABI (MSVC と ClangCL の両方) では空の基底の最適化が効かず、型が大きくなる。今の `nox::detail::IECSBase` / `nox::detail::ISystemBase` はこの印の基底に置き換えるか、印の基底から派生させる。
  - 1 本の鎖なら、EntityLogic の状態の大きさは増えず、「状態を持たない」の判定 (`std::is_empty_v`) も変わらない。実装ではテストで確かめる (状態を持たないロジックが `std::is_empty_v` を満たすこと、状態を持つロジックの `sizeof` が、同じメンバだけを持つ構造体の `sizeof` と一致すること)。

### 置かない場所

- 無名名前空間: ReflectionGenerator が型名を書けない。
- `nox::detail`: 内部の実装専用。利用者が書く型は置かない。

## 2. フレームの区間

この文書では、フレームの大きな区切りを「区間」、Update の区間の中の区切りを「段階」と呼ぶ。

**利用者が書く System と Logic は、全て Update の区間で動く。** 区間を選ぶ書き方は用意しない。

```
[エンジンの取り込みの区間] → [Update の区間] → [エンジンの後処理の区間]
  入力・受信など               利用者の System / Logic   描画への受け渡し・GC など
```

- エンジンの取り込みと後処理の区間は、エンジンの Service だけが使う内部の区間で、利用者は選べない。
- 構造変更 (§8) は、各区間の末尾にまとめて反映する。Update の区間で作った entity が利用者の System / Logic から見えるのは、次のフレームの Update の区間から。
- Update の区間の中は、さらに次の段階に分かれる。段階の順番は固定。

```
[Add の段階] → [毎フレームの段階] → (区間の末尾) [Remove の段階] → [構造変更の反映: 行を移す・消す]
```

| 段階 | 呼ばれるもの |
|---|---|
| Add | 前回の反映で Query に入った entity について、`OnAdd` (System) / `Trigger::Add` のメソッド (Logic) |
| 毎フレーム | `OnUpdate` (System) / `Trigger::Update` のメソッド (Logic) |
| Remove | これから反映する構造変更で Query から出る entity について、`OnRemove` (System) / `Trigger::Remove` のメソッド (Logic) |

- Remove の段階は、反映の前に、Query から出る entity を全て集めてから走らせる。コマンドを 1 件ずつ反映しながら呼ぶのではない。Remove のメソッドはデータを消す前に呼ばれるので、出ていく entity の Component を読める。
- Add / Remove の「Query」は、System ではそのメソッドの Query (§3)、Logic ではロジックの Query (§4)。
- エンジンの区間の反映 (Editor からの削除など) で Query から出る entity の Remove をいつ呼ぶかと、Remove のメソッドが `EntityCommands&` に積んだ構造変更をいつ反映するかは、まだ決めていない (§10)。

### 理由

- 区間の区切りは、同期が必要な境目 (入力が届く、描画へ渡す、固定時間で回す) で決めるもの。その多くはエンジンの都合で、利用者のロジックが区間を選ぶ必要はまだ無い。
- 利用者のロジックが入力の後に動くことを、全てのノードに `RunAfter` を書かずに保証するには、エンジンの取り込みを Update より前の区間に置けばよい。
- 区間の境目は並列性を断ち切る。利用者に選ばせると、判断が増えるうえに並列にできる範囲が狭くなりやすい。

### 必要になったら足すもの

| きっかけ | 足すもの |
|---|---|
| 物理を入れる | 固定時間の区間 (1 フレームに 0 回以上回す) |
| 利用者が描画の準備を書く | 後処理の区間を利用者にも選べるようにするオプション (`InPhase<...>` など) |
| 同じフレームの中で作った entity を見たい | 区間を増やす前に、構造変更の反映点を読み書きの宣言から自動で入れる方法を検討する |

## 3. EntitySystem の形

```cpp
namespace nox::systems
{
	class Movement final : public nox::EntitySystem<Movement,
		nox::RunAfter<Input>,
		nox::RequireComponents<nox::components::Movable>>
	{
		NOX_ECS_DECLARE_VERIFY(Movement);
	private:
		static void OnUpdate(nox::components::LocalPosition& position, const nox::components::Velocity& velocity);
	};
}
```

### メソッドは決まった名前の static だけ

- 定義できるのは `OnUpdate` / `OnAdd` / `OnRemove` の 3 つで、**全て static**。登録の一覧 (`RegisterList`) は書かない。
- **インスタンスは作れない。** 基底のコンストラクタは削除してある。System は状態を持たない。entity ごとの状態は Component に、World に 1 つの状態は Service に置く。
- 関数の中の static 変数や、書き換えるグローバル変数を使わない。隠れた共有状態になり、並列に回すと競合する (コンパイル時には検出できないので規約で守る)。
- 1 つも定義していなければコンパイルエラーにする (名前の書き間違いで、黙って何もしない System になるのを防ぐ)。この検査は、登録する翻訳単位でも行う。`NOX_ECS_DECLARE_VERIFY` は書かなくてもよいマクロなので、マクロ経由の検査だけに頼ると、書き忘れたときに検査が走らない。
- 3 つのアクセス指定は揃える。private にするなら 3 つとも private。private のメソッドは、`NOX_ECS_DECLARE_VERIFY` が friend として宣言する登録用の入口から検出する。マクロを書き忘れると private のメソッドが見えなくなるが、揃えておけば「全部見えない」として上の検査でエラーになる。
- 同じ名前のオーバーロードは書かない。関数のアドレスを取る式で存在を調べると、オーバーロードされた名前は「無い」と判定され、黙って無視される。検出の実装は、名前の存在と曖昧さを区別して `static_assert` で弾くこと。
- `OnUpdate` は Component を 1 つ以上受け取る。Component が無いと Query が全ての Archetype に一致し、entity の数だけ呼ばれてしまう。1 フレームに 1 回の処理は Service のメソッドにする。
- 各メソッドの Query は、そのメソッドの引数の Component と、オプションの `RequireComponents` / `ExcludeComponents` から決まる。`OnAdd` / `OnRemove` は、そのメソッドの Query に入ったとき・出るときに呼ばれる。
- 1 つの System は 1 つの関心事だけを持つ。別の処理を足したくなったら System を分ける。

### static だけにする理由

- System のメンバへの書き込みがなくなるので、Chunk を並列に回しても安全になる。
- System の型そのものは衝突の原因にならず、衝突は引数の宣言だけで決まる。
- 呼び出す関数はテンプレート引数の定数になり、インライン展開できる。

### オプションは基底のテンプレート引数

- `nox::EntitySystem<T, Options...>` の `Options` に、種類の印が付いた型を順番自由で並べる (オプションの一覧は §7)。
- 同じ種類を 2 回書いたら、つないで 1 つにする (`nox::RunAfter<A>, nox::RunAfter<B>` は `A` と `B` の両方の後)。
- 基底の並びはクラスのスコープの外なので、名前空間まで書く。System の定義を `namespace nox::systems { ... }` の中に書けば、同じ名前空間の相手は短く書ける。

### 基底はオプションを持つだけ

基底の `nox::EntitySystem<T, Options...>` は、ヘッダを読む全ての翻訳単位で実体化される (EntityLogic の基底も同じ)。

- 基底はオプションを型の別名として持つだけにする。派生クラスの補完に出ないよう private に置き、登録用の入口から読む。派生クラスに書く friend 宣言は基底の private に届かないので、基底の側でも登録用の入口を friend にする。
- 基底で行う検査は、型特性だけで済む軽いものに限る (未知のオプションが無いか、メソッドが 1 つ以上あるか)。
- オプションの解析 (種類ごとの配列への振り分け、型情報の取得) は、登録する翻訳単位でだけ行う。
- 登録する翻訳単位では、オプションに書いた相手の型のヘッダも読み、完全型にしておく。型情報 (`nox::reflection::Typeof<T>()`) は不完全型だとサイズ 0 として作られるため、翻訳単位によって完全型と不完全型が混ざると、同じ定数の中身が食い違い ODR 違反になる。

## 4. EntityLogic の形

entity ごとに状態を持つ振る舞い。少数の主要な個体 (プレイヤー、ボス、UI) に使う。

```cpp
namespace nox::entity_logics
{
	class EnemyAI final : public nox::EntityLogic<EnemyAI,
		nox::RequireComponents<nox::components::Enemy, nox::components::Spawn>>
	{
		NOX_ECS_DECLARE_VERIFY(EnemyAI);
	private:
		void Initialize(nox::Entity self, const nox::components::Spawn& spawn);
		void UpdateAI(nox::Entity self, nox::components::LocalPosition& position, const nox::services::Time& time);
	public:
		using RegisterList = std::tuple<
			Register<&EnemyAI::Initialize, Trigger::Add>,
			Register<&EnemyAI::UpdateAI, Trigger::Update, nox::RunAfter<nox::systems::Navigation>>
		>;
	private:
		enum class State : nox::uint8 { Idle, Chase, Attack };
		State state_;
		nox::float32 cooldown_;
	};
}
```

### メソッドは `RegisterList` に自由な名前で登録する

- `Register<&T::Method, Trigger, Options...>` を `RegisterList` (`std::tuple`) に並べる。メソッドの名前は自由。
- `Trigger` は呼ばれるきっかけで、`Update` (毎フレーム) / `Add` (Query に入ったとき) / `Remove` (Query から出るとき) のどれか。呼ばれる段階は §2 のとおり。
- **ロジックの Query** は、ロジック自身のタグ + 基底の `RequireComponents` / `ExcludeComponents` で決まる。`Trigger::Add` / `Trigger::Remove` のメソッドは、entity がロジックの Query に入ったとき・出るときに呼ばれる。
- 毎フレームのメソッドの Query は、ロジックの Query に、そのメソッドの引数の Component と `Register` のオプションを足したもの。ロジックの Query より広くならないので、毎フレームのメソッドは必ず Add の後に呼ばれる (Add の段階は毎フレームの段階より先)。状態の初期値を Add のメソッドで入れられるのは、この保証による。
- 登録する翻訳単位で、Add / Remove のメソッドについて次を検査する: 引数に取る Component はロジックの Query で保証される型 (基底の `RequireComponents`) に限る。`Register` に Query を変えるオプション (`RequireComponents` / `ExcludeComponents`) を書かない (`RunAfter` / `RunBefore` は書ける)。
- メソッドは static でないメンバ関数。状態を持つことが EntityLogic の存在理由なので、static で済むなら EntitySystem にする。
- メソッドは private のままでよい (ポインタをクラスの中で取るので、アクセスの検査はそこで済む)。
- `RegisterList` は public に書く (`NOX_ECS_DECLARE_VERIFY` は省略できるので、friend に頼らず登録の処理から読めるようにする)。
- 登録する翻訳単位で次を検査する: 一覧が `Register` の並びである、メソッドが `T` 自身のメンバ関数である、static でない、同じメソッドを 2 回登録していない。

### 1 つの処理は 1 つのメソッドにまとめる

- 別々に登録したメソッドは、UpdaterGraph の別々のノードになる。衝突しなければ並列に動き、衝突すれば全体の実行順 (明示した辺のトポロジカル順。決まらない所は完全修飾の型名 (`Type::GetTypeName()`) + メソッド名の順。種類をまたいで短い名前が同じ型があるので、短い名前では順番が決まらない) で直列になる。
- **同じロジックの、別々に登録したメソッドの間に順番の保証はない。** `RegisterList` に書いた順番にも意味は無い。
- 手順を踏む処理 (知覚 → 判断 → 行動など) は、1 つのメソッド (`UpdateAI`) の中で、状態 (`State` など) を持って進める。
- 別々に登録するのは、並列にしたいとき、きっかけが違うとき、必要な Component が違うとき (メソッドごとのオプション) に限る。

### オプションは基底と `Register` の両方に書ける

| 書く場所 | 掛かる範囲 |
|---|---|
| 基底のテンプレート引数 `nox::EntityLogic<T, Options...>` | そのロジックの全メソッド |
| `Register<&T::Method, Trigger, Options...>` | そのメソッドだけ |

メソッドの記述子を作るときに、両方をつないで 1 つにする。同じ種類が重なったら、EntitySystem と同じくつなぐ。

### 状態 (`this`)

- 状態は Archetype の列として、entity ごとに 1 つ持つ。entity ごとに別のインスタンスなので、1 つのメソッドを entity ごとに並列で回しても行どうしはぶつからない。
- **メソッドの `this` は、ロジック自身の Component へのアクセスとして衝突を判定する。** const のメソッドなら読み取り、そうでなければ書き込み。
- 状態を書き換えるメソッドどうしは、別々のメンバに触っていても衝突する (メンバ単位では追跡しない)。別々に登録して並列の恩恵があるのは、状態を書き換えないメソッドか、状態を持たないロジック。
- 状態を持たないロジック (空のクラス) は列を持たないタグとして扱い、衝突の原因にならない。

### 状態の型の条件

- final、仮想関数なし、trivially copyable、trivially destructible (列として memcpy で移すため)。
- **trivially default constructible** (メンバの初期値の記述を禁止する)。Archetype は新しい行を 0 で埋めるので、初期値を書いても黙って無視される。初期値は `Trigger::Add` のメソッドで入れる。
- デストラクタを持てないので、ヒープやハンドルなどの資源は持たせない。資源は Service に置く。

### そのほか

- 自分の entity は引数の `nox::Entity` で受け取る (基底に `GetEntity` は持たせない。Chunk に Entity の列が既にあるので、状態に持たせると重複する)。
- Component を 1 つも受け取らないメソッドも書ける。ロジック自身のタグが Query の必須の型になるので、全ての Archetype に一致する問題は起きない。
- 引数の種類は EntitySystem と同じ (§6)。

## 5. 引数とオプションの分け方

規則は 1 つだけ。**メソッドの中で使うもの (読み書きするもの) は引数、使わないものはオプション。**

| 分類 | 何を書くか | 置き場所 |
|---|---|---|
| 使うもの | Component、Service、他の entity の参照、構造変更のバッファ | 引数 |
| 使わないもの | 対象の絞り込み、実行の順序 | オプション |

使うものを引数にすると、受け取ったものしか触れないことが型で保証される。オプションで宣言だけして実物を別の経路 (World から型で引くなど) で取ると、宣言と実際の使い方がずれても気づけない。

## 6. 引数の種類

| 引数 | 渡し方 | 衝突の判定 |
|---|---|---|
| Component の参照 (`T&` / `const T&`) | 行ごと (列の先頭 + 行) | const なら読み取り、そうでなければ書き込み |
| `nox::Entity` | 行ごと (Entity の列) | 対象外 |
| Service の参照 / ポインタ | 全行で同じもの | const なら読み取り。参照で受け取るのに World に無ければ呼び出しを打ち切り、ポインタなら nullptr を渡す |
| `nox::EntityCommands&` | 全行で同じもの (このノード専用のバッファ) | 衝突の原因にしない (§8) |
| `nox::ComponentLookup<const T>` | 全行で同じもの (他の entity の Component を読む窓口) | `T` の読み取り |

- `ComponentLookup` は読み取り専用に限る。他の entity へ書き込めると、2 つの行が同じ entity に書き込みうるので、行ごとに並列で回してよいという前提が崩れる。他の entity を書き換えるときは、`EntityCommands` に積むか、並列にしないノードで行う。
- 同じ型 `T` について、`T&` と `nox::ComponentLookup<const T>` を同じメソッドで受け取らない (`static_assert` で弾く)。他の行が書き換えている途中の `T` を読むことになり、並列ならデータ競合、直列でも結果が entity を回る順番に左右される。
- EntityLogic のメソッドでは、これに加えて `this` がロジック自身の Component へのアクセスになる (§4)。

## 7. オプション

| オプション | 意味 | Query への効果 | 実行順への効果 |
|---|---|---|---|
| `nox::RequireComponents<Ts...>` | この型を全て持つ entity だけを対象にする | 必須の型に足す | なし (読み書きしないので衝突しない) |
| `nox::ExcludeComponents<Ts...>` | この型をどれか持つ entity を除く | 除外の型に足す | なし |
| `nox::RunAfter<Ts...>` | `Ts` の後に実行する | なし | `Ts` → 自分 の辺 |
| `nox::RunBefore<Ts...>` | `Ts` の前に実行する | なし | 自分 → `Ts` の辺 |

### オプションの型は印にとどめる

- オプションの型は、名前を並べるだけの印にする。中身を確かめる制約 (相手が System か、Component か、など) は付けない。
- 特に、`std::is_base_of_v` などで相手が完全型であることを要求しない。相手は前方宣言でよい。互いに `RunAfter` / `RunBefore` で参照し合う 2 つの型も書ける。
- 相手の種類の検査は、登録する翻訳単位で、完全型が揃ってから行う。`RunAfter` / `RunBefore` の相手は `nox::concepts::OrderTarget` (§1)、`RequireComponents` / `ExcludeComponents` の型は `nox::concepts::Component` を満たすことを `static_assert` で確かめる。

### `RunAfter` / `RunBefore` の相手と範囲

- 相手は EntitySystem / EntityLogic / Service のどれでもよい。型単位で指定する。
- 辺は、自分のメソッドと、**相手の型の同じ段階 (§2 の Add / 毎フレーム / Remove) のメソッド全部**との間に張る。段階どうしの順番は固定なので、段階をまたぐ辺は張らない。

| 自分のメソッド | 相手のどのメソッドとの間に辺を張るか |
|---|---|
| `OnUpdate` / `Trigger::Update` | 相手の、毎フレームの段階のメソッド全部 |
| `OnAdd` / `Trigger::Add` | 相手の、Add の段階のメソッド全部 |
| `OnRemove` / `Trigger::Remove` | 相手の、Remove の段階のメソッド全部 |

- 相手が Service のときは、その Service のフレームのノードになるメソッドとの順番を決める。Update の区間にある Service のノードは、毎フレームの段階として扱う。Service の初期化と終了の順番 (寿命) とは別のもので、`RunAfter` は寿命に影響しない。
- 相手のメソッドが区間や段階の順番だけで既に満たされている場合 (`RunAfter` の相手がエンジンの取り込みの区間や、自分より前の段階にしか無い。`RunBefore` の相手がエンジンの後処理の区間や、自分より後の段階にしか無い) は、辺を張らずに満たされているとみなす。順番と逆向きのメソッドの組 (`RunAfter` の相手が後処理の区間や、自分より後の段階にしか無い、など) は、辺も張れず満たされもしない組として扱う。それだけでは起動を失敗させず、下の (自分の型, 相手の型) の組ごとの判定に含める。
- 自分自身の型は指定できない (宣言の誤りとしてエラーにする)。自分のメソッドどうしの順番は保証しないと決めているうえ、自分への辺は循環になるため。
- **(自分の型, 相手の型) の組ごとに、自分の全メソッドを通じて 1 本も辺が張れず、区間や段階の順番でも満たされていなければ、起動を失敗させる** (相手の書き間違いや、相手のモジュールの取り込み漏れ)。`RunAfter<A, B>` は `A` と `B` を別々に判定する。一部のメソッドの組だけ辺が張れないのは正常とする (例: 基底に書いた `RunAfter<B>` が自分の `Add` のメソッドにも掛かるが、`B` には毎フレームの段階のメソッドしか無い)。
- 型単位なので、相手のメソッドの一部とだけ順番を付けたい場合も、全部と順番が付く。メソッド単位で相手を指定するオプションは、必要になってから足す。

## 8. 構造変更 (EntityCommands)

entity の生成・破棄、Component の追加・削除は、`nox::EntityCommands&` を引数で受け取って積む。区間の末尾に反映し、順番は「ノードの外のバッファ → ノードのバッファを全体の実行順の昇順」で固定する。どのワーカーが先に走ったかに左右されない。Update の区間では、反映の前に、Query から出る entity について Remove の段階を走らせる (§2)。

### Service にしない理由

衝突の規則は「同じ Service に書き込みが絡めば衝突」なので、構造変更のバッファを Service にすると、構造変更をするノードが全て互いに衝突し、並列に動かなくなる。バッファは `nox::EntityCommands&` を宣言したノード 1 つにつき 1 本持ち、ノード同士の衝突の原因にしない。

### Chunk を並列に回す System からの構造変更 (方針。未実装)

1 つのノードのバッファに複数のワーカーが積むと、積まれる順番がワーカーのスケジュールで毎フレーム変わる。反映の順番が変わると Archetype に行が入る順番が変わり、次のフレームで entity を回る順番まで変わる。`work/updater-graph-unification` では、Chunk 並列の宣言と `nox::EntityCommands&` を同時に使えないようにしている。

両立させるときは次の形にする。

- バッファは、ノードごとに、Chunk の数ではなくワーカーの数だけ持つ (起動時に決まるので固定で確保できる)。
- 各コマンドに (Query の中での Archetype の順番, Chunk の番号, 行, その行の中での通し番号) を付ける。Chunk の番号は Archetype ごとの番号なので、Archetype の順番も含めないと印が一意にならない。
- 反映のときに、そのノードのワーカーごとのバッファを集めて印の順に並べ替えてから反映する。ノード同士の順番は、上の全体の実行順に従う。

どのワーカーがどの Chunk を処理しても、反映の順番は同じになる。代償は反映のときの並べ替えだけ。

## 9. 採らなかった案

| 案 | 採らなかった理由 |
|---|---|
| 種類ごとの名前空間を使わず `nox` 直下に置く | Component の名前が他の型とぶつかる。`nox::` の補完が System / Logic で埋まる |
| 名前空間を「種類 → モジュール」の順にする (`nox::components::render::...`) | 名前空間の持ち主がモジュールを表さなくなり、既存の `nox::render` と揃わない |
| 型名の末尾に種類の語を付ける (`nox::systems::MoveSystem`) | 名前空間で種類が分かるので重複になる。名前空間なしで表示される場面は、表示を完全修飾名にすることで補う |
| `IComponentData` を旧版を消すまで `nox` 直下に残す | Component の基底の置き場所が、新旧で割れたままになる。直接継承している所 (テスト・ベンチマーク・`nox::LocalTransform`) と名前で参照している所 (旧版) は機械的に直せるので、今移す |
| 利用者も FrameIngress / Update / Presentation などの区間を選べるようにする | 区間を必要としているのはエンジンの都合 (入力の取り込み、描画への受け渡し) で、利用者のロジックにはまだ要らない。選ばせると判断が増え、区間の境目で並列性も切れる |
| EntitySystem も `RegisterList` でメソッドを登録する | よくある形 (`OnUpdate` 1 つ) の記述が増え、メソッド名ときっかけを二重に書く。メソッドごとのオプションと複数メソッドは、関心事が 1 つの System では使わない |
| EntitySystem にインスタンス (メンバ) を持たせる | Chunk を並列に回したときにメンバへの書き込みが競合する。状態は Component か Service に置けば足りる |
| EntityLogic も決まった名前 (`OnUpdate` / `OnAdd` / `OnRemove`) にする | 1 つの Update では足りず、並列化のために別々のノードとして登録したいメソッドや、必要な Component が違うメソッドを書けない |
| EntityLogic のメソッドを `RegisterList` に書いた順に実行する | 順番が要る処理は 1 つのメソッドにまとめる方針。別々に登録したメソッドは UpdaterGraph が並列・直列を決める |
| `Register` のオプションを `std::tuple<...>` で包んで渡す | 包み忘れたときに中身の無い基本テンプレートに当たり、分かりにくいエラーになる。最初から可変長で受け取れば部分特殊化も要らない |
| きっかけの列挙を `Phase::OnUpdate` / `OnAdd` / `OnRemove` と呼ぶ | メソッドの名前が自由になると、`OnUpdate` は「`OnUpdate` という名前のメソッド」と紛らわしい。フレームの区間とも紛れるので、`Trigger::Update` / `Add` / `Remove` にする |
| EntityLogic の Add / Remove のきっかけを、メソッドごとの Query にする | Add のメソッドの引数が揃わない entity では Add が呼ばれず、毎フレームのメソッドが状態 0 のまま動きうる。状態の初期値を Add で入れる前提が崩れる |
| 同じ EntityLogic の型のメソッドを必ず衝突させる (`work/updater-graph-unification` の規則) | `this` を Component へのアクセスとして扱えば同じことが一般の規則で表せ、const のメソッドや状態の無いロジックは並列にできる |
| EntityLogic の状態をメンバの初期値で初期化する | 行は 0 で埋めるので初期値が黙って無視される。行ごとに構築すると行の追加が重くなる |
| オプションを位置で決まるテンプレート引数にする (旧い設計。今は Doxygen の説明にだけ残る) | 使わない位置にも空の型を並べる必要があり、オプションを足すたびに全ての書き方が変わる |
| オプションをクラスの中の `using Options` に書く | 機能は同じだが、派生クラスの補完に名前が 1 つ増える。基底の並びの方がクラスの先頭で何者か分かる |
| オプションの種類ごとにクラスの中で public の `using RunAfter = nox::TypeList<...>;` などを書く (`work/updater-graph-unification` の形) | オプションの種類ごとに名前が補完に増える。private に書くと見えず宣言が無いのと同じになる、という落とし穴がある (同ブランチの設計書にも記録がある) |
| オプションの型に、相手の種類を確かめる制約を付ける | 制約を検査するには相手が完全型である必要があり、前方宣言で書けなくなる (互いに参照し合う 2 つの型も書けない) |
| 他の entity へのアクセサや構造変更のバッファをオプションで宣言する | 使うものなので引数にする (§5) |
| `ExtraResource` のオプション | World に 1 つの共有状態は Service にまとめる (実行モデルの設計書でも Resource を別の概念にしない)。Service を引数で受け取れば足りる |
| 構造変更のバッファを Service にする | 構造変更をするノードが全て直列になる (§8) |

## 10. 決まっていない点

- Service の書き方 (寿命、ノードになるメソッドの宣言、基底を `nox::detail` に移す時期)。
- エンジンの区間の反映で Query から出る entity の Remove をいつ呼ぶか (反映の前にその場で呼ぶか、Update の区間まで遅らせるか)。利用者のメソッドは Update の区間で動くという規則との兼ね合いで決める。
- Remove のメソッドが `EntityCommands&` に積んだ構造変更をいつ反映するか (同じ反映に含めるか、次の区間の末尾に回すか、Remove のメソッドでは受け取れないようにするか)。
- EntityLogic を、その World で登録していないときの扱い (データとして付けられるが動かない、にするか)。
- 旧名を残す属性の形 (名前空間や型名を変えたときの読み替え)。
- 既存の型の置き場所。`nox::GarbageCollector` / `nox::dev::net::SocketScheduler` / render モジュールの `Renderer` などは §1 の規則 (`services` の名前空間に置く) に沿っていない。Service へ移すときに名前空間を移すか。core の Component `nox::LocalTransform` も `nox::components` に置いていない (Editor が `Editor/Core/SceneHierarchyManager.cs` で完全修飾名 `"nox::LocalTransform"` を定数で持っているので、移すなら一緒に直す)。保存データに載る前に決める。
- Chunk 並列の構造変更 (§8) を実装する時期。
