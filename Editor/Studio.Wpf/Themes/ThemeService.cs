using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.Json;
using System.Windows;
using System.Windows.Markup;
using System.Windows.Media;
using AvalonDock.Themes;
using Prism.Mvvm;
using Studio.Wpf.Themes.Variants;

namespace Studio.Wpf.Themes;

/// <summary>
/// アプリケーションテーマサービス。
/// </summary>
public sealed class ThemeService : BindableBase, IThemeService
{
    private const string DefaultThemeKey = "Nox";
    private static readonly string[] CustomThemeKeys = ["Custom1", "Custom2", "Custom3", "Custom4"];
    private sealed record ThemeDefinition(ThemeOption Option, Func<Theme> Factory);

    private readonly ThemeDefinition[] _themes =
    [
        new(new ThemeOption("Nox", "Nox"), static () => new NoxTheme()),
        new(new ThemeOption("Gunmetal", "Gunmetal"), static () => new GunmetalTheme()),
        new(new ThemeOption("BrushedSteel", "Brushed Steel"), static () => new BrushedSteelTheme()),
        new(new ThemeOption("Bronze", "Bronze"), static () => new BronzeTheme()),
        new(new ThemeOption("Monochrome", "Monochrome"), static () => new MonochromeTheme()),
    ];

    private readonly Dictionary<string, CustomThemeStoreItem> _customThemes = new(StringComparer.Ordinal);
    private readonly string _storagePath;
    private readonly ThemeOption[] _builtInThemes;
    private ThemeOption[] _availableThemes = [];
    private CustomThemeSlotInfo[] _customThemeSlots = [];
    private Theme _currentTheme = new NoxTheme();
    private string _currentThemeKey = DefaultThemeKey;

    public ThemeService()
    {
        _builtInThemes = _themes.Select(static theme => theme.Option).ToArray();
        _storagePath = GetStoragePath();

        string startupThemeKey = LoadStore();
        RefreshThemeCollections();
        TryApplyStartupTheme(startupThemeKey);
    }

    public Theme CurrentTheme
    {
        get => _currentTheme;
        private set => SetProperty(ref _currentTheme, value);
    }

    public IReadOnlyList<ThemeOption> AvailableThemes => _availableThemes;

    public IReadOnlyList<ThemeOption> BuiltInThemes => _builtInThemes;

    public IReadOnlyList<CustomThemeSlotInfo> CustomThemeSlots => _customThemeSlots;

    public int MaxCustomThemeCount => CustomThemeKeys.Length;

    public string CurrentThemeKey
    {
        get => _currentThemeKey;
        private set => SetProperty(ref _currentThemeKey, value);
    }

    public void ApplyTheme(string key)
        => ApplyThemeCore(key, persist: true);

    public IReadOnlyList<ThemeBrushColor> GetThemeBrushes(string key)
        => ExtractBrushValues(CreateThemeDictionaryForKey(key));

    public CustomThemeDefinition CreateCustomTheme(string key, string baseThemeKey, string displayName)
    {
        ValidateCustomThemeKey(key);
        ThemeDefinition baseTheme = GetBuiltInThemeDefinition(baseThemeKey);
        string resolvedDisplayName = string.IsNullOrWhiteSpace(displayName)
            ? $"Custom Theme {GetSlotIndex(key)}"
            : displayName.Trim();

        return new CustomThemeDefinition(
            key,
            resolvedDisplayName,
            baseTheme.Option.Key,
            GetThemeBrushes(baseTheme.Option.Key));
    }

    public CustomThemeDefinition? GetCustomTheme(string key)
    {
        ValidateCustomThemeKey(key);
        if (_customThemes.TryGetValue(key, out CustomThemeStoreItem? customTheme) == false)
        {
            return null;
        }

        return CreateDefinition(customTheme);
    }

    public void SaveCustomTheme(CustomThemeDefinition definition, bool applyTheme)
    {
        ValidateCustomThemeKey(definition.Key);
        ThemeDefinition baseTheme = GetBuiltInThemeDefinition(definition.BaseThemeKey);
        Dictionary<string, string> brushColors = NormalizeBrushValues(definition.Brushes, baseTheme.Option.Key);
        _customThemes[definition.Key] = new CustomThemeStoreItem
        {
            Key = definition.Key,
            DisplayName = string.IsNullOrWhiteSpace(definition.DisplayName)
                ? $"Custom Theme {GetSlotIndex(definition.Key)}"
                : definition.DisplayName.Trim(),
            BaseThemeKey = baseTheme.Option.Key,
            BrushColors = brushColors,
        };

        RefreshThemeCollections();
        if (applyTheme)
        {
            ApplyThemeCore(definition.Key, persist: true);
            return;
        }

        PersistStore(CurrentThemeKey);
    }

    public void DeleteCustomTheme(string key)
    {
        ValidateCustomThemeKey(key);
        if (_customThemes.Remove(key) == false)
        {
            return;
        }

        RefreshThemeCollections();
        if (string.Equals(CurrentThemeKey, key, StringComparison.Ordinal))
        {
            ApplyThemeCore(DefaultThemeKey, persist: false);
        }

        PersistStore(CurrentThemeKey);
    }

    private void ApplyThemeCore(string key, bool persist)
    {
        ResourceDictionary themeDictionary = CreateThemeDictionaryForKey(key);
        Theme nextTheme = CreateAvalonDockThemeForKey(key);
        UpdateApplicationResources(themeDictionary);
        CurrentTheme = nextTheme;
        CurrentThemeKey = key;

        if (persist)
        {
            PersistStore(key);
        }
    }

    private string LoadStore()
    {
        if (File.Exists(_storagePath) == false)
        {
            return DefaultThemeKey;
        }

        try
        {
            ThemeCustomizationStore? store = JsonSerializer.Deserialize<ThemeCustomizationStore>(File.ReadAllText(_storagePath));
            if (store == null)
            {
                return DefaultThemeKey;
            }

            foreach (CustomThemeStoreItem item in store.CustomThemes)
            {
                if (CustomThemeKeys.Contains(item.Key, StringComparer.Ordinal) == false)
                {
                    continue;
                }

                if (_themes.Any(theme => string.Equals(theme.Option.Key, item.BaseThemeKey, StringComparison.Ordinal)) == false)
                {
                    continue;
                }

                _customThemes[item.Key] = item;
            }

            return string.IsNullOrWhiteSpace(store.ActiveThemeKey) ? DefaultThemeKey : store.ActiveThemeKey;
        }
        catch (IOException)
        {
            return DefaultThemeKey;
        }
        catch (JsonException)
        {
            return DefaultThemeKey;
        }
    }

    private void PersistStore(string activeThemeKey)
    {
        Directory.CreateDirectory(Path.GetDirectoryName(_storagePath)!);
        ThemeCustomizationStore store = new()
        {
            ActiveThemeKey = activeThemeKey,
            CustomThemes = _customThemes
                .Values
                .OrderBy(static item => item.Key, StringComparer.Ordinal)
                .Select(static item => new CustomThemeStoreItem
                {
                    Key = item.Key,
                    DisplayName = item.DisplayName,
                    BaseThemeKey = item.BaseThemeKey,
                    BrushColors = new Dictionary<string, string>(item.BrushColors, StringComparer.Ordinal),
                })
                .ToList(),
        };

        JsonSerializerOptions options = new()
        {
            WriteIndented = true,
        };
        File.WriteAllText(_storagePath, JsonSerializer.Serialize(store, options));
    }

    private void RefreshThemeCollections()
    {
        ThemeOption[] nextAvailableThemes =
            [
                .. _builtInThemes,
                .. CustomThemeKeys
                    .Where(key => _customThemes.ContainsKey(key))
                    .Select(key => new ThemeOption(key, _customThemes[key].DisplayName, IsCustom: true)),
            ];

        CustomThemeSlotInfo[] nextCustomThemeSlots =
            [
                .. CustomThemeKeys.Select((key, index) =>
                {
                    if (_customThemes.TryGetValue(key, out CustomThemeStoreItem? customTheme))
                    {
                        return new CustomThemeSlotInfo(key, index + 1, true, customTheme.DisplayName, customTheme.BaseThemeKey);
                    }

                    return new CustomThemeSlotInfo(key, index + 1, false, $"Custom Theme {index + 1}", DefaultThemeKey);
                }),
            ];

        _availableThemes = nextAvailableThemes;
        _customThemeSlots = nextCustomThemeSlots;
        RaisePropertyChanged(nameof(AvailableThemes));
        RaisePropertyChanged(nameof(CustomThemeSlots));
    }

    private void TryApplyStartupTheme(string themeKey)
    {
        try
        {
            ApplyThemeCore(themeKey, persist: false);
            PersistStore(CurrentThemeKey);
        }
        catch (ArgumentException)
        {
            ApplyThemeCore(DefaultThemeKey, persist: false);
            PersistStore(CurrentThemeKey);
        }
        catch (InvalidOperationException)
        {
            ApplyThemeCore(DefaultThemeKey, persist: false);
            PersistStore(CurrentThemeKey);
        }
    }

    private ResourceDictionary CreateThemeDictionaryForKey(string key)
    {
        if (TryGetBuiltInThemeDefinition(key, out ThemeDefinition? builtInTheme))
        {
            return CreateThemeDictionary(builtInTheme!.Factory().GetResourceUri());
        }

        if (_customThemes.TryGetValue(key, out CustomThemeStoreItem? customTheme))
        {
            ThemeDefinition baseTheme = GetBuiltInThemeDefinition(customTheme.BaseThemeKey);
            ResourceDictionary compositeDictionary = new();
            compositeDictionary.MergedDictionaries.Add(CreateThemeDictionary(baseTheme.Factory().GetResourceUri()));
            compositeDictionary.MergedDictionaries.Add(CreateCustomOverrideDictionary(customTheme));
            return compositeDictionary;
        }

        throw new ArgumentException($"Unknown theme key: {key}", nameof(key));
    }

    private Theme CreateAvalonDockThemeForKey(string key)
    {
        if (TryGetBuiltInThemeDefinition(key, out ThemeDefinition? builtInTheme))
        {
            return builtInTheme!.Factory();
        }

        if (_customThemes.TryGetValue(key, out CustomThemeStoreItem? customTheme))
        {
            return GetBuiltInThemeDefinition(customTheme.BaseThemeKey).Factory();
        }

        throw new ArgumentException($"Unknown theme key: {key}", nameof(key));
    }

    private static ResourceDictionary CreateThemeDictionary(Uri themeResourceUri)
    {
        try
        {
            return new ResourceDictionary
            {
                Source = themeResourceUri,
            };
        }
        catch (IOException ex)
        {
            throw new InvalidOperationException($"Failed to load theme resource: {themeResourceUri}", ex);
        }
        catch (InvalidOperationException ex)
        {
            throw new InvalidOperationException($"Failed to load theme resource: {themeResourceUri}", ex);
        }
        catch (XamlParseException ex)
        {
            throw new InvalidOperationException($"Failed to load theme resource: {themeResourceUri}", ex);
        }
    }

    private static IReadOnlyList<ThemeBrushColor> ExtractBrushValues(ResourceDictionary dictionary)
    {
        List<ThemeBrushColor> brushes = new(ThemePaletteCatalog.Brushes.Count);
        foreach (ThemeBrushDescriptor descriptor in ThemePaletteCatalog.Brushes)
        {
            if (TryGetResource(dictionary, descriptor.ResourceKey, out object? resource) == false ||
                resource is not SolidColorBrush brush)
            {
                throw new InvalidOperationException($"Theme resource '{descriptor.ResourceKey}' is missing or is not a SolidColorBrush.");
            }

            brushes.Add(new ThemeBrushColor(descriptor.ResourceKey, descriptor.DisplayName, descriptor.Category, ToHex(brush.Color)));
        }

        return brushes;
    }

    private static ResourceDictionary CreateCustomOverrideDictionary(CustomThemeStoreItem customTheme)
    {
        ResourceDictionary dictionary = new();
        foreach (ThemeBrushDescriptor descriptor in ThemePaletteCatalog.Brushes)
        {
            string hexValue = customTheme.BrushColors.TryGetValue(descriptor.ResourceKey, out string? savedValue)
                ? savedValue
                : throw new InvalidOperationException($"Custom theme '{customTheme.Key}' is missing brush '{descriptor.ResourceKey}'.");

            dictionary[descriptor.ResourceKey] = CreateBrush(ParseHexColor(hexValue));
        }

        foreach ((object key, string brushKey) in ThemePaletteCatalog.DerivedBrushMappings)
        {
            dictionary[key] = dictionary[brushKey];
        }

        return dictionary;
    }

    private Dictionary<string, string> NormalizeBrushValues(IReadOnlyList<ThemeBrushColor> brushes, string baseThemeKey)
    {
        Dictionary<string, string> values = new(StringComparer.Ordinal);
        foreach (ThemeBrushColor brush in brushes)
        {
            values[brush.ResourceKey] = ToHex(ParseHexColor(brush.HexValue));
        }

        if (ThemePaletteCatalog.Brushes.Any(descriptor => values.ContainsKey(descriptor.ResourceKey) == false))
        {
            foreach (ThemeBrushColor baseBrush in GetThemeBrushes(baseThemeKey))
            {
                values.TryAdd(baseBrush.ResourceKey, baseBrush.HexValue);
            }
        }

        return values;
    }

    private static bool TryGetResource(ResourceDictionary dictionary, object key, out object? resource)
    {
        if (dictionary.Contains(key))
        {
            resource = dictionary[key];
            return true;
        }

        for (int i = dictionary.MergedDictionaries.Count - 1; i >= 0; --i)
        {
            if (TryGetResource(dictionary.MergedDictionaries[i], key, out resource))
            {
                return true;
            }
        }

        resource = null;
        return false;
    }

    private static SolidColorBrush CreateBrush(Color color)
    {
        SolidColorBrush brush = new(color);
        brush.Freeze();
        return brush;
    }

    private static string ToHex(Color color)
        => $"#{color.A:X2}{color.R:X2}{color.G:X2}{color.B:X2}";

    private static Color ParseHexColor(string hexValue)
    {
        if (string.IsNullOrWhiteSpace(hexValue))
        {
            throw new ArgumentException("Color value is empty.", nameof(hexValue));
        }

        string normalized = hexValue.Trim();
        if (normalized.StartsWith('#'))
        {
            normalized = normalized[1..];
        }

        return normalized.Length switch
        {
            6 => Color.FromArgb(
                0xFF,
                Convert.ToByte(normalized[0..2], 16),
                Convert.ToByte(normalized[2..4], 16),
                Convert.ToByte(normalized[4..6], 16)),
            8 => Color.FromArgb(
                Convert.ToByte(normalized[0..2], 16),
                Convert.ToByte(normalized[2..4], 16),
                Convert.ToByte(normalized[4..6], 16),
                Convert.ToByte(normalized[6..8], 16)),
            _ => throw new ArgumentException($"Invalid color format: {hexValue}", nameof(hexValue)),
        };
    }

    private static string GetStoragePath()
        => Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
            "Nox",
            "Studio",
            "ThemeSettings.json");

    private bool TryGetBuiltInThemeDefinition(string key, out ThemeDefinition? definition)
    {
        definition = _themes.FirstOrDefault(theme => string.Equals(theme.Option.Key, key, StringComparison.Ordinal));
        return definition != null;
    }

    private ThemeDefinition GetBuiltInThemeDefinition(string key)
    {
        if (TryGetBuiltInThemeDefinition(key, out ThemeDefinition? definition))
        {
            return definition!;
        }

        throw new ArgumentException($"Unknown base theme key: {key}", nameof(key));
    }

    private static int GetSlotIndex(string key)
        => Array.IndexOf(CustomThemeKeys, key) + 1;

    private static void ValidateCustomThemeKey(string key)
    {
        if (CustomThemeKeys.Contains(key, StringComparer.Ordinal) == false)
        {
            throw new ArgumentException($"Unknown custom theme key: {key}", nameof(key));
        }
    }

    private CustomThemeDefinition CreateDefinition(CustomThemeStoreItem customTheme)
    {
        Dictionary<string, string> baseBrushes = GetThemeBrushes(customTheme.BaseThemeKey)
            .ToDictionary(brush => brush.ResourceKey, brush => brush.HexValue, StringComparer.Ordinal);

        return new CustomThemeDefinition(
            customTheme.Key,
            customTheme.DisplayName,
            customTheme.BaseThemeKey,
            ThemePaletteCatalog.Brushes
                .Select(descriptor => new ThemeBrushColor(
                    descriptor.ResourceKey,
                    descriptor.DisplayName,
                    descriptor.Category,
                    customTheme.BrushColors.TryGetValue(descriptor.ResourceKey, out string? value)
                        ? value
                        : baseBrushes[descriptor.ResourceKey]))
                .ToArray());
    }

    private static void UpdateApplicationResources(ResourceDictionary themeDictionary)
    {
        var dictionaries = Application.Current.Resources.MergedDictionaries;

        if (dictionaries.Count == 0)
        {
            dictionaries.Add(themeDictionary);
            return;
        }

        dictionaries[0] = themeDictionary;
    }
}
