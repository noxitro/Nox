## 最優先ルール

- CPU/メモリ性能を最優先する。設計・実装・レビューでは常に実行時コストで判断する。
- ヒープ確保は可能な限り撤廃する。必要な場合も回数・寿命・配置を厳密に管理し、連続フレーム中の不要な確保を避ける。
- Runtime (C++) は例外を使わない。エラーは戻り値・アサート・ログで明示的に扱う。

## 言語

- PRやコードレビューなどは日本語で書く

## 作業ルール(エージェント共通)

### ブランチ

長命なブランチは `master` 1 本だけにする (作業ブランチと併存させて、同じ内容が別 SHA で二重に積まれた事故があった)。

- エージェントは `work/<タスク名>` を切り、検証が通ったら `master` へマージしてブランチを消す。セッション側で作業ブランチが指定されている場合 (クラウドのセッションなど) はそれに従い、`master` への取り込みは PR にしてユーザーに委ねる。
- ユーザーは `master` へ直接コミットし、実験だけ `user/<topic>` を切る。
- **`master` への force-push は禁止。** ユーザーのコミットが失われる。

### ワークツリー

**ユーザーのマシンでは、エージェントはメインのチェックアウトで編集しない** (Visual Studio のファイルロックで長時間止まった実績がある)。`git worktree add` で専用のツリーを作って編集・コミットし、メインで行うのは git 操作 (commit / merge / push) だけにする。着手時に触るファイルを宣言すると衝突を避けられる。

### 検証

**検証は CI に任せ、エージェントはローカルでビルド・テストしない** (ユーザーのマシンの負荷を抑えるため)。ユーザーが明示的に頼んだときだけ行い、手順は `docs/local-build.md` に従う。

- 作業ブランチを push すると CI (`.github/workflows/ci.yml`) がビルド・テスト・`runtime.exe` の起動確認を走らせる。構成の詳細は `ci.yml` 冒頭のコメント。作業ブランチ (`work/*` またはセッションで指定されたブランチ。`master` は含まない) の push は確認なしでよい。`.github` 以下を変えると Workflow lint (actionlint / ruff) も走る。
- 落ちたらログ (`gh run view <run-id> --log-failed` など) を読み、直して push し直す。
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

マージ前にメインのチェックアウトの `git status` が綺麗か確かめる。汚れていたら (ユーザーが作業中) 手を止めて報告する。

### 外部資料の名前を書かない

コード・コメント・ドキュメント・コミットメッセージ・PR に、参考にした外部資料 (発表・書籍・記事・他社の製品や設計) の名前を書かない。「〇〇式」「〇〇の規則そのまま」のような出典の明示も同じ。設計の説明は資料名を出さずに自分の言葉で書く。

- 禁止する名前は `tools/git-hooks/external-names.sha256` にハッシュで置いてあり、フック (commit-msg / pre-commit / pre-push) と CI の Secret scan が検査する。
- 新しく避けたい名前が出たら `python3 tools/git-hooks/check-external-names.py --hash '<名前>'` の出力を追記する。名前そのものはコメントにも書かない。
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

Windows 専用 (Linux / macOS ではビルド・実行できない)。Visual Studio (C++ ワークロード) と .NET SDK を使う。

### Runtime (C++)

`runtime/runtime.slnx`。

- `PlatformToolset` / `CharacterSet` は `runtime/Directory.Build.props` で一括管理し、vcxproj には書かない (VS のプロパティページで変えると書き戻されるので、その行は消す)。
- `property_sheet/*.props` は `Microsoft.Cpp.props` の後に読まれるため、ツールセットを置いても切り替わらない。コンパイラ・リンカの設定はこちらに置く。

### ReflectionGenerator (C#)

C++ ビルドの前にリフレクション生成が要る。プレビルドでも走るが、問題の切り分けでは手動で実行する。

```powershell
dotnet build runtime/bin/source/ReflectionGenerator/ReflectionGenerator.slnx
```

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

### Reflection / 属性マクロ

`NOX_ATTR_DECLARATION` / `NOX_ATTR_DECLARE` は型専用ではない。メンバ変数・メンバ関数に加え、グローバル変数・グローバル関数など型以外の宣言にも付く。ReflectionGenerator を直すときも型専用として扱わない。

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
