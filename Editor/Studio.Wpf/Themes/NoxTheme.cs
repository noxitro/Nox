using System;
using AvalonDock.Themes;

namespace Studio.Wpf.Themes;

/// <summary>
/// Nox エディタ専用の AvalonDock テーマ。
/// </summary>
public sealed class NoxTheme : Theme
{
    public string DisplayName => "Nox";

    public override Uri GetResourceUri()
        => new("/Studio.Wpf;component/Themes/NoxTheme.xaml", UriKind.Relative);
}
