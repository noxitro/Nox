// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;

namespace Studio.Wpf.Themes.Variants;

public sealed class BronzeTheme : MetallicThemeBase
{
    public override MetallicThemeKind Kind => MetallicThemeKind.Bronze;
    public override string DisplayName => "Bronze";

    protected override Uri VariantResourceUri { get; } = new Uri(
        "/Studio.Wpf;component/Themes/Variants/BronzeTheme.xaml",
        UriKind.Relative);
}
