using System;

namespace Studio.Wpf.Themes.Variants;

public sealed class GunmetalTheme : MetallicThemeBase
{
    public override MetallicThemeKind Kind => MetallicThemeKind.Gunmetal;
    public override string DisplayName => "Gunmetal";

    protected override Uri VariantResourceUri { get; } = new Uri(
        "/Studio.Wpf;component/Themes/Variants/GunmetalTheme.xaml",
        UriKind.Relative);
}
