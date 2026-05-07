using System;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.Linq;

namespace Core.UI.ViewModels
{
	public sealed class AssetTreeNodeViewModel : NoxUI.ViewModelBase
	{
		#region 公開プロパティ
		public Core.AssetTreeNode Model { get; }
		public ObservableCollection<AssetTreeNodeViewModel> Children { get; } = new();
		public string Name => Model.Name;
		public string RelativePath => Model.RelativePath;
		public bool IsFolder => Model.IsFolder;
		public string DisplayPath => string.IsNullOrWhiteSpace(RelativePath) ? Name : RelativePath;
		public string Icon => IsFolder ? "" : GetIcon(Model.Asset?.Kind ?? Core.AssetKind.Unknown);
		#endregion

		public AssetTreeNodeViewModel(Core.AssetTreeNode model)
		{
			Model = model;
			foreach (Core.AssetTreeNode child in model.Children)
			{
				Children.Add(new AssetTreeNodeViewModel(child));
			}
		}

		private static string GetIcon(Core.AssetKind kind)
		{
			return kind switch
			{
				Core.AssetKind.Scene => "",
				Core.AssetKind.Model => "",
				Core.AssetKind.Texture => "",
				Core.AssetKind.Material => "",
				Core.AssetKind.Shader => "",
				Core.AssetKind.Script => "",
				Core.AssetKind.Audio => "",
				Core.AssetKind.Font => "",
				Core.AssetKind.Document => "",
				_ => "",
			};
		}
	}

	public sealed class ProjectAssetViewModel : NoxUI.ViewModelBase
	{
		#region 公開プロパティ
		public Core.ProjectAsset Asset { get; }
		public string Name => Asset.Name;
		public string RelativePath => Asset.RelativePath;
		public string Guid => Asset.Guid;
		public string Kind => Asset.Kind.ToString();
		public string SizeText => Asset.Kind == Core.AssetKind.Folder ? string.Empty : FormatSize(Asset.Size);
		public string LastWriteTime => Asset.LastWriteTime.ToString("yyyy/MM/dd HH:mm:ss");
		#endregion

		public ProjectAssetViewModel(Core.ProjectAsset asset)
		{
			Asset = asset;
		}

		private static string FormatSize(long size)
		{
			string[] units = ["B", "KB", "MB", "GB"];
			double value = size;
			int unitIndex = 0;
			while (value >= 1024 && unitIndex < units.Length - 1)
			{
				value /= 1024;
				++unitIndex;
			}

			return $"{value:0.#} {units[unitIndex]}";
		}
	}

	public sealed class AssetBrowserViewModel : NoxUI.ViewModelBase
	{
		#region 非公開フィールド
		private readonly Core.Workspace _Workspace;
		private readonly Core.AssetManager _AssetManager;
		private NoxUI.ViewModelCommand? _RefreshCommand;
		private NoxUI.ViewModelCommand? _OpenInExplorerCommand;
		#endregion

		#region 公開プロパティ
		public ObservableCollection<AssetTreeNodeViewModel> RootNodes { get; } = new();
		public ObservableCollection<ProjectAssetViewModel> Assets { get; } = new();

		public string SearchKeyword
		{
			get => field;
			set
			{
				if (SetProperty(ref field, value))
				{
					RefreshAssetList();
				}
			}
		} = string.Empty;

		public ProjectAssetViewModel? SelectedAsset
		{
			get => field;
			set
			{
				if (SetProperty(ref field, value))
				{
					_Workspace.Selection.SelectAsset(value?.Asset);
					RaisePropertyChanged(nameof(SelectedAssetName));
					RaisePropertyChanged(nameof(SelectedAssetPath));
					RaisePropertyChanged(nameof(SelectedAssetMetadata));
					_OpenInExplorerCommand?.RaiseCanExecuteChanged();
				}
			}
		}

		public AssetTreeNodeViewModel? SelectedTreeNode
		{
			get => field;
			set
			{
				if (SetProperty(ref field, value))
				{
					RefreshAssetList();
					RaisePropertyChanged(nameof(SelectedFolderText));
					_OpenInExplorerCommand?.RaiseCanExecuteChanged();
				}
			}
		}

		public string SelectedFolderText => SelectedTreeNode == null ? _Workspace.AssetFolderName : SelectedTreeNode.DisplayPath;
		public string StatusText => $"{Assets.Count} item(s) / {SelectedFolderText}";
		public string SelectedAssetName => SelectedAsset?.Name ?? "No asset selected";
		public string SelectedAssetPath => SelectedAsset?.RelativePath ?? "Select an asset to inspect or reveal it.";
		public string SelectedAssetMetadata => SelectedAsset == null
			? string.Empty
			: $"{SelectedAsset.Kind} / GUID {SelectedAsset.Guid} / {SelectedAsset.SizeText} / {SelectedAsset.LastWriteTime}";

		public NoxUI.ViewModelCommand RefreshCommand => _RefreshCommand ??= new(Refresh);
		public NoxUI.ViewModelCommand OpenInExplorerCommand => _OpenInExplorerCommand ??= new(OpenInExplorer, CanOpenInExplorer);
		#endregion

		public AssetBrowserViewModel()
		{
			_Workspace = Core.StudioManager.Instance.Workspace;
			_AssetManager = _Workspace.AssetManager;
			_AssetManager.Changed += OnAssetManagerChanged;
			Refresh();
		}

		#region 非公開メソッド
		private void Refresh()
		{
			_AssetManager.Refresh();
			RefreshTree();
			RefreshAssetList();
		}

		private void RefreshTree()
		{
			RootNodes.Clear();
			RootNodes.Add(new AssetTreeNodeViewModel(_AssetManager.RootNode));
		}

		private void RefreshAssetList()
		{
			Assets.Clear();
			foreach (Core.ProjectAsset asset in _AssetManager.Search(SearchKeyword).Where(IsInSelectedFolder))
			{
				Assets.Add(new ProjectAssetViewModel(asset));
			}

			RaisePropertyChanged(nameof(StatusText));
		}

		private void OnAssetManagerChanged(object? sender, EventArgs e)
		{
			System.Windows.Threading.Dispatcher? dispatcher = System.Windows.Application.Current?.Dispatcher;
			if (dispatcher != null && dispatcher.CheckAccess() == false)
			{
				dispatcher.BeginInvoke((Action)(() =>
				{
					RefreshTree();
					RefreshAssetList();
				}));
				return;
			}

			RefreshTree();
			RefreshAssetList();
		}

		private bool CanOpenInExplorer()
		{
			return SelectedAsset != null || SelectedTreeNode != null;
		}

		private void OpenInExplorer()
		{
			string path = GetExplorerPath();
			ProcessStartInfo psi = new()
			{
				FileName = "explorer.exe",
				Arguments = Directory.Exists(path) ? $"\"{path}\"" : $"/select,\"{path}\"",
				UseShellExecute = true,
			};
			Process.Start(psi);
		}

		private bool IsInSelectedFolder(Core.ProjectAsset asset)
		{
			if (SelectedTreeNode == null || string.IsNullOrWhiteSpace(SelectedTreeNode.RelativePath))
			{
				return true;
			}

			string selectedPath = SelectedTreeNode.RelativePath.TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
			return asset.RelativePath.Equals(selectedPath, StringComparison.OrdinalIgnoreCase) ||
				asset.RelativePath.StartsWith(selectedPath + Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase) ||
				asset.RelativePath.StartsWith(selectedPath + Path.AltDirectorySeparatorChar, StringComparison.OrdinalIgnoreCase);
		}

		private string GetExplorerPath()
		{
			if (SelectedAsset != null)
			{
				return SelectedAsset.Asset.FullPath;
			}

			if (SelectedTreeNode?.Model.Asset != null)
			{
				return SelectedTreeNode.Model.Asset.FullPath;
			}

			return _Workspace.AssetRootPath;
		}
		#endregion
	}
}
