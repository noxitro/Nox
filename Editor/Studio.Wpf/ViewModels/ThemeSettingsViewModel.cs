using System;
using System.Collections.ObjectModel;
using System.Linq;

namespace Studio.Wpf.ViewModels;

public sealed class ThemeSettingsViewModel : NoxUI.ViewModelBase, IDisposable
{
    public sealed class ThemeBrushEditorViewModel : NoxUI.ViewModelBase
    {
        private readonly Action _markDirty;
        private bool _isUpdating;
        private string _hexValue = "#FF000000";
        private byte _alpha = 0xFF;
        private byte _red;
        private byte _green;
        private byte _blue;
        private string _errorText = string.Empty;

        public ThemeBrushEditorViewModel(Studio.Wpf.Themes.ThemeBrushColor brush, Action markDirty)
        {
            ResourceKey = brush.ResourceKey;
            DisplayName = brush.DisplayName;
            Category = brush.Category;
            _markDirty = markDirty;
            Load(brush);
        }

        public string ResourceKey { get; }

        public string DisplayName { get; }

        public string Category { get; }

        public string BaseHexValue { get; private set; } = "#FF000000";

        public string HexValue
        {
            get => _hexValue;
            set
            {
                if (SetProperty(ref _hexValue, value) == false || _isUpdating)
                {
                    return;
                }

                if (TryNormalizeHex(value, out string normalizedHex, out byte alpha, out byte red, out byte green, out byte blue))
                {
                    _isUpdating = true;
                    _hexValue = normalizedHex;
                    RaisePropertyChanged(nameof(HexValue));
                    SetProperty(ref _alpha, alpha, nameof(Alpha));
                    SetProperty(ref _red, red, nameof(Red));
                    SetProperty(ref _green, green, nameof(Green));
                    SetProperty(ref _blue, blue, nameof(Blue));
                    _isUpdating = false;
                    ErrorText = string.Empty;
                    _markDirty();
                    return;
                }

                ErrorText = "Invalid color. Use #RRGGBB or #AARRGGBB.";
            }
        }

        public byte Alpha
        {
            get => _alpha;
            set => SetChannel(ref _alpha, value, nameof(Alpha));
        }

        public byte Red
        {
            get => _red;
            set => SetChannel(ref _red, value, nameof(Red));
        }

        public byte Green
        {
            get => _green;
            set => SetChannel(ref _green, value, nameof(Green));
        }

        public byte Blue
        {
            get => _blue;
            set => SetChannel(ref _blue, value, nameof(Blue));
        }

        public string ErrorText
        {
            get => _errorText;
            private set
            {
                if (SetProperty(ref _errorText, value))
                {
                    RaisePropertyChanged(nameof(IsValid));
                }
            }
        }

        public bool IsValid => string.IsNullOrEmpty(ErrorText);

        public Studio.Wpf.Themes.ThemeBrushColor ToBrushColor()
            => new(ResourceKey, DisplayName, Category, _hexValue);

        public void Load(Studio.Wpf.Themes.ThemeBrushColor brush)
        {
            BaseHexValue = brush.HexValue;
            LoadCurrentValue(brush.HexValue);
        }

        public void ApplyBaseColor(string baseHexValue)
        {
            BaseHexValue = NormalizeHex(baseHexValue);
            LoadCurrentValue(BaseHexValue);
        }

        public void ResetToBase()
        {
            LoadCurrentValue(BaseHexValue);
            _markDirty();
        }

        private void LoadCurrentValue(string hexValue)
        {
            if (TryNormalizeHex(hexValue, out string normalizedHex, out byte alpha, out byte red, out byte green, out byte blue) == false)
            {
                throw new ArgumentException($"Invalid theme color value: {hexValue}", nameof(hexValue));
            }

            _isUpdating = true;
            _hexValue = normalizedHex;
            RaisePropertyChanged(nameof(HexValue));
            SetProperty(ref _alpha, alpha, nameof(Alpha));
            SetProperty(ref _red, red, nameof(Red));
            SetProperty(ref _green, green, nameof(Green));
            SetProperty(ref _blue, blue, nameof(Blue));
            _isUpdating = false;
            ErrorText = string.Empty;
        }

        private void SetChannel(ref byte field, byte value, string propertyName)
        {
            if (SetProperty(ref field, value, propertyName) == false || _isUpdating)
            {
                return;
            }

            _isUpdating = true;
            _hexValue = $"#{_alpha:X2}{_red:X2}{_green:X2}{_blue:X2}";
            RaisePropertyChanged(nameof(HexValue));
            _isUpdating = false;
            ErrorText = string.Empty;
            _markDirty();
        }

        private static string NormalizeHex(string hexValue)
        {
            if (TryNormalizeHex(hexValue, out string normalizedHex, out _, out _, out _, out _) == false)
            {
                throw new ArgumentException($"Invalid color format: {hexValue}", nameof(hexValue));
            }

            return normalizedHex;
        }

        private static bool TryNormalizeHex(
            string hexValue,
            out string normalizedHex,
            out byte alpha,
            out byte red,
            out byte green,
            out byte blue)
        {
            normalizedHex = "#FF000000";
            alpha = 0xFF;
            red = 0;
            green = 0;
            blue = 0;

            if (string.IsNullOrWhiteSpace(hexValue))
            {
                return false;
            }

            string normalized = hexValue.Trim();
            if (normalized.StartsWith('#'))
            {
                normalized = normalized[1..];
            }

            if (normalized.Length == 6)
            {
                alpha = 0xFF;
                red = Convert.ToByte(normalized[0..2], 16);
                green = Convert.ToByte(normalized[2..4], 16);
                blue = Convert.ToByte(normalized[4..6], 16);
                normalizedHex = $"#{alpha:X2}{red:X2}{green:X2}{blue:X2}";
                return true;
            }

            if (normalized.Length == 8)
            {
                alpha = Convert.ToByte(normalized[0..2], 16);
                red = Convert.ToByte(normalized[2..4], 16);
                green = Convert.ToByte(normalized[4..6], 16);
                blue = Convert.ToByte(normalized[6..8], 16);
                normalizedHex = $"#{alpha:X2}{red:X2}{green:X2}{blue:X2}";
                return true;
            }

            return false;
        }
    }

    private readonly Studio.Wpf.Themes.IThemeService _themeService;
    private Studio.Wpf.Themes.ThemeOption? _selectedBuiltInTheme;
    private Studio.Wpf.Themes.CustomThemeSlotInfo? _selectedCustomThemeSlot;
    private Studio.Wpf.Themes.ThemeOption? _selectedBaseTheme;
    private ThemeBrushEditorViewModel? _selectedBrush;
    private string _customThemeName = string.Empty;
    private bool _isDirty;
    private bool _isLoadingDraft;

    private NoxUI.ViewModelCommand? _applyBuiltInThemeCommand;
    private NoxUI.ViewModelCommand? _copyFromBaseCommand;
    private NoxUI.ViewModelCommand? _saveCustomThemeCommand;
    private NoxUI.ViewModelCommand? _applyCustomThemeCommand;
    private NoxUI.ViewModelCommand? _deleteCustomThemeCommand;
    private NoxUI.ViewModelCommand? _resetSelectedBrushCommand;

    public ThemeSettingsViewModel()
        : this(Prism.Ioc.ContainerLocator.Container.Resolve<Studio.Wpf.Themes.IThemeService>())
    {
    }

    public ThemeSettingsViewModel(Studio.Wpf.Themes.IThemeService themeService)
    {
        _themeService = themeService;

        BuiltInThemes = new ObservableCollection<Studio.Wpf.Themes.ThemeOption>();
        CustomThemeSlots = new ObservableCollection<Studio.Wpf.Themes.CustomThemeSlotInfo>();
        Brushes = new ObservableCollection<ThemeBrushEditorViewModel>();

        RefreshBuiltInThemes();
        RefreshCustomThemeSlots();

        _selectedBuiltInTheme = BuiltInThemes.FirstOrDefault(theme => string.Equals(theme.Key, ResolveSelectedBuiltInKey(), StringComparison.Ordinal))
            ?? BuiltInThemes.FirstOrDefault();
        RaisePropertyChanged(nameof(SelectedBuiltInTheme));

        _selectedCustomThemeSlot = CustomThemeSlots.FirstOrDefault();
        RaisePropertyChanged(nameof(SelectedCustomThemeSlot));
        LoadSelectedCustomThemeSlot();

        _themeService.PropertyChanged += OnThemeServicePropertyChanged;
    }

    public ObservableCollection<Studio.Wpf.Themes.ThemeOption> BuiltInThemes { get; }

    public ObservableCollection<Studio.Wpf.Themes.CustomThemeSlotInfo> CustomThemeSlots { get; }

    public ObservableCollection<ThemeBrushEditorViewModel> Brushes { get; }

    public Studio.Wpf.Themes.ThemeOption? SelectedBuiltInTheme
    {
        get => _selectedBuiltInTheme;
        set
        {
            if (SetProperty(ref _selectedBuiltInTheme, value))
            {
                RefreshCommandStates();
            }
        }
    }

    public Studio.Wpf.Themes.CustomThemeSlotInfo? SelectedCustomThemeSlot
    {
        get => _selectedCustomThemeSlot;
        set
        {
            if (ReferenceEquals(_selectedCustomThemeSlot, value))
            {
                return;
            }

            if (ConfirmSwitchCustomThemeSlot() == false)
            {
                RaisePropertyChanged(nameof(SelectedCustomThemeSlot));
                return;
            }

            if (SetProperty(ref _selectedCustomThemeSlot, value))
            {
                LoadSelectedCustomThemeSlot();
                RefreshCommandStates();
            }
        }
    }

    public Studio.Wpf.Themes.ThemeOption? SelectedBaseTheme
    {
        get => _selectedBaseTheme;
        set
        {
            if (SetProperty(ref _selectedBaseTheme, value))
            {
                RefreshCommandStates();
            }
        }
    }

    public ThemeBrushEditorViewModel? SelectedBrush
    {
        get => _selectedBrush;
        set
        {
            if (SetProperty(ref _selectedBrush, value))
            {
                RaisePropertyChanged(nameof(HasSelectedBrush));
                RefreshCommandStates();
            }
        }
    }

    public string CustomThemeName
    {
        get => _customThemeName;
        set
        {
            if (SetProperty(ref _customThemeName, value))
            {
                MarkDirty();
            }
        }
    }

    public bool IsDirty
    {
        get => _isDirty;
        private set
        {
            if (SetProperty(ref _isDirty, value))
            {
                RaisePropertyChanged(nameof(StatusText));
                RefreshCommandStates();
            }
        }
    }

    public bool HasSelectedBrush => SelectedBrush != null;

    public string StatusText => IsDirty ? "Unsaved theme changes" : $"Current Theme: {ResolveCurrentThemeDisplayName()}";

    public NoxUI.ViewModelCommand ApplyBuiltInThemeCommand => _applyBuiltInThemeCommand ??= new(ApplyBuiltInTheme, CanApplyBuiltInTheme);

    public NoxUI.ViewModelCommand CopyFromBaseCommand => _copyFromBaseCommand ??= new(CopyFromBase, CanEditCustomTheme);

    public NoxUI.ViewModelCommand SaveCustomThemeCommand => _saveCustomThemeCommand ??= new(SaveCustomTheme, CanSaveCustomTheme);

    public NoxUI.ViewModelCommand ApplyCustomThemeCommand => _applyCustomThemeCommand ??= new(ApplyCustomTheme, CanSaveCustomTheme);

    public NoxUI.ViewModelCommand DeleteCustomThemeCommand => _deleteCustomThemeCommand ??= new(DeleteCustomTheme, CanDeleteCustomTheme);

    public NoxUI.ViewModelCommand ResetSelectedBrushCommand => _resetSelectedBrushCommand ??= new(ResetSelectedBrush, () => SelectedBrush != null);

    public bool ConfirmClose()
    {
        if (IsDirty == false)
        {
            return true;
        }

        System.Windows.MessageBoxResult result = Core.UI.MessageBox.ShowDialog(
            "Theme settings have unsaved changes. Save before closing?",
            "Theme Settings",
            System.Windows.MessageBoxImage.Question,
            System.Windows.MessageBoxButton.YesNoCancel);

        if (result == System.Windows.MessageBoxResult.Cancel)
        {
            return false;
        }

        if (result == System.Windows.MessageBoxResult.Yes)
        {
            SaveCustomTheme();
        }

        return true;
    }

    private void RefreshBuiltInThemes()
    {
        BuiltInThemes.Clear();
        foreach (Studio.Wpf.Themes.ThemeOption theme in _themeService.BuiltInThemes)
        {
            BuiltInThemes.Add(theme);
        }
    }

    private void RefreshCustomThemeSlots()
    {
        string? selectedKey = _selectedCustomThemeSlot?.Key;
        CustomThemeSlots.Clear();
        foreach (Studio.Wpf.Themes.CustomThemeSlotInfo slot in _themeService.CustomThemeSlots)
        {
            CustomThemeSlots.Add(slot);
        }

        _selectedCustomThemeSlot = CustomThemeSlots.FirstOrDefault(slot => string.Equals(slot.Key, selectedKey, StringComparison.Ordinal))
            ?? CustomThemeSlots.FirstOrDefault();
        RaisePropertyChanged(nameof(SelectedCustomThemeSlot));
    }

    private void LoadSelectedCustomThemeSlot()
    {
        if (SelectedCustomThemeSlot == null)
        {
            Brushes.Clear();
            SelectedBrush = null;
            CustomThemeName = string.Empty;
            SelectedBaseTheme = null;
            IsDirty = false;
            return;
        }

        _isLoadingDraft = true;
        try
        {
            Studio.Wpf.Themes.CustomThemeDefinition? customTheme = _themeService.GetCustomTheme(SelectedCustomThemeSlot.Key);
            if (customTheme != null)
            {
                CustomThemeName = customTheme.DisplayName;
                SelectedBaseTheme = BuiltInThemes.FirstOrDefault(theme => string.Equals(theme.Key, customTheme.BaseThemeKey, StringComparison.Ordinal))
                    ?? BuiltInThemes.FirstOrDefault();
                LoadBrushes(customTheme.Brushes);
            }
            else
            {
                string baseThemeKey = SelectedBuiltInTheme?.Key ?? BuiltInThemes.First().Key;
                CustomThemeName = $"Custom Theme {SelectedCustomThemeSlot.SlotIndex}";
                SelectedBaseTheme = BuiltInThemes.First(theme => string.Equals(theme.Key, baseThemeKey, StringComparison.Ordinal));
                LoadBrushes(_themeService.GetThemeBrushes(baseThemeKey));
            }
        }
        finally
        {
            _isLoadingDraft = false;
            IsDirty = false;
        }
    }

    private void LoadBrushes(System.Collections.Generic.IReadOnlyList<Studio.Wpf.Themes.ThemeBrushColor> brushes)
    {
        string? selectedResourceKey = SelectedBrush?.ResourceKey;
        Brushes.Clear();
        foreach (Studio.Wpf.Themes.ThemeBrushColor brush in brushes)
        {
            Brushes.Add(new ThemeBrushEditorViewModel(brush, MarkDirty));
        }

        SelectedBrush = Brushes.FirstOrDefault(brush => string.Equals(brush.ResourceKey, selectedResourceKey, StringComparison.Ordinal))
            ?? Brushes.FirstOrDefault();
    }

    private void ApplyBuiltInTheme()
    {
        if (SelectedBuiltInTheme == null)
        {
            return;
        }

        _themeService.ApplyTheme(SelectedBuiltInTheme.Key);
        RaisePropertyChanged(nameof(StatusText));
    }

    private bool CanApplyBuiltInTheme()
        => SelectedBuiltInTheme != null;

    private void CopyFromBase()
    {
        if (SelectedBaseTheme == null)
        {
            return;
        }

        _isLoadingDraft = true;
        try
        {
            System.Collections.Generic.IReadOnlyList<Studio.Wpf.Themes.ThemeBrushColor> brushes = _themeService.GetThemeBrushes(SelectedBaseTheme.Key);
            LoadBrushes(brushes);
            foreach (ThemeBrushEditorViewModel brush in Brushes)
            {
                brush.ApplyBaseColor(brush.HexValue);
            }
        }
        finally
        {
            _isLoadingDraft = false;
        }

        MarkDirty();
    }

    private bool CanEditCustomTheme()
        => SelectedCustomThemeSlot != null && SelectedBaseTheme != null;

    private void SaveCustomTheme()
    {
        SaveCustomTheme(applyTheme: false);
    }

    private void ApplyCustomTheme()
    {
        SaveCustomTheme(applyTheme: true);
    }

    private void SaveCustomTheme(bool applyTheme)
    {
        if (SelectedCustomThemeSlot == null || SelectedBaseTheme == null)
        {
            return;
        }

        Studio.Wpf.Themes.CustomThemeDefinition definition = new(
            SelectedCustomThemeSlot.Key,
            string.IsNullOrWhiteSpace(CustomThemeName) ? $"Custom Theme {SelectedCustomThemeSlot.SlotIndex}" : CustomThemeName.Trim(),
            SelectedBaseTheme.Key,
            Brushes.Select(static brush => brush.ToBrushColor()).ToArray());

        _themeService.SaveCustomTheme(definition, applyTheme);
        RefreshCustomThemeSlots();
        _selectedCustomThemeSlot = CustomThemeSlots.FirstOrDefault(slot => string.Equals(slot.Key, definition.Key, StringComparison.Ordinal));
        RaisePropertyChanged(nameof(SelectedCustomThemeSlot));
        IsDirty = false;
        RaisePropertyChanged(nameof(StatusText));
    }

    private bool CanSaveCustomTheme()
        => SelectedCustomThemeSlot != null &&
           SelectedBaseTheme != null &&
           Brushes.Count > 0 &&
           Brushes.All(static brush => brush.IsValid);

    private void DeleteCustomTheme()
    {
        if (SelectedCustomThemeSlot == null || SelectedCustomThemeSlot.HasTheme == false)
        {
            return;
        }

        System.Windows.MessageBoxResult result = Core.UI.MessageBox.ShowDialog(
            $"Delete custom theme '{SelectedCustomThemeSlot.DisplayName}'?",
            "Theme Settings",
            System.Windows.MessageBoxImage.Question,
            System.Windows.MessageBoxButton.YesNo);
        if (result != System.Windows.MessageBoxResult.Yes)
        {
            return;
        }

        string deletedKey = SelectedCustomThemeSlot.Key;
        _themeService.DeleteCustomTheme(deletedKey);
        RefreshCustomThemeSlots();
        _selectedCustomThemeSlot = CustomThemeSlots.FirstOrDefault(slot => string.Equals(slot.Key, deletedKey, StringComparison.Ordinal))
            ?? CustomThemeSlots.FirstOrDefault();
        RaisePropertyChanged(nameof(SelectedCustomThemeSlot));
        LoadSelectedCustomThemeSlot();
    }

    private bool CanDeleteCustomTheme()
        => SelectedCustomThemeSlot?.HasTheme == true;

    private void ResetSelectedBrush()
    {
        SelectedBrush?.ResetToBase();
    }

    private void MarkDirty()
    {
        if (_isLoadingDraft)
        {
            return;
        }

        IsDirty = true;
    }

    private bool ConfirmSwitchCustomThemeSlot()
    {
        if (IsDirty == false)
        {
            return true;
        }

        System.Windows.MessageBoxResult result = Core.UI.MessageBox.ShowDialog(
            "Save theme changes before switching custom theme slots?",
            "Theme Settings",
            System.Windows.MessageBoxImage.Question,
            System.Windows.MessageBoxButton.YesNoCancel);

        if (result == System.Windows.MessageBoxResult.Cancel)
        {
            return false;
        }

        if (result == System.Windows.MessageBoxResult.Yes)
        {
            SaveCustomTheme();
        }

        return true;
    }

    private string ResolveSelectedBuiltInKey()
    {
        if (_themeService.CurrentThemeKey.StartsWith("Custom", StringComparison.Ordinal))
        {
            return _themeService.GetCustomTheme(_themeService.CurrentThemeKey)?.BaseThemeKey ?? BuiltInThemes.First().Key;
        }

        return _themeService.CurrentThemeKey;
    }

    private string ResolveCurrentThemeDisplayName()
        => _themeService.AvailableThemes.FirstOrDefault(theme => string.Equals(theme.Key, _themeService.CurrentThemeKey, StringComparison.Ordinal))?.DisplayName
            ?? _themeService.CurrentThemeKey;

    private void RefreshCommandStates()
    {
        _applyBuiltInThemeCommand?.RaiseCanExecuteChanged();
        _copyFromBaseCommand?.RaiseCanExecuteChanged();
        _saveCustomThemeCommand?.RaiseCanExecuteChanged();
        _applyCustomThemeCommand?.RaiseCanExecuteChanged();
        _deleteCustomThemeCommand?.RaiseCanExecuteChanged();
        _resetSelectedBrushCommand?.RaiseCanExecuteChanged();
    }

    private void OnThemeServicePropertyChanged(object? sender, System.ComponentModel.PropertyChangedEventArgs e)
    {
        if (e.PropertyName == nameof(Studio.Wpf.Themes.IThemeService.BuiltInThemes) ||
            e.PropertyName == nameof(Studio.Wpf.Themes.IThemeService.AvailableThemes) ||
            e.PropertyName == nameof(Studio.Wpf.Themes.IThemeService.CustomThemeSlots))
        {
            RefreshBuiltInThemes();
            RefreshCustomThemeSlots();
        }

        if (e.PropertyName == nameof(Studio.Wpf.Themes.IThemeService.CurrentThemeKey) ||
            e.PropertyName == nameof(Studio.Wpf.Themes.IThemeService.AvailableThemes))
        {
            RaisePropertyChanged(nameof(StatusText));
        }
    }

    public new void Dispose()
    {
        _themeService.PropertyChanged -= OnThemeServicePropertyChanged;
    }
}
