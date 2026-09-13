# Product

<!-- impeccable:product-schema 1 -->

## Platform

adaptive

## Users

- **暫定前提:** 主対象は、Windows 上で Nox Game Engine を開発しながらシーン編集と Runtime デバッグを日常的に行うエンジン開発者。
- **暫定前提:** ゲーム制作者も利用するが、初期再構築ではエンジン内部状態を理解するユーザーの高密度な作業を優先する。

## Product Purpose

Studio は、Nox のシーン、アセット、オブジェクトプロパティ、Runtime 実行状態、診断情報を一つの編集環境で操作・観察する Windows デスクトップアプリケーション。成功とは、編集と Runtime 検証を往復する際の探索、待機、誤操作を減らし、現在の接続・同期・実行状態を常に把握できること。

## Positioning

Editor と Runtime を別プロセスに保ちながら、RemoteObject と明示的な Sync を通じて編集状態を安全に Runtime へ実体化できることが Studio 固有の中核機構。

## Operating Context

- WPF と AvalonDock による、複数ペインを並べた長時間のデスクトップ作業。
- Hierarchy、Scene、Inspector、Asset Browser を中心に、ログ、メモリプロファイラー、EngineSystem グラフ、RemoteInstance 管理を併用する。
- Runtime は別プロセスで動作し、Studio と TCP/IP で通信する。
- ユーザーは編集、同期、実行、停止、診断を短いサイクルで繰り返す。

## Capabilities and Constraints

- Windows 専用。WPF、.NET 10、AvalonDock を維持する。
- ViewModel 基底は `NoxUI.ViewModelBase`、コマンドは `NoxUI.ViewModelCommand` を使用する。
- MVVM ツールキット (CommunityToolkit.Mvvm) への依存は NoxUI 側へ閉じ込める。DI は Microsoft.Extensions.DependencyInjection。
- テーマ色は `Nox.Brush.*` の `DynamicResource` から解決し、View や code-behind で色を直接生成しない。
- Editor と Runtime のプロセス分離、明示的 Sync、RemoteInstanceId の符号規約を維持する。
- 連続フレーム中の不要な通信、監視、動的メモリ確保を避ける。
- **未決定:** 初期再構築を Studio 全体へ一括適用するか、主要ワークフローから段階導入するか。現時点では段階導入を仮定する。

## Brand Commitments

- 製品名 `Nox Studio` と Nox Game Engine の技術的・専門的な性格を維持する。
- 既存の複数テーマ機構とテーマ設定機能は維持する。

## Evidence on Hand

- 既存 UI 実装: `Studio.Wpf/MainWindow.xaml`、`Core.UI/Views/`。
- 既存テーマとトークン: `Studio.Wpf/Themes/`。
- 主要 ViewModel: `Core.UI/ViewModels/`。
- UI 自動化プロジェクト: `Studio.Wpf.UITests/`。
- 外部ユーザー調査、利用ログ、ブランドガイド、確定済みロゴ資産は未確認。将来の設計で捏造しない。

## Product Principles

1. 編集対象、選択対象、Runtime 実体、同期状態の違いを曖昧にしない。
2. 高頻度操作は少ない視線移動と短い操作距離で完了させる。
3. 高密度でも階層と重要状態を瞬時に走査できるようにする。
4. Runtime の障害や切断から Editor の編集状態を守る。
5. 診断機能を日常の編集導線から到達可能にしつつ、通常作業を圧迫させない。

## Accessibility & Inclusion

- キーボード操作、明確なフォーカス表示、AutomationProperties、十分なコントラストを維持・拡張する。
- 色だけに依存せず、接続、同期、警告、実行状態を形状・文言・アイコンでも伝える。
