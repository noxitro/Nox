# 🚀 Nox Game Engine

**Nox** は、Windows プラットフォームに特化した **ArchetypeベースのECS** を採用するハイパフォーマンスゲームエンジンです。
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
- **Windows 10 / 11**
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
- ユニットテスト（`runtime_test.slnx`）の実行は MSVC ツールチェーンのみです。

### ビルド手順

1. **リフレクション生成** (C++ビルドの前に必須)
   ```powershell
   cd runtime/bin/source/ReflectionGenerator/
   dotnet build ReflectionGenerator.slnx