using System.ComponentModel;
using System.Windows;
using AvalonDock.Layout;
using Studio.Wpf.ViewModels;

namespace Studio.Wpf;

public partial class MainWindow : Window
{
    private LayoutDocument? _ProjectSettingsDocument;

    public MainWindow()
    {
        InitializeComponent();
        DataContext = new MainWindowViewModel();
        Core.UI.ProjectSettingsViewService.Register(ShowProjectSettings);
        Closing += OnWindowClosing;
        Closed += OnClosed;
    }

    private void ShowProjectSettings()
    {
        if (_ProjectSettingsDocument == null || _ProjectSettingsDocument.Root == null)
        {
            Core.UI.Views.ProjectSettingsView view = new();
            _ProjectSettingsDocument = new LayoutDocument
            {
                Title = "Project Settings",
                ContentId = "ProjectSettings",
                CanClose = true,
                CanFloat = false,
                Content = view,
            };
            _ProjectSettingsDocument.Closing += OnProjectSettingsClosing;
            _ProjectSettingsDocument.Closed += OnProjectSettingsClosed;
            MainDocumentPane.Children.Add(_ProjectSettingsDocument);
        }

        _ProjectSettingsDocument.IsSelected = true;
    }

    private void OnProjectSettingsClosing(object? sender, CancelEventArgs e)
    {
        if (sender is not LayoutDocument { Content: Core.UI.Views.ProjectSettingsView view })
        {
            return;
        }

        if (view.DataContext is Core.UI.ViewModels.ProjectSettingsViewModel viewModel)
        {
            e.Cancel = viewModel.ConfirmClose() == false;
        }
    }

    private void OnProjectSettingsClosed(object? sender, System.EventArgs e)
    {
        if (_ProjectSettingsDocument != null)
        {
            _ProjectSettingsDocument.Closing -= OnProjectSettingsClosing;
            _ProjectSettingsDocument.Closed -= OnProjectSettingsClosed;
            _ProjectSettingsDocument = null;
        }
    }

    private void OnWindowClosing(object? sender, CancelEventArgs e)
    {
        e.Cancel = ConfirmProjectSettingsClose() == false;
    }

    private void OnClosed(object? sender, System.EventArgs e)
    {
        Core.UI.ProjectSettingsViewService.Unregister(ShowProjectSettings);
    }

    private bool ConfirmProjectSettingsClose()
    {
        if (_ProjectSettingsDocument?.Content is not Core.UI.Views.ProjectSettingsView view)
        {
            return true;
        }

        return view.DataContext is not Core.UI.ViewModels.ProjectSettingsViewModel viewModel || viewModel.ConfirmClose();
    }
}
