using System;
using System.Collections.Generic;
using System.Linq;
using Prism.Mvvm;
using Studio.Wpf.Themes.Variants;

namespace Studio.Wpf.Themes;

/// <summary>
/// 既定実装。<see cref="MetallicThemeKind"/> をキーに辞書ベースで管理するため、
/// バリアント追加時はコンストラクタに 1 行追加するだけで済む。
/// </summary>
public sealed class ThemeService : BindableBase, IThemeService
{
    private readonly IReadOnlyDictionary<MetallicThemeKind, MetallicThemeBase> _themeMap;
    private MetallicThemeBase _currentTheme;

    public ThemeService()
    {
        MetallicThemeBase[] themes =
        {
            new GunmetalTheme(),
            new BrushedSteelTheme(),
            new BronzeTheme(),
        };

        _themeMap = themes.ToDictionary(t => t.Kind);
        AvailableThemes = themes;
        _currentTheme = _themeMap[MetallicThemeKind.Gunmetal];
    }

    public MetallicThemeBase CurrentTheme
    {
        get => _currentTheme;
        private set => SetProperty(ref _currentTheme, value);
    }

    public IReadOnlyList<MetallicThemeBase> AvailableThemes { get; }

    public void SetTheme(MetallicThemeKind kind)
    {
        if (!_themeMap.TryGetValue(kind, out var theme))
        {
            throw new ArgumentOutOfRangeException(nameof(kind), kind, "Unknown theme kind.");
        }
        CurrentTheme = theme;
    }
}
