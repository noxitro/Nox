// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Windows;
using AvalonDock.Layout;
using Studio.Wpf.ViewModels;

namespace Studio.Wpf;

public partial class MainWindow : Window
{
    private LayoutDocument? _ProjectSettingsDocument;
    private LayoutDocument? _ThemeSettingsDocument;
    private LayoutDocument? _EngineSystemGraphDocument;
    private LayoutDocument? _RemoteInstanceManagerDocument;
    private LayoutDocument? _MemoryProfilerDocument;
    private bool _ShutdownPrepared;

    public MainWindow()
    {
        InitializeComponent();
        DataContext = new MainWindowViewModel();
        Core.UI.ProjectSettingsViewService.Register(ShowProjectSettings);
        ThemeSettingsViewService.Register(ShowThemeSettings);
        Core.UI.CoreDiagnosticsViewService.Register(ShowCoreDiagnosticsView);
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

    private void ShowThemeSettings()
    {
        if (_ThemeSettingsDocument == null || _ThemeSettingsDocument.Root == null)
        {
            Views.ThemeSettingsView view = new();
            _ThemeSettingsDocument = new LayoutDocument
            {
                Title = "Theme Settings",
                ContentId = "ThemeSettings",
                CanClose = true,
                CanFloat = false,
                Content = view,
            };
            _ThemeSettingsDocument.Closing += OnThemeSettingsClosing;
            _ThemeSettingsDocument.Closed += OnThemeSettingsClosed;
            MainDocumentPane.Children.Add(_ThemeSettingsDocument);
        }

        _ThemeSettingsDocument.IsSelected = true;
    }

    private void ShowCoreDiagnosticsView(Core.UI.CoreDiagnosticsViewKind kind)
    {
        LayoutDocument document = kind switch
        {
            Core.UI.CoreDiagnosticsViewKind.EngineSystemGraph => GetOrCreateEngineSystemGraphDocument(),
            Core.UI.CoreDiagnosticsViewKind.RemoteInstances => GetOrCreateRemoteInstanceManagerDocument(),
            Core.UI.CoreDiagnosticsViewKind.MemoryProfiler => GetOrCreateMemoryProfilerDocument(),
            _ => throw new ArgumentOutOfRangeException(nameof(kind), kind, null),
        };
        document.IsSelected = true;
    }

    private LayoutDocument GetOrCreateEngineSystemGraphDocument()
    {
        if (_EngineSystemGraphDocument == null || _EngineSystemGraphDocument.Root == null)
        {
            _EngineSystemGraphDocument = CreateDiagnosticsDocument("EngineSystem Graph", "EngineSystemGraph", new Core.UI.Views.EngineSystemGraphView());
            _EngineSystemGraphDocument.Closed += (_, _) => _EngineSystemGraphDocument = null;
        }
        return _EngineSystemGraphDocument;
    }

    private LayoutDocument GetOrCreateRemoteInstanceManagerDocument()
    {
        if (_RemoteInstanceManagerDocument == null || _RemoteInstanceManagerDocument.Root == null)
        {
            _RemoteInstanceManagerDocument = CreateDiagnosticsDocument("Remote Instances", "RemoteInstances", new Core.UI.Views.RemoteInstanceManagerView());
            _RemoteInstanceManagerDocument.Closed += (_, _) => _RemoteInstanceManagerDocument = null;
        }
        return _RemoteInstanceManagerDocument;
    }

    private LayoutDocument GetOrCreateMemoryProfilerDocument()
    {
        if (_MemoryProfilerDocument == null || _MemoryProfilerDocument.Root == null)
        {
            _MemoryProfilerDocument = CreateDiagnosticsDocument("Memory Profiler", "MemoryProfiler", new Core.UI.Views.MemoryProfilerView());
            _MemoryProfilerDocument.Closed += (_, _) => _MemoryProfilerDocument = null;
        }
        return _MemoryProfilerDocument;
    }

    private LayoutDocument CreateDiagnosticsDocument(string title, string contentId, object content)
    {
        LayoutDocument document = new()
        {
            Title = title,
            ContentId = contentId,
            CanClose = true,
            CanFloat = false,
            Content = content,
        };
        MainDocumentPane.Children.Add(document);
        return document;
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

    private void OnThemeSettingsClosing(object? sender, CancelEventArgs e)
    {
        if (sender is not LayoutDocument { Content: Views.ThemeSettingsView view })
        {
            return;
        }

        if (view.DataContext is Studio.Wpf.ViewModels.ThemeSettingsViewModel viewModel)
        {
            e.Cancel = viewModel.ConfirmClose() == false;
        }
    }

    private void OnThemeSettingsClosed(object? sender, System.EventArgs e)
    {
        if (_ThemeSettingsDocument != null)
        {
            if (_ThemeSettingsDocument.Content is Views.ThemeSettingsView { DataContext: IDisposable disposable })
            {
                disposable.Dispose();
            }
            _ThemeSettingsDocument.Closing -= OnThemeSettingsClosing;
            _ThemeSettingsDocument.Closed -= OnThemeSettingsClosed;
            _ThemeSettingsDocument = null;
        }
    }

    private void OnWindowClosing(object? sender, CancelEventArgs e)
    {
        e.Cancel = ConfirmProjectSettingsClose() == false || ConfirmThemeSettingsClose() == false;
    }

    private void OnClosed(object? sender, System.EventArgs e)
    {
        if (_ShutdownPrepared == false)
        {
            PrepareForShutdown();
        }
        Core.UI.ProjectSettingsViewService.Unregister(ShowProjectSettings);
        ThemeSettingsViewService.Unregister(ShowThemeSettings);
        Core.UI.CoreDiagnosticsViewService.Unregister(ShowCoreDiagnosticsView);
    }

    private void PrepareForShutdown()
    {
        if (_ShutdownPrepared)
        {
            return;
        }

        _ShutdownPrepared = true;
        Core.StudioManager.Instance.Workspace.RuntimeSessions.Dispose();
        DisposeOwnedDataContexts();
    }

    private void DisposeOwnedDataContexts()
    {
        HashSet<object> disposed = new(ReferenceEqualityComparer.Instance);
        TryDisposeDataContext(this, disposed);
        foreach (LayoutContent content in DockManager.Layout.Descendents().OfType<LayoutContent>())
        {
            TryDisposeDataContext(content.Content, disposed);
        }
    }

    private static void TryDisposeDataContext(object? owner, HashSet<object> disposed)
    {
        if (owner is not FrameworkElement { DataContext: IDisposable disposable } element)
        {
            return;
        }

        if (element.DataContext != null && disposed.Add(element.DataContext))
        {
            disposable.Dispose();
        }
    }

    private bool ConfirmProjectSettingsClose()
    {
        if (_ProjectSettingsDocument?.Content is not Core.UI.Views.ProjectSettingsView view)
        {
            return true;
        }

        return view.DataContext is not Core.UI.ViewModels.ProjectSettingsViewModel viewModel || viewModel.ConfirmClose();
    }

    private bool ConfirmThemeSettingsClose()
    {
        if (_ThemeSettingsDocument?.Content is not Views.ThemeSettingsView view)
        {
            return true;
        }

        return view.DataContext is not Studio.Wpf.ViewModels.ThemeSettingsViewModel viewModel || viewModel.ConfirmClose();
    }
}
