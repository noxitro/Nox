using System;

namespace Studio.Wpf;

public static class ThemeSettingsViewService
{
    private static Action? _show;

    public static void Register(Action show)
    {
        _show = show;
    }

    public static void Unregister(Action show)
    {
        if (_show == show)
        {
            _show = null;
        }
    }

    public static void Show()
    {
        _show?.Invoke();
    }
}
