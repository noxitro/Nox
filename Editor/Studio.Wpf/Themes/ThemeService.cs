using AvalonDock.Themes;
using Prism.Mvvm;

namespace Studio.Wpf.Themes;

/// <summary>
/// アプリケーションテーマサービス。
/// </summary>
public sealed class ThemeService : BindableBase, IThemeService
{
    private Theme _currentTheme = new NoxTheme();

    public Theme CurrentTheme
    {
        get => _currentTheme;
        private set => SetProperty(ref _currentTheme, value);
    }
}
