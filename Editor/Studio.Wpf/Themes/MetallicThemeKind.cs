// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

namespace Studio.Wpf.Themes;

/// <summary>
/// メタリック系テーマのバリアント識別子。
/// 新規バリアントを追加する場合は、この enum と
/// 対応する <see cref="MetallicThemeBase"/> 派生クラスを追加するだけでよい。
/// </summary>
public enum MetallicThemeKind
{
    /// <summary>暗いガンメタル（既定）。</summary>
    Gunmetal,

    /// <summary>ヘアライン入りのブラッシュドスチール調。</summary>
    BrushedSteel,

    /// <summary>暖色系ブロンズ調。</summary>
    Bronze,
}
