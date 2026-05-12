using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Text;
using System.ComponentModel;

namespace Studio.Wpf.ViewModels
{
	public class MainWindowViewModel : NoxUI.ViewModelBase, IDisposable
	{
		public sealed class ThemeMenuItem : NoxUI.ViewModelBase
		{
			private bool _IsChecked;

			public required string Header { get; init; }
			public required string AutomationId { get; init; }
			public required NoxUI.ViewModelCommand Command { get; init; }

			public bool IsChecked
			{
				get => _IsChecked;
				set => SetProperty(ref _IsChecked, value);
			}
		}

		public MainWindowViewModel()
			: this(Prism.Ioc.ContainerLocator.Container.Resolve<Studio.Wpf.Themes.IThemeService>())
		{
		}

		public MainWindowViewModel(Studio.Wpf.Themes.IThemeService themeService)
		{
			Theme = themeService;
			_ThemeMenuItems = [];
			RefreshThemeMenuItems();
			themeService.PropertyChanged += OnThemeServicePropertyChanged;
		}

		#region 非公開フィールド
		//	dockingmanager
		AvalonDock.DockingManager? _DockingManager = null;

		private Core.UI.ViewModels.ToolViewModel[] _ToolViewModelList = [];
		private Core.UI.ViewModels.AssetViewModel[] _AssetViewModelList = [];
		private readonly ObservableCollection<ThemeMenuItem> _ThemeMenuItems;
		private bool _Disposed;
		#endregion

		#region 公開プロパティ
		public IReadOnlyList<Core.UI.ViewModels.ToolViewModel> ToolViewModelList => _ToolViewModelList;
		public IReadOnlyList<Core.UI.ViewModels.AssetViewModel> AssetViewModelList => _AssetViewModelList;
		public ObservableCollection<ThemeMenuItem> ThemeMenuItems => _ThemeMenuItems;

		/// <summary>テーマサービス。<c>Theme.CurrentTheme</c> を AvalonDock にバインドする。</summary>
		public Studio.Wpf.Themes.IThemeService Theme { get; }
		public NoxUI.ViewModelCommand GenerateRemoteCodeCommand => field ??= new(GenerateRemoteCode);
		public NoxUI.ViewModelCommand ShowInspectorCommand => field ??= new(ShowInspector);
		public NoxUI.ViewModelCommand ShowHierarchyCommand => field ??= new(ShowHierarchy);
		public NoxUI.ViewModelCommand ShowProjectSettingsCommand => field ??= new(ShowProjectSettings);
		public NoxUI.ViewModelCommand ShowThemeSettingsCommand => field ??= new(ShowThemeSettings);
		public NoxUI.ViewModelCommand ShowEngineSystemGraphCommand => field ??= new(ShowEngineSystemGraph);
		public NoxUI.ViewModelCommand ShowRemoteInstanceManagerCommand => field ??= new(ShowRemoteInstanceManager);
		public NoxUI.ViewModelCommand ShowMemoryProfilerCommand => field ??= new(ShowMemoryProfiler);
		public NoxUI.ViewModelCommand NewSceneCommand => field ??= new(NewScene);
		public NoxUI.ViewModelCommand SaveSceneCommand => field ??= new(SaveScene);
		public NoxUI.ViewModelCommand SaveSceneAsCommand => field ??= new(SaveSceneAs);
		public NoxUI.ViewModelCommand ExitCommand => field ??= new(Exit);
		#endregion

		#region 公開メソッド
		public void Loaded(AvalonDock.DockingManager dockingManager)
		{
			_DockingManager = dockingManager;

			//_ToolViewModelList = ;
		}

		public void Unloaded(AvalonDock.DockingManager dockingManager)
		{
			if (_DockingManager == dockingManager)
			{
				_DockingManager = null;
			}
		}
		#endregion

		#region 非公開メソッド
		private static void ApplyTheme(Studio.Wpf.Themes.IThemeService themeService, string key)
		{
			try
			{
				themeService.ApplyTheme(key);
			}
			catch (ArgumentException ex)
			{
				Nox.LogTrace.ErrorLine<Core.LogId.Runtime>($"Theme selection failed: {ex.Message}");
				Core.UI.MessageBox.ShowDialog(
					$"テーマの切り替えに失敗しました。\n{ex.Message}",
					"Theme",
					System.Windows.MessageBoxImage.Warning,
					System.Windows.MessageBoxButton.OK);
			}
			catch (InvalidOperationException ex)
			{
				Nox.LogTrace.ErrorLine<Core.LogId.Runtime>($"Theme load failed: {ex.Message}");
				Core.UI.MessageBox.ShowDialog(
					$"テーマリソースの読み込みに失敗しました。\n{ex.Message}",
					"Theme",
					System.Windows.MessageBoxImage.Warning,
					System.Windows.MessageBoxButton.OK);
			}
		}

		private void OnThemeServicePropertyChanged(object? sender, PropertyChangedEventArgs e)
		{
			if (e.PropertyName == nameof(Studio.Wpf.Themes.IThemeService.AvailableThemes) ||
				e.PropertyName == nameof(Studio.Wpf.Themes.IThemeService.CurrentThemeKey))
			{
				RefreshThemeMenuItems();
			}
		}

		private void RefreshThemeMenuItems()
		{
			string currentKey = Theme.CurrentThemeKey;
			IReadOnlyList<Studio.Wpf.Themes.ThemeOption> options = Theme.AvailableThemes;
			_ThemeMenuItems.Clear();
			foreach (Studio.Wpf.Themes.ThemeOption option in options)
			{
				string key = option.Key;
				_ThemeMenuItems.Add(new ThemeMenuItem
				{
					Header = option.DisplayName,
					AutomationId = $"NoxStudio.Theme.{option.Key}",
					Command = new NoxUI.ViewModelCommand(() => ApplyTheme(Theme, key)),
					IsChecked = string.Equals(option.Key, currentKey, StringComparison.Ordinal),
				});
			}
		}

		private void GenerateRemoteCode()
		{
			var result = Core.UI.MessageBox.ShowDialog(
				"RemoteCodeを出力しますか？",
				"確認",
				System.Windows.MessageBoxImage.Information,
				System.Windows.MessageBoxButton.YesNo
				);

			if (result == System.Windows.MessageBoxResult.Yes)
			{
				Core.RuntimeRemote.RuntimeRemoteCodeGenerator generator = new();
				generator.GenerateCode();

				Core.UI.MessageBox.ShowDialog(
					"RemoteCodeの出力が完了しました。",
					"情報",
					System.Windows.MessageBoxImage.Information,
					System.Windows.MessageBoxButton.OK
					);
			}
		}

		private void ShowInspector()
		{
			// TODO: Inspector の表示処理を実装
		}

		private void ShowHierarchy()
		{
			Core.StudioManager.Instance.Workspace.SceneHierarchy.InitializeDefaultScene();
		}

		private void ShowProjectSettings()
		{
			Core.UI.ProjectSettingsViewService.Show();
		}

		private void ShowThemeSettings()
		{
			Studio.Wpf.ThemeSettingsViewService.Show();
		}

		private static void ShowEngineSystemGraph()
		{
			Core.UI.CoreDiagnosticsViewService.Show(Core.UI.CoreDiagnosticsViewKind.EngineSystemGraph);
		}

		private static void ShowRemoteInstanceManager()
		{
			Core.UI.CoreDiagnosticsViewService.Show(Core.UI.CoreDiagnosticsViewKind.RemoteInstances);
		}

		private static void ShowMemoryProfiler()
		{
			Core.UI.CoreDiagnosticsViewService.Show(Core.UI.CoreDiagnosticsViewKind.MemoryProfiler);
		}

		private static void NewScene()
		{
			Core.StudioManager.Instance.Workspace.SceneHierarchy.NewScene();
			Nox.LogTrace.InfoLine<Core.LogId.Runtime>("New scene created.");
		}

		private static void SaveScene()
		{
			Core.Workspace workspace = Core.StudioManager.Instance.Workspace;
			string filePath = workspace.SceneHierarchy.SaveScene(workspace.AssetRootPath);
			workspace.AssetManager.Refresh();
			Nox.LogTrace.InfoLine<Core.LogId.Runtime>($"Scene saved: {filePath}");
		}

		private static void SaveSceneAs()
		{
			Core.Workspace workspace = Core.StudioManager.Instance.Workspace;
			Microsoft.Win32.SaveFileDialog dialog = new()
			{
				AddExtension = true,
				DefaultExt = ".noxscene",
				FileName = "Main Scene.noxscene",
				Filter = "Nox Scene (*.noxscene)|*.noxscene|All files (*.*)|*.*",
				InitialDirectory = workspace.AssetRootPath,
				OverwritePrompt = true,
				Title = "Save Scene As",
			};

			if (dialog.ShowDialog() != true)
			{
				return;
			}

			workspace.SceneHierarchy.SaveSceneAs(dialog.FileName);
			workspace.AssetManager.Refresh();
			Nox.LogTrace.InfoLine<Core.LogId.Runtime>($"Scene saved as: {dialog.FileName}");
		}

		private static void Exit()
		{
			System.Windows.Application.Current.Shutdown();
		}

		public override void Dispose()
		{
			if (_Disposed)
			{
				return;
			}

			Theme.PropertyChanged -= OnThemeServicePropertyChanged;
			_Disposed = true;
		}
		#endregion
	}
}
