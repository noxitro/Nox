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
- **Visual Studio** (C++ ワークロード)
- **.NET SDK** (6.0 以降)

### ビルド手順

1. **リフレクション生成** (C++ビルドの前に必須)
   ```powershell
   cd runtime/bin/source/ReflectionGenerator/
   dotnet build ReflectionGenerator.slnx