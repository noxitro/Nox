// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;

namespace Studio.Wpf.Themes.Variants;

public sealed class BrushedSteelTheme : MetallicThemeBase
{
    public override MetallicThemeKind Kind => MetallicThemeKind.BrushedSteel;
    public override string DisplayName => "Brushed Steel";

    protected override Uri VariantResourceUri { get; } = new Uri(
        "/Studio.Wpf;component/Themes/Variants/BrushedSteelTheme.xaml",
        UriKind.Relative);
}
