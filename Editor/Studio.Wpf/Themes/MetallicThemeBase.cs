// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;
using AvalonDock.Themes;

namespace Studio.Wpf.Themes;

/// <summary>
/// メタリック系 AvalonDock テーマの基底クラス。
/// 派生クラスはバリアント固有の <see cref="ResourceDictionary"/> を指す URI と
/// <see cref="MetallicThemeKind"/> を返すだけでよい。
/// </summary>
public abstract class MetallicThemeBase : Theme
{
    /// <summary>このテーマのバリアント識別子。</summary>
    public abstract MetallicThemeKind Kind { get; }

    /// <summary>表示名（メニュー等で利用）。</summary>
    public abstract string DisplayName { get; }

    /// <summary>
    /// バリアント XAML への相対 URI（例:
    /// <c>/Studio.Wpf;component/Themes/Variants/GunmetalTheme.xaml</c>）。
    /// </summary>
    protected abstract Uri VariantResourceUri { get; }

    /// <inheritdoc />
    public sealed override Uri GetResourceUri() => VariantResourceUri;
}
