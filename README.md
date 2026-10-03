# 🚀 Nox Game Engine

[![CI](https://github.com/noxitro/Nox/actions/workflows/ci.yml/badge.svg?branch=master)](https://github.com/noxitro/Nox/actions/workflows/ci.yml)
[![Secret scan](https://github.com/noxitro/Nox/actions/workflows/secret-scan.yml/badge.svg?branch=master)](https://github.com/noxitro/Nox/actions/workflows/secret-scan.yml)
[![CodeQL](https://github.com/noxitro/Nox/actions/workflows/codeql.yml/badge.svg?branch=master)](https://github.com/noxitro/Nox/actions/workflows/codeql.yml)
[![File format](https://github.com/noxitro/Nox/actions/workflows/file-format.yml/badge.svg?branch=master)](https://github.com/noxitro/Nox/actions/workflows/file-format.yml)
[![Workflow lint](https://github.com/noxitro/Nox/actions/workflows/workflow-lint.yml/badge.svg?branch=master)](https://github.com/noxitro/Nox/actions/workflows/workflow-lint.yml)
[![Benchmarks](https://img.shields.io/badge/benchmarks-dashboard-8250df)](https://noxitro.github.io/Nox/)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

**Nox** は、**ArchetypeベースのECS** を採用するハイパフォーマンスゲームエンジンです。
Runtime は現在 Windows のみ実装済みで、Linux / Android / macOS に対応予定です。
ランタイム（C++）と WPF ベースのエディタ（C#）はプロセス分離されており、Editor の安定性を保ちながら、フレームループ中の**ゼロアロケーション**を徹底することで、予測可能なリアルタイム性能を実現します。

## ✨ コア特徴 (Key Features)

- **🧩 Archetype-based ECS**:
  - Entity は Component の組み合わせごとに連続メモリ（SoA構造）に配置されます。
  - キャッシュミスを劇的に削減し、大量のエンティティ処理に最適化されています。
- **⛔ Zero-Allocation Runtime**:
  - ゲームループ（Update）中のヒープアロケーション（`malloc`/`new`）を完全に禁止。
  - メモリ断片化やガベージコレクションのオーバーヘッドを排除し、クリティカルなフレームレートを維持します。
- **🪞 強力なリフレクション (Reflection)**:
  - カスタムビルドツールによる自動リフレクション生成で、C++の静的型付けを保ったままシリアライズ・エディタ連携を実現。
- **🖥️ 分離型エディタ (WPF)**:
  - Runtime と Editor は TCP/IP で通信する別プロセスです。
  - Runtime がクラッシュしても Editor の編集状態が破損しない、堅牢なワークフローを提供します。

## 🛠️ クイックスタート (ビルド)

### 要件 (Prerequisites)
- **Windows 10 / 11**（現在ビルド・実行できるのは Windows のみ）
- **Visual Studio 2026** (C++ ワークロード)
  - Runtime の各プロジェクトは PlatformToolset `v145` を指定しています。
  - ソリューションは `.slnx` 形式のため、MSBuild 18 以降が必要です（`.sln` はリポジトリに存在しません）。
- **.NET SDK 10.0 以降**
  - Editor / ビルドツールは **.NET 10** (`net10.0` / `net10.0-windows`) を対象としています。
  - リフレクション生成器が依存する Roslyn アナライザ部分のみ `netstandard2.0` です。

### 対応コンパイラ (Toolchain)

Runtime は **C++23** (`/std:c++latest`) の x64 ビルドで、以下 2 つのツールセットを**どちらも公式サポート**しています。

| ツールセット | PlatformToolset | CI での扱い |
| --- | --- | --- |
| MSVC | `v145` | 必須ゲート (Debug / Release / Master) |
| clang-cl (VS 同梱) | `ClangCL` | 必須ゲート (Debug / Release / Master) |

- CI は `compiler x configuration` の直積 6 ジョブで `runtime.slnx` をビルドします。
- **clang-cl も必須ゲートです。** MSVC が素通りさせる非適合コード（実質機能していない `if constexpr` ガード、未使用変数、未出力関数など）を実際に検出した実績があるため、`continue-on-error` には戻さない方針です。ClangCL ジョブが落ちた場合は無効化で回避せず、原因を修正してください。
- ユニットテスト（`runtime.slnx` の `tests` フォルダ、GoogleTest）は MSVC / clang-cl の Debug / Release の 4 ジョブで実行します。Master はテスト型のリフレクションが生成されないため対象外です。
- `runtime.exe` の起動と終了は 6 ジョブすべてで確かめます。`--exit-after-frames=N` を付けて起動すると、N フレーム目にウィンドウを閉じたときと同じ経路で終了します。
- ベンチマーク（`runtime/bench`）は Release / Master の 4 構成で毎 push 計測し、[結果ページ](https://noxitro.github.io/Nox/)（GitHub Pages）に履歴を積みます。
  共有ランナーの揺れを避けるため、master で建てた exe と同じ VM で交互に走らせて比較します。
  1 op あたりのヒープ確保回数が予算を超えたときだけ CI が落ちます（詳細は [`runtime/bench/README.md`](runtime/bench/README.md)）。

### ビルド手順

1. **リフレクション生成** (C++ビルドの前に必須)
   ```powershell
   cd runtime/bin/source/ReflectionGenerator/
   dotnet build ReflectionGenerator.slnx
   ```

### 開発フックの導入

フックの本体は全リポジトリ共通で、`noxitro/github-templates` の `git-hooks/` にある
(このリポジトリには置かない)。マシンごとに 1 回だけ実行する。コミット時と push 時に、
秘密情報・ビルド成果物・ローカル絶対パスが混ざっていないかを検査するようになる。

```sh
git clone https://github.com/noxitro/github-templates ~/.config/nox/github-templates
sh ~/.config/nox/github-templates/git-hooks/install.sh
```

global の `core.hooksPath` を設定するので、`noxitro/` の全リポジトリ・全ブランチで効く。
更新は `git -C ~/.config/nox/github-templates pull` だけでよい。
以前の手順 (`sh tools/git-hooks/install.sh`) で入れていたら、このリポジトリの中で `install.sh` を
実行する。残っているローカルの `core.hooksPath` を外す (外さないと global より優先され、
フックが黙って走らなくなる)。

誤検出は `.githooks-allow` にパスを 1 行で足して除外する (理由をコメントで残すこと)。
gitleaks の誤検知は `.gitleaks.toml` の allowlist に足す。

検査は 2 段になっている。`scan.sh` が汎用のスキャナで拾えないもの
(ビルド成果物の混入、ローカル絶対パス、個人メール、MIT と両立しないライセンスの文言や
他者の著作権表示、家庭用ゲーム機の非公開 SDK の識別子、外部資料の名前) を見て、
`gitleaks` が汎用の秘密情報を見る。外部資料の名前は Python 3 を使い、無ければ手元では
飛ばして CI の Secret scan だけで検査する。gitleaks は任意だが、入れると検出できる
トークン形式が大幅に増えるので推奨する。`pre-push` は、コミットの作成者・
コミッターのメールアドレスが noreply でなければ止める。

```sh
winget install Gitleaks.Gitleaks
```

所属先など、ハッシュにしてもリポジトリに置きたくない名前は、手元だけの非公開リスト
(`~/.config/nox/private-names.sha256`) に入れる。フックが同じように止める
(CI には無いので、止めるのは手元のフックだけ)。

```sh
python3 ~/.config/nox/github-templates/git-hooks/check-external-names.py --add-private '<名前>'
```

Claude Code on the web のセッションでは `.claude/hooks/session-start.sh`
(SessionStart フック) が同じ導入を自動で行い、gitleaks も固定バージョンで入れる。
クラウドからの push も `pre-push` で止まる。

同じ検査は CI の `Secret scan` ワークフロー (本体は github-templates の共通ワークフロー) でも走る。
手元のフックを入れ忘れても、push された内容はそちらで検査される。

ただし CI は push の後に走るので、流出そのものは防げない。
外に出る前に止まるのは `pre-push` と、GitHub 側の push protection
(Settings > Code security) の 2 つだけ。

### Visual Studio の項目テンプレート

「新しい項目の追加」に出る `nox_header` / `nox_source` / `nox_header_source` の
元は `tools/vs-templates/` にある。ライセンスヘッダ付きで .h / .cpp を作れる。
テンプレートを直したら、次を再実行して VS へ反映する (VS は再起動が要る)。

```powershell
pwsh tools/vs-templates/install.ps1
```

各フォルダを zip にして `ドキュメント\Visual Studio 18\Templates\ItemTemplates` へ
コピーする。OneDrive のバックアップが有効なら `%OneDrive%\Documents` 側も探す。
見つからないときは `-Destination` で置き場を指定する。
