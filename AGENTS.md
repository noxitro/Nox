# Nox Game Engine

Nox は、カスタムのリフレクション生成システムと WPF ベースのエディタを備えた Windows 向け C++ ゲームエンジンです。

このドキュメントを最初に参照してください。
ただし、実装詳細・現在のエラー・実ファイルの状態は、対象ファイルやビルドログを確認して判断してください。

**このファイルがエージェント向け規約の正典です。** `CLAUDE.md` と
`.github/copilot-instructions.md` はここへの参照だけを置いています。
規約を足すときは必ずこのファイルを直してください。複製すると必ず片方が腐ります。

## 作業ルール(エージェント共通)

### ブランチ

長命なブランチは `master` ただ 1 本です。

- エージェントは `work/<タスク名>` を切り、**検証が通ってから** `master` へマージして、ブランチを削除します。寿命は数時間で、常設のブランチを増やしません。
- ユーザーは `master` へ直接コミットします。実験的な作業だけ `user/<topic>` を切ります。
- **`master` への force-push は禁止です。** ユーザーのコミットが迷子になります。
- 過去に `master` と長命な作業ブランチを併存させて、同じ内容が別 SHA で二重に積まれた事故があります。だから 1 本に絞っています。

### ワークツリー

**エージェントはメインのチェックアウトでビルドしません。** ユーザーが Visual Studio で作業しているので、ビルド出力と生成物の取り合いになり、実際にファイルロックで長時間ブロックされたことがあります。

- `git worktree add` で専用のツリーを作り、編集とコミットはそこで行います。検証は下の「検証は CI で行う」に従います。
- 以下はローカルでビルドする場合の注意です。
- `OutDir` は `$(SolutionDir)build\` なのでワークツリーごとに独立します。vcpkg は環境変数 `NOX_VCPKG_INSTALLED_DIR` でメインのものを共有できます。
- 生成器のバイナリ (`runtime/bin/`) は `.gitignore` 済みなのでコピーが要りますが、**同ディレクトリには追跡ファイルも混ざっています**。まとめて上書きすると巻き添えで壊すので、コピー後に `git status` を確認してください。
- メインのチェックアウトで行ってよいのは git 操作 (commit / merge / push) だけです。

### 検証は CI で行う

**エージェントは原則としてローカルでビルド・テストを走らせません。** 6 構成のフルビルドはユーザーの PC を長時間占有し、Visual Studio での作業を妨げるためです。

- `work/<タスク名>` を push すると、CI (`.github/workflows/ci.yml`) が 6 構成のビルドと全テストを走らせます。「master へ入れる条件」の判定にはこの結果を使います。
- `work/*` ブランチの push はこの規約で許可済みです。確認は要りません。
- CI が落ちたら `gh run view <run-id> --log-failed` でログを読み、直して push し直します。
- ローカルでビルド・テストするのは、ユーザーが明示的に頼んだときだけです。
- 文書だけの変更 (`**.md` / `docs/**` など `paths-ignore` の対象) は CI が走らないので、ビルド検証は要りません。

### マージ

- **マージ前に `git status` が綺麗なことを確認します。** ユーザーが作業中だとマージが中断します。汚れていたら手を止めて報告してください。
- 着手時に「どのファイルを触るか」を宣言すると、ユーザー側が避けられて衝突しません。

### master へ入れる条件

- **6 構成すべてがビルド成功 (exit 0 / エラー 0)**: MSVC(v145) / ClangCL × Debug / Release / Master (CI で確認)
- **全テストが PASS** (CI で確認)
- **意図しない変更がないこと** (`git status` で確認)

ClangCL は必須ゲートです。MSVC が見逃す非適合を実際に拾った実績があるので、落ちたら原因を直してください。`continue-on-error` で回避しないこと。
Master 構成は `NOX_ASSERT` も `NOX_DEVELOP` も消えるため、Debug / Release では出ない警告やエラーが出ます。省略しないでください。

### ファイル形式

**このリポジトリは BOM の有無も改行コード (CRLF / LF) も混在しています。**

- ファイルを書き換えるときは、**そのファイルの元の形式を必ず保ってください。** 一括変換すると差分が全行になり、レビューも履歴も追えなくなります。
- 実際に「29 ファイルの BOM 剥がれ」「ヘッダ 1 本の CRLF 一括変換」を起こしています。
- `.gitattributes` で git 側の改行変換は無効化してありますが、**書き換えツール自身が壊すのは防げません。**
- 変更後は `git diff --stat` を見て、行数が実際の編集量と釣り合っているか確認してください。

## 最優先ルール

- CPU/メモリパフォーマンスを最優先とする。設計・実装・レビューでは常に実行時コストを優先して判断する。
- 動的メモリ確保（ヒープアロケーション）は可能な限り撤廃する。必要な場合も回数・寿命・配置を厳密に管理し、連続フレーム中の不要な確保を避ける。
- Runtime（C++）は例外を使用しない。エラー処理は戻り値・アサート・ログなどで明示的に行う。
- Windows 専用。Linux/macOS でのビルドや実行は不可。
- Visual Studio (C++ ワークロード) と必要な .NET SDK を使用。
- C++ ビルド前にリフレクション生成を完了させる。

## Runtime (C++) コーディング規約

原則として [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html) に従う。
ただし上の「最優先ルール」と以下の例外が優先する。

### 命名 (Google に従う)

- 型 (class / struct / enum / 型エイリアス / concept): `PascalCase`
- 関数・メンバ関数: `PascalCase`
- 変数・引数・struct のデータメンバ: `snake_case`
- class の非公開データメンバ: `snake_case_` (末尾アンダースコア)
- 定数 (`constexpr` / `const` の名前空間スコープ・静的メンバ) と列挙子: `kPascalCase`
- 名前空間: `snake_case`、ファイル名: `snake_case`
- マクロ: `NOX_` 接頭辞の `UPPER_SNAKE_CASE`

定数・列挙子は `k_snake_case` / 接頭辞なし `PascalCase` からの移行中。触ったファイルでは `kPascalCase` へ寄せてよいが、無関係な箇所の一括リネームはしない。

### 例外 (既存コードに合わせ、Google と異なる)

- インデントはタブ。
- 波括弧は独立行 (Allman)。
- インクルードガードは `#pragma once`。
- 拡張子は `.h` / `.cpp`。
- 行長の上限は設けない (目安 120 桁)。
- コメントは日本語の Doxygen 形式 (`/// @brief` など) でよい。
- C++ 例外は使わない (Google と同じ)。

## Reflection / 属性マクロ規約

- `NOX_ATTR_DECLARATION` / `NOX_ATTR_DECLARE` マクロは、メンバ変数・メンバ関数だけでなく、グローバル変数・グローバル関数にも付与できる属性マクロである。
- 対象は `class` / `struct` などの型宣言に限定しない。
- 型以外の宣言全般も対象とする。
- ReflectionGenerator の修正時は、これらのマクロを型専用として扱わない。

## ビルド

### Runtime (C++)

- ソリューション: `runtime/runtime.slnx`
- `PlatformToolset` / `CharacterSet` は `runtime/Directory.Build.props` で一括管理します。vcxproj には書きません (VS のプロパティページで変えると書き戻されるので、その行は消す)。
  - `property_sheet/*.props` は `Microsoft.Cpp.props` の後に読まれるため、ツールセットを置いても切り替わりません。コンパイラ・リンカの設定はこちらに置きます。

### ReflectionGenerator (C#)

- 位置: `runtime/bin/source/ReflectionGenerator/`
- ソリューション: `ReflectionGenerator.slnx`
- コマンド:

```powershell
dotnet build ReflectionGenerator.slnx
```

- C++ ビルドのプレビルドでも実行されるが、問題切り分け時は手動実行を優先。

### Editor (WPF)

- 位置: `Editor/`
- 主ソリューション: `Studio.slnx`
- 主プロジェクト:
  - `Studio.Wpf` WPF エディタ
  - `Core.UI` 共通 View / ViewModel
  - `NoxUI` ViewModelBase / ViewModelCommand
- 代表コマンド:

```powershell
dotnet build Editor/Studio.Wpf/Studio.Wpf.csproj
dotnet build Editor/Studio.slnx
```

## 検証

- 必須: `runtime.exe` がクラッシュせず起動・終了すること。
- C++ テスト:
  - runtime/core/test/ (core の GoogleTest)
  - runtime/kernel/test/ (kernel の GoogleTest)
  - 重点: reflection, delegate, type system, memory management
- Editor は自動 UI テストが限定的なため、Windows 上の手動確認を基本とする。

## エディタと Runtime の責務

- エディタ (`Studio.Wpf`): シーン編集、アセット管理、インスペクタ、ログ表示、Runtime 操作 UI。
- Runtime (C++): ゲームループ、レンダリング、シーン実行、リフレクション。
- 通信: TCP/IP。

## Editor / Runtime リモート設計方針

- Editor と Runtime はプロセスを分離し、TCP/IP 経由で同期する。Runtime クラッシュが Editor の編集状態を破壊しない設計を優先する。
- リモート通信は共通のバイナリプロトコルに集約し、Query / Response と RemoteObject 同期を基本単位にする。
- Editor 側で作成した RemoteObject は正の RemoteInstanceId、Runtime 側で作成した RemoteObject は負の RemoteInstanceId を使用する。
- Editor 上の RuntimeObject は Runtime インスタンスの仮想表現として扱い、必要なタイミングで `Sync` を明示的に呼び出して Runtime 側に実体化・同期する。
- 自動同期やモニター同期は通信・CPU コストが高いため、Inspector 表示中など必要範囲に限定する。連続フレーム中の不要送信・不要アロケーションを避ける。

## ViewModel / コマンド規約

- ViewModel 基底は `NoxUI.ViewModelBase` を使用。
- コマンドは `NoxUI.ViewModelCommand` を使用。
- MVVM ツールキット (CommunityToolkit.Mvvm) への依存は NoxUI 側に閉じ込める。他プロジェクトから `CommunityToolkit.*` を直接参照しない。
- ViewModel でない通知オブジェクト (サービス等) の基底は `NoxUI.ObservableBase`。
- DI は `Microsoft.Extensions.DependencyInjection`。登録は `App.ConfigureServices` と各 `Core.UI.EntryBase.RegisterTypes`、View と ViewModel の結線は `noxui:ViewModelLocator.AutoWireViewModel="True"` (規約: `Views.X` → `ViewModels.XViewModel`)。
- `NoxUI.ServiceLocator` は XAML 生成の View と引数なしコンストラクタが要る場所だけで使う。コンストラクタ注入で済むところでは使わない。

## WPF テーマ適用規約

- メニュー、コンテキストメニュー、ポップアップ、ツールチップ、ドロップダウンを含む可視 UI 全面にテーマリソースを適用し、WPF 既定色へのフォールバックを避ける。
- WPF の View / Control 実装で色値（`#RRGGBB` など）の直書きや `SolidColorBrush` の直接生成を行わず、背景・境界線・前景色・ホバー・選択状態は必ず `Nox.Brush.*` への `DynamicResource` バインディングを使う。
- ViewModel / code-behind でも `SystemColors.*Brush`、`Brushes.*`、`new SolidColorBrush(...)` などで UI 色を直接返さない。色は View 側のテーマリソースで解決する。
- 例外として、`Themes/*.xaml` などテーマ定義ファイル内での `Color` / `SolidColorBrush` リソース定義は許容する。
- `ContextMenu` などポップアップ系コントロールでは、ポップアップ側リソーススコープに `SystemColors.*BrushKey` の上書きを明示し、白いシステム既定色への戻りを防ぐ。
- `ToolTip` / `ComboBox` / `ContextMenu` / `MenuItem` / `TreeViewItem` など WPF 既定テンプレートに戻りやすいコントロールは、グローバルテーマスタイルまたは既存テーマスタイルを `BasedOn` で継承して使用する。
- メニュー項目追加時は既定 `MenuItem` テンプレートを未設定のまま使わず、テーマ対応済み `MenuItem` スタイルを定義または再利用する。
