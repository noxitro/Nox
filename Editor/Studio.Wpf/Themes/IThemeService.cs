// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System.ComponentModel;
using AvalonDock.Themes;

namespace Studio.Wpf.Themes;

public sealed record ThemeOption(string Key, string DisplayName, bool IsCustom = false);

/// <summary>
/// アプリケーションテーマの取得を行うサービス。
/// 実装は <see cref="INotifyPropertyChanged"/> を介して
/// <see cref="CurrentTheme"/> 変更を通知すること。
/// </summary>
public interface IThemeService : INotifyPropertyChanged
{
    /// <summary>現在適用中の AvalonDock テーマ。</summary>
    Theme CurrentTheme { get; }

    IReadOnlyList<ThemeOption> AvailableThemes { get; }

    IReadOnlyList<ThemeOption> BuiltInThemes { get; }

    IReadOnlyList<CustomThemeSlotInfo> CustomThemeSlots { get; }

    int MaxCustomThemeCount { get; }

    string CurrentThemeKey { get; }

    void ApplyTheme(string key);

    IReadOnlyList<ThemeBrushColor> GetThemeBrushes(string key);

    CustomThemeDefinition CreateCustomTheme(string key, string baseThemeKey, string displayName);

    CustomThemeDefinition? GetCustomTheme(string key);

    void SaveCustomTheme(CustomThemeDefinition definition, bool applyTheme);

    void DeleteCustomTheme(string key);
}
