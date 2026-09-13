// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System.Collections.Generic;

namespace Studio.Wpf.Themes;

public sealed record ThemeBrushColor(string ResourceKey, string DisplayName, string Category, string HexValue);

public sealed record CustomThemeDefinition(
    string Key,
    string DisplayName,
    string BaseThemeKey,
    IReadOnlyList<ThemeBrushColor> Brushes);

public sealed record CustomThemeSlotInfo(
    string Key,
    int SlotIndex,
    bool HasTheme,
    string DisplayName,
    string BaseThemeKey)
{
    public string StateText => HasTheme ? "Saved" : "Empty";
}

internal sealed record ThemeBrushDescriptor(string ResourceKey, string DisplayName, string Category);

internal sealed class ThemeCustomizationStore
{
    public string ActiveThemeKey { get; set; } = "Nox";

    public List<CustomThemeStoreItem> CustomThemes { get; set; } = [];
}

internal sealed class CustomThemeStoreItem
{
    public string Key { get; set; } = string.Empty;

    public string DisplayName { get; set; } = string.Empty;

    public string BaseThemeKey { get; set; } = "Nox";

    public Dictionary<string, string> BrushColors { get; set; } = new(System.StringComparer.Ordinal);
}
