## 最優先ルール

- CPU/メモリ性能を最優先する。設計・実装・レビューでは常に実行時コストで判断する。
- ヒープ確保は可能な限り撤廃する。必要な場合も回数・寿命・配置を厳密に管理し、連続フレーム中の不要な確保を避ける。
- Runtime (C++) は例外を使わない。エラーは戻り値・アサート・ログで明示的に扱う。

## 言語

- PRやコードレビューなどは日本語で書く

## 作業ルール(エージェント共通)

### ブランチ

長命なブランチは `master` 1 本だけにする (作業ブランチと併存させて、同じ内容が別 SHA で二重に積まれた事故があった)。

- エージェントは `work/<タスク名>` を切り (セッション側で作業ブランチが指定されている場合はそれに従う)、検証が通ったら PR を作って止める。`master` への取り込み (PR のマージ) はユーザーが行う。「進めて」「再開して」などは取り込みの許可ではない。
- ユーザーは `master` へ直接コミットし、実験だけ `user/<topic>` を切る。
- **`master` への force-push は禁止。** ユーザーのコミットが失われる。

`master` はルールセットで守っている。PR 必須 (承認 1。回避できるのはリポジトリ管理者 = ユーザーだけ) で、削除と force-push は誰もできない。手元のエージェントは GitHub App (`noxitro-claude[bot]`) の身元で push と PR 作成をするので、`master` へ直接 push できず、自分の PR もマージできない。ユーザーの身元で動くエージェント (クラウドのセッションなど) はルールセットでは止まらないので、上の規則を守る。

### ワークツリー

**ユーザーのマシンでは、エージェントはメインのチェックアウトで編集しない** (Visual Studio のファイルロックで長時間止まった実績がある)。`git worktree add` で専用のツリーを作って編集・コミットし、メインで行うのは git 操作 (commit / merge / push) だけにする。着手時に触るファイルを宣言すると衝突を避けられる。

### 検証

**検証は CI に任せ、エージェントはローカルでビルド・テストしない** (ユーザーのマシンの負荷を抑えるため)。ユーザーが明示的に頼んだときだけ行い、手順は `docs/local-build.md` に従う。

- 作業ブランチを push すると CI (`.github/workflows/ci.yml`) がビルド・テスト・`runtime.exe` の起動確認を走らせる。構成の詳細は `ci.yml` 冒頭のコメント。作業ブランチ (`work/*` またはセッションで指定されたブランチ。`master` は含まない) の push は確認なしでよい。`.github` 以下を変えると Workflow lint (actionlint / ruff) も走る。
- 落ちたらログ (`gh run view <run-id> --log-failed` など) を読み、直して push し直す。
- runtime に効く入力が検証済みの run と同じ push (Editor だけの変更など) では、runtime の 6 構成のビルドとテストを飛ばし、キャッシュした `runtime.exe` と TypeDB で Editor と FlaUI だけを走らせる。判定は `ci.yml` の plan ジョブ。FlaUI だけ落ちて直すときは「Re-run all jobs」を使う。
- 文書だけの変更 (`paths-ignore` の対象) ではビルドの CI は走らない。File format の検査は走る。
- テストは `runtime/core/test/` と `runtime/kernel/test/` (GoogleTest)。reflection / delegate / 型システム / メモリ管理を重点に書く。
- ベンチマークは `runtime/bench/`。1 op あたりの確保回数が予算 (`alloc_budget`) を超えると CI が落ちる。詳細は `runtime/bench/README.md`。
- Editor の見た目や操作は UI テストで拾いきれないので、Windows 上で手動確認する。

### master へ入れる条件

- CIビルド成功 (エラー 0)
- 全テストが PASS
- File format の検査が通る
- `runtime.exe` がクラッシュせず起動・終了する (CI が全構成で確かめる)
- `git status` に意図しない変更がない

ClangCL は必須ゲート。MSVC が見逃す非適合を実際に拾っているので、落ちたら原因を直す。`continue-on-error` で回避しない。

### 外部資料の名前を書かない

コード・コメント・ドキュメント・コミットメッセージ・PR に、参考にした外部資料 (発表・書籍・記事・他社の製品や設計) の名前を書かない。「〇〇式」「〇〇の規則そのまま」のような出典の明示も同じ。設計の説明は資料名を出さずに自分の言葉で書く。

- 禁止する名前は共通の git フック (`noxitro/github-templates` の `git-hooks/external-names.sha256`) にハッシュで置いてあり、フック (commit-msg / pre-commit / pre-push) と CI の Secret scan が検査する。
- 新しく避けたい名前が出たら `python3 ~/.config/nox/github-templates/git-hooks/check-external-names.py --hash '<名前>'` の出力を github-templates の `external-names.sha256` に追記する (PR で入れる)。名前そのものはコメントにも書かない。
- ユーザーの所属先など、リポジトリにハッシュでも置かない名前は手元の非公開リスト (`~/.config/nox/private-names.sha256`) にあり、フックが `[PRIVATE-NAME]` で止める。止まったら、その語を消して書き直す (リストの中身を調べたり、該当語を推測して書いたりしない)。
- 手元の資料は `docs/references/local/` (`.gitignore` 済み) に置き、コミットしない。

### ファイル形式

文字コードは BOM なしの UTF-8 に統一している。改行コード (CRLF / LF) はファイルごとに混在している。**書き換えるときは改行コードを元の形式のまま保つ。**

- 例外 (BOM 付き) は PowerShell スクリプトなど。一覧と理由は `.github/scripts/bom_policy.py` (`.editorconfig` と揃えてある)。
- VS がプロジェクトファイルを保存し直したときなどに BOM が付いたら、`python3 .github/scripts/strip-bom.py <パス>` で外す。
- 変更後は `git diff --stat` の行数が実際の編集量と釣り合うか確かめる。
- 手元では `python3 .github/scripts/check-file-format.py --base origin/master` で確かめられる (CI の File format と同じ検査)。壊したら、直すコミットを足せば通る。
- 意図して形式を変えるときは、コミットメッセージに `Format-Change: <パス or glob>` の行を書く。

## 環境とビルド

現在は Windows のみ実装済み (Linux / Android / macOS ではまだビルド・実行できない)。Runtime は Linux / Android / macOS への対応を予定しているので、Windows 専用を前提にした設計をしない。OS 固有の型やヘッダーは実装ファイルに閉じ込め、公開ヘッダーへ持ち込まない。

ビルドには Visual Studio (C++ ワークロード) と .NET SDK を使う。

### Runtime (C++)

`runtime/runtime.slnx`。

- `PlatformToolset` / `CharacterSet` は `runtime/Directory.Build.props` で一括管理し、vcxproj には書かない (VS のプロパティページで変えると書き戻されるので、その行は消す)。
- `property_sheet/*.props` は `Microsoft.Cpp.props` の後に読まれるため、ツールセットを置いても切り替わらない。コンパイラ・リンカの設定はこちらに置く。

### ReflectionGenerator (C#)

C++ ビルドの前に、`runtime/bin/source` のコード生成器 (CustomTask と ReflectionGenerator) を建てて `runtime/bin/` に置く必要がある。リポジトリ直下の `startup.ps1` (ダブルクリック用は `startup.bat`) がこれを行い、CI (`setup-nox-build`) も同じスクリプトを呼ぶ。`dotnet build` だけでは `runtime/bin/` へ配置されない。

```powershell
./startup.ps1
```

生成器のソースが配置済みのバイナリより新しいと、`reflection_generated` のビルドがエラー (`NoxCheckCodeGenerator`) で止まる。そのときは `startup.ps1` を実行し直す。

### Editor (WPF)

`Editor/Studio.slnx`。

```powershell
dotnet build Editor/Studio.Wpf/Studio.Wpf.csproj
dotnet build Editor/Studio.slnx
```

## Runtime (C++) コーディング規約

原則として [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html) に従う。ただし「最優先ルール」と以下の例外が優先する。

- マクロ: `NOX_` 接頭辞の `UPPER_SNAKE_CASE`

定数・列挙子は `k_snake_case` / 接頭辞なし `PascalCase` からの移行中。触ったファイルでは `kPascalCase` へ寄せてよいが、無関係な箇所の一括リネームはしない。

型特性 (型から bool・値・型を 1 つ求めるメタ関数) は標準ライブラリと同じ `snake_case` にし、`std::` の型特性と並べて読めるようにする。

- bool は `is_xxx_v` / `has_xxx_v`、値は `xxx_v`、型の変換は `xxx_t`。`kPascalCase` の定数規則は当てはめない。
- 構造体で実装するときは `detail::is_xxx` / `detail::xxx` に置き、結果はメンバ `::value` / `::type` で返す (`std::true_type` などを継承してよい)。detail 内の変数テンプレートも `_v` を付ける。
- 利用者に特殊化させる拡張点は、公開の構造体 `is_xxx` のままでよい (`nox::reflection::is_only_attribute`)。
- concept は `PascalCase` (`TupleLike`)。`Enum` / `Class` / `Char` のようにキーワードと衝突する名前があるため。
- 複数のメンバを持つ解析用のクラス (`detail::FunctionSignature` など) は通常のクラスとして `PascalCase`。メンバ型 (`ResultType` / `ClassType` など) も `PascalCase` で、bool のメンバは `is_xxx`。

### 例外 (既存コードに合わせ、Google と異なる)

- インデントはタブ。
- 波括弧は独立行 (Allman)。
- インクルードガードは `#pragma once`。
- 拡張子は `.h` / `.cpp`。
- 行長の上限は設けない (目安 120 桁)。
- コメントは日本語の Doxygen 形式 (`/// @brief` など) でよい。

### 他プロジェクトのヘッダのインクルード

依存先のプロジェクト (kernel / reflection / core / render / sound など) は、C# のプロジェクト参照のように「参照したら全部見える」ものとして扱う。依存はファイル単位ではなくプロジェクト単位で管理する。各 module の作業者が、依存先のどのヘッダが要るかを都度意識しなくて済むようにするためと、依存先のプロジェクトが増えたときに、それを使う全ファイルへ include を書き足さずに済むようにするため。Google の「使うものを直接 include する」方針より優先する。

- 依存先は、その代表ヘッダ (`kernel/kernel.h`、`reflection/reflection.h`、`core/core.h` など) だけを、各プロジェクトの `pch.h` から include する。個別のヘッダ (`kernel/vector.h` など) やサブフォルダ (`kernel/memory/` など) は直接 include しない。代表ヘッダも `pch.h` 以外 (ヘッダや `.cpp`) からは include しない。
- 依存先に、外から使いたいのに代表ヘッダに載っていないものがあれば、個別に include せず代表ヘッダへ足す。
- テストプロジェクト (`core_test` など) も同じ。テスト対象の内部ヘッダが要るときも、個別に include せず対象の代表ヘッダへ足す。
- 標準ライブラリも kernel の一部として扱い、kernel 以外のプロジェクトは標準ヘッダ (`<span>` など) を直接 include しない。要るものが足りなければ `kernel.h` の「標準ライブラリ」の一覧へ足す。MSVC の標準ライブラリが連鎖して読み込むものに頼らず、使うものは一覧に明示する (libstdc++ / libc++ では連鎖の範囲が違い、移植時にコンパイルエラーになる)。
- kernel の中は、従来どおり使う標準ヘッダと kernel のヘッダを直接 include する。
- 例外は OS の API を包むヘッダ (`kernel/win64_api.h` / `kernel/win64_socket.h`) と、OS・外部ライブラリのヘッダ (`<d3d12.h>`、`<gtest/gtest.h>` など)。
- 例外は PCH を使わない翻訳単位 (`test_support/test_new_delete.cpp`)。必要な標準ヘッダを直接 include する。
- 例外は ReflectionGenerator の解析の起点 `reflection_generated/reflect.cpp`。PCH なしで単独で解析されるので、代表ヘッダを直接 include する。
- プロジェクト内のヘッダ同士 (core の中で `archetype.h` を読むなど) は、従来どおり必要なものを直接 include する。

### Reflection / 属性マクロ

`NOX_ATTR_DECLARATION` / `NOX_ATTR_DECLARE` は型専用ではない。メンバ変数・メンバ関数に加え、グローバル変数・グローバル関数など型以外の宣言にも付く。ReflectionGenerator を直すときも型専用として扱わない。

### ECS の型の置き場所と書き方

- 名前空間は「モジュール → 種類」の順にする。core は `nox` 直下 (`nox::components` / `nox::services` / `nox::systems` / `nox::entity_logics`)、ほかのモジュールは `nox::render::components` のように置く。
- 型名に種類を表す語 (`Service` / `System` / `Logic`) は付けず、種類は名前空間だけで表す (`nox::systems::Camera`)。型名を文字列で出す所 (Editor・ログ・プロファイラ) は完全修飾名を使う。種類の名前空間を 2 つ以上同時に `using namespace` しない。ヘッダでは `using namespace` しない (名前空間の別名は可)。
- 型の完全修飾名は保存データと Editor の通信に使うので、原則として変えない (旧名を読み替える仕組みができるまでは、保存データに載った後には変えられない)。
- Component / EntitySystem / EntityLogic は、テンプレートの `nox::Component<T>` / `nox::EntitySystem<T, Options...>` / `nox::EntityLogic<T, Options...>` を継承する。テンプレートでない基底 (`IComponentData` / `EntitySystemBase` / `EntityLogicBase`) は `nox::detail` に置き、Component の基底を直接継承しない (Service の基底は当面今の場所のまま。理由は `docs/runtime-ecs.md` §1、§10)。EntitySystem / EntityLogic / Service の基底は共通の印 `nox::detail::UpdaterNodeOwnerBase` を継承し、`RunAfter` / `RunBefore` の相手は登録する翻訳単位で `nox::concepts::OrderTarget` を確かめる。
- EntitySystem のメソッドは static の `OnUpdate` / `OnAdd` / `OnRemove` だけで、インスタンスは作れない。オプション (`RequireComponents` / `ExcludeComponents` / `RunAfter` / `RunBefore`) は基底のテンプレート引数に順番自由で並べる。
- EntityLogic のメソッドは、自由な名前のメンバ関数を `RegisterList` に `Register<&T::Method, Trigger, Options...>` で登録する。オプションは基底と `Register` の両方に書ける (Add / Remove の `Register` には Query を変えるオプションを書けない)。別々に登録したメソッドの間に順番の保証はない。
- メソッドの中で使うもの (Component、Service、`EntityCommands&` など) は引数で受け取り、オプションにしない。
- 利用者の System / Logic は全て Update の区間で動く (区間を選ぶ書き方は無い)。
- 詳細と決めた理由は `docs/runtime-ecs.md`。

## Editor / Runtime の構成

- Editor と Runtime は別プロセスで、TCP/IP で同期する。Runtime がクラッシュしても Editor の編集状態を壊さない設計を優先する。
- 通信は共通のバイナリプロトコルに集約し、Query / Response と RemoteObject 同期を基本単位にする。
- RemoteInstanceId は、Editor 側で作った RemoteObject が正、Runtime 側で作ったものが負。
- Editor 上の RuntimeObject は Runtime インスタンスの仮想表現で、必要なときに `Sync` を明示的に呼んで Runtime 側へ実体化・同期する。
- 自動同期・モニター同期は通信と CPU のコストが高いので、Inspector 表示中など必要な範囲に限る。連続フレーム中の不要な送信も避ける。

## ViewModel / コマンド規約

- ViewModel の基底は `NoxUI.ViewModelBase`、コマンドは `NoxUI.ViewModelCommand`。ViewModel でない通知オブジェクト (サービス等) の基底は `NoxUI.ObservableBase`。
- CommunityToolkit.Mvvm への依存は NoxUI に閉じ込め、他のプロジェクトから `CommunityToolkit.*` を直接参照しない。
- DI は `Microsoft.Extensions.DependencyInjection`。登録は `App.ConfigureServices` と各 `Core.UI.EntryBase.RegisterTypes`、View と ViewModel の結線は `noxui:ViewModelLocator.AutoWireViewModel="True"` (規約: `Views.X` → `ViewModels.XViewModel`)。
- `NoxUI.ServiceLocator` は XAML 生成の View と引数なしコンストラクタが要る場所だけで使う。コンストラクタ注入で済むところでは使わない。

## WPF テーマ適用規約

- メニュー、コンテキストメニュー、ポップアップ、ツールチップ、ドロップダウンを含む可視 UI のすべてにテーマを当て、WPF 既定色へ落とさない。
- 色は必ず `Nox.Brush.*` への `DynamicResource` で指定する (背景・境界線・前景色・ホバー・選択状態)。View / Control で色値 (`#RRGGBB` など) を直書きしたり `SolidColorBrush` を生成したりしない。ViewModel / code-behind でも `SystemColors.*Brush`・`Brushes.*`・`new SolidColorBrush(...)` で UI 色を返さない。
- 例外: `Themes/*.xaml` などテーマ定義ファイルでの `Color` / `SolidColorBrush` リソース定義。
- `ContextMenu` などポップアップ系は、ポップアップ側のリソーススコープで `SystemColors.*BrushKey` を明示的に上書きする (白いシステム既定色へ戻るのを防ぐ)。
- `ToolTip` / `ComboBox` / `ContextMenu` / `MenuItem` / `TreeViewItem` など既定テンプレートに戻りやすいコントロールは、グローバルまたは既存のテーマスタイルを `BasedOn` で継承する。`MenuItem` を既定テンプレートのまま足さない。
