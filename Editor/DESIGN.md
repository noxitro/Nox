# Nox Studio Design System

<!-- impeccable:design-schema 1 -->

## Direction

**Interlocking Console — 連動盤としての編集環境**

Nox Studio は一般的な「暗い IDE」ではなく、Editor と Runtime の二つの世界を安全に接続する連動盤として設計する。鉄道信号扱所の連動盤を文化的な基盤とし、シーン、RemoteObject、同期、実行を、経路・区間・信号・閉塞の情報文法で表す。比喩は装飾に使わず、状態の因果関係、操作可能性、危険な遷移を理解しやすくするために限定する。

## Mode

Operate。表現性より、走査性、一貫性、ネイティブ WPF 操作、長時間利用、低コストな描画を優先する。

## Core Principles

1. **State before chrome** — 接続、実行、同期、選択、未保存を装飾より先に知覚できる。
2. **One active route** — 現在の主要作業経路を一つ強く示し、他は静かな背景へ退かせる。
3. **Explicit transitions** — Play、Stop、Connect、Sync は状態遷移として配置し、結果と危険性を隣接表示する。
4. **Dense, not cramped** — 高密度を保ちながら、8px 系列の間隔と強い整列で視線移動を減らす。
5. **No hidden safety state** — Runtime 切断、同期待ち、エラーは色だけでなく文言、形状、位置でも表す。
6. **Low runtime cost** — 常時アニメーション、重いぼかし、巨大な影、頻繁な再テンプレート化を避ける。

## Visual World

### Material

- マットな制御卓。面は平坦で、深度は背景・作業面・浮上面の3段階に限定する。
- 細い区画線と短いアクセント線で領域を接続する。
- 角丸は小さく、カードの集合ではなく一枚の計器盤として見せる。
- グラデーション、ガラス表現、装飾的な発光は使用しない。

### Color Roles

実値は各テーマ辞書で定義し、View は必ず `Nox.Brush.*` の `DynamicResource` を参照する。

- **Foundation:** WindowBackground、PanelBackground、SurfaceBackground。
- **Track:** GridLine、ControlBorder。構造と接続を示す低コントラスト線。
- **Route active:** Accent。選択済み経路、フォーカス、実行中の主要状態。
- **Safe/ready:** Success 系セマンティックトークン。接続済み、同期済み、完了。
- **Caution:** Warning 系セマンティックトークン。未同期、保留、変更あり。
- **Stop/fault:** Danger。停止要求、切断、失敗。通常アクションには使わない。
- すべての状態はラベルまたはアイコン形状を併記する。

### Typography

- UI 本文は Windows ネイティブの `Segoe UI Variable Text` を第一候補とする。
- 状態値、識別子、カウンターは `Cascadia Mono` を第一候補とする。
- 12px を標準、11px を補助、13px を操作、14px をペイン見出しとする。
- 全大文字は短い状態ラベルのみに限定し、本文やメニューには使わない。

### Geometry

- 基本間隔: 4 / 8 / 12 / 16 / 24。
- 標準コントロール高さ: 28。高優先アクション: 32。
- 角丸: 2 / 4 / 6。主要シェルで 8 以上を使わない。
- 境界線: 原則 1px。選択・フォーカスのみ 2px 相当の視覚強度を許可する。

## Shell Composition

1. **Menu rail (30px):** 低頻度・アプリ全体の操作。File、Project、Develop、View の順に配置する。
2. **Operations rail (56–64px):** Runtime 接続、実行、停止、同期、現在状態。中央の主要状態を基準に左右へ操作を分離する。
3. **Workspace:** AvalonDock を維持。Scene を中心経路、Hierarchy を入力、Inspector を変換、Asset Browser と Trace を供給・観測として扱う。
4. **Pane headers:** タイトル、短いコンテキスト、主要アクション、補助アクションの順を全ペインで統一する。

## Interaction Grammar

- **Primary route:** Accent の短い線、選択背景、強い見出しで示す。
- **Hover:** 面の明度を1段階上げる。レイアウトやサイズを変えない。
- **Pressed:** 面を1段階下げ、境界を強くする。
- **Focus:** 2px 相当の Accent 境界。キーボード利用時に明確に表示する。
- **Disabled:** 不透明度だけに依存せず、Muted 前景と静かな境界を併用する。
- **Destructive:** Danger と明示的な動詞を使う。隣接する主要操作と十分に分離する。
- **Busy:** 常時回転アニメーションは避け、短い進行表示または状態文言を使う。

## Component Rules

### Command Button

アイコンまたは短い動詞を持つ。Primary、Secondary、Danger の3階層。アイコンのみの場合は ToolTip と AutomationProperties.Name を必須とする。

### Status Block

状態マーカー、短い状態名、必要なら補助値を横並びにする。色だけで状態を表さない。接続と実行は別ブロックにし、同一概念に見せない。

### Pane Header

左にタイトルとコンテキスト、右に検索・フィルター・主要アクションを置く。本文と境界線で分離し、ペインごとの独自カードヘッダーを作らない。

### Search Field

検索対象が分かる具体的なプレースホルダーを使う。クリア、フォーカス、空結果を一貫して扱う。検索フィールドを見出しより強くしない。

### Tree and List

選択、ホバー、フォーカスを別状態として扱う。階層インデントは一定にし、展開記号・種類・名前・状態の列順を維持する。

## Motion

- 通常操作は 80–140ms の色・不透明度変化のみ。
- レイアウトを連続的に動かさない。
- 点滅、ストロボ、無限パルスは禁止。
- `SystemParameters.ClientAreaAnimation` が無効なら遷移を省略する。

## Accessibility

- 既存 AutomationId を維持する。
- キーボードフォーカスを常に視認可能にする。
- 文字と背景は通常テキストで 4.5:1 以上を目標とする。
- 状態、警告、選択を色だけで区別しない。
- 125–200% DPI で切り詰めず、必要な領域はスクロールまたは省略記号を使う。

## Anti-Patterns

- 一般的な SaaS ダッシュボード風カードグリッド。
- 大きな角丸、過剰な余白、装飾的な影、ネオン発光。
- ラベルのないアイコン列。
- 同じ階層に複数の Accent ボタン。
- ペインごとに異なる検索・見出し・ContextMenu スタイル。
- Runtime と Editor の状態を一つの曖昧な「オンライン」表示へ統合すること。
