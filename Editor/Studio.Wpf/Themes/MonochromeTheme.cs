// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;
using AvalonDock.Themes;

namespace Studio.Wpf.Themes;

public sealed class MonochromeTheme : Theme
{
    public string DisplayName => "Monochrome";

    public override Uri GetResourceUri()
        => new("/Studio.Wpf;component/Themes/MonochromeTheme.xaml", UriKind.Relative);
}
