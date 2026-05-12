using System;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Xml.Linq;

namespace Core.UI.ViewModels
{
	public enum AssetBrowserContentViewMode : byte
	{
		Details,
		Thumbnail,
	}

	public sealed record AssetBrowserViewModeOption(AssetBrowserContentViewMode Value, string DisplayName);

	public sealed class AssetTreeNodeViewModel : NoxUI.ViewModelBase
	{
		#region Non-public fields
		private readonly bool _IsRoot;
		#endregion

		#region Public properties
		public Core.AssetTreeNode Model { get; }
		public ObservableCollection<AssetTreeNodeViewModel> Children { get; } = new();
		public string Name
		{
			get => field;
			private set => SetProperty(ref field, value);
		}
		public string RelativePath => Model.RelativePath;
		public bool IsFolder => true;
		public bool CanRename => _IsRoot == false;
		public string EditingName
		{
			get => field;
			set => SetProperty(ref field, value);
		}
		public bool IsRenaming
		{
			get => field;
			set => SetProperty(ref field, value);
		}
		public string DisplayPath => _IsRoot || string.IsNullOrWhiteSpace(RelativePath) ? "Assets://" : $"Assets://{NormalizePath(RelativePath)}";
		public string Icon => "";

		public bool IsExpanded
		{
			get => field;
			set => SetProperty(ref field, value);
		} = true;

		public bool IsSelected
		{
			get => field;
			set => SetProperty(ref field, value);
		}
		#endregion

		public AssetTreeNodeViewModel(Core.AssetTreeNode model, bool isRoot = false)
		{
			Model = model;
			_IsRoot = isRoot;
			Name = _IsRoot ? "Assets://" : Model.Name;
			EditingName = Name;
			foreach (Core.AssetTreeNode child in model.Children.Where(static child => child.IsFolder))
			{
				Children.Add(new AssetTreeNodeViewModel(child));
			}
		}

		public void BeginRename()
		{
			if (CanRename == false)
			{
				return;
			}

			EditingName = Name;
			IsRenaming = true;
		}

		public void CancelRename()
		{
			EditingName = Name;
			IsRenaming = false;
		}

		private static string NormalizePath(string relativePath)
		{
			return relativePath.Replace(Path.DirectorySeparatorChar, '/').Replace(Path.AltDirectorySeparatorChar, '/');
		}
	}

	public sealed class ProjectAssetViewModel : NoxUI.ViewModelBase
	{
		#region Public properties
		public Core.ProjectAsset Asset { get; }
		public string Name
		{
			get => field;
			private set => SetProperty(ref field, value);
		}
		public string RelativePath => Asset.RelativePath;
		public string Guid => Asset.Guid;
		public string Kind => Asset.Kind.ToString();
		public string Extension => Asset.Extension;
		public string Icon => GetIcon(Asset.Kind);
		public string SizeText => FormatSize(Asset.Size);
		public string LastWriteTime => Asset.LastWriteTime.ToString("yyyy/MM/dd HH:mm:ss");
		public string EditingName
		{
			get => field;
			set => SetProperty(ref field, value);
		}
		public bool IsRenaming
		{
			get => field;
			set => SetProperty(ref field, value);
		}
		#endregion

		public ProjectAssetViewModel(Core.ProjectAsset asset)
		{
			Asset = asset;
			Name = asset.Name;
			EditingName = Name;
		}

		public void BeginRename()
		{
			EditingName = Name;
			IsRenaming = true;
		}

		public void CancelRename()
		{
			EditingName = Name;
			IsRenaming = false;
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

	public sealed class AssetBrowserViewModel : NoxUI.ViewModelBase, IDisposable
	{
		#region Non-public fields
		private readonly Core.Workspace _Workspace;
		private readonly Core.AssetManager _AssetManager;
		private AssetTreeNodeViewModel? _SelectedTreeNode;
		private NoxUI.ViewModelCommand? _RefreshCommand;
		private NoxUI.ViewModelCommand? _OpenInExplorerCommand;
		private NoxUI.ViewModelCommand? _RenameAssetCommand;
		private NoxUI.ViewModelCommand? _RenameFolderCommand;
		private NoxUI.ViewModelCommand? _CreateFolderCommand;
		private NoxUI.ViewModelCommand? _CreateScriptCommand;
		private string? _PendingTreeSelectionRelativePath;
		private string? _PendingAssetSelectionRelativePath;
		private string? _PendingFolderRenameRelativePath;
		private bool _Disposed;
		#endregion

		#region Public properties
		public ObservableCollection<AssetTreeNodeViewModel> RootNodes { get; } = new();
		public ObservableCollection<ProjectAssetViewModel> Assets { get; } = new();
		public ObservableCollection<AssetBrowserViewModeOption> ViewModes { get; } =
		[
			new(AssetBrowserContentViewMode.Details, "Details"),
			new(AssetBrowserContentViewMode.Thumbnail, "Thumbnails"),
		];

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
					_OpenInExplorerCommand?.RaiseCanExecuteChanged();
					_RenameAssetCommand?.RaiseCanExecuteChanged();
				}
			}
		}

		public AssetTreeNodeViewModel? SelectedTreeNode
		{
			get => _SelectedTreeNode;
			set => UpdateSelectedTreeNode(value);
		}

		public AssetBrowserViewModeOption? SelectedViewMode
		{
			get => field;
			set
			{
				if (SetProperty(ref field, value))
				{
					RaisePropertyChanged(nameof(IsDetailsView));
					RaisePropertyChanged(nameof(IsThumbnailView));
				}
			}
		}

		public bool IsDetailsView => SelectedViewMode?.Value != AssetBrowserContentViewMode.Thumbnail;
		public bool IsThumbnailView => SelectedViewMode?.Value == AssetBrowserContentViewMode.Thumbnail;
		public string SelectedFolderText => SelectedTreeNode?.DisplayPath ?? "Assets://";
		public string StatusText => $"{Assets.Count} item(s) in {SelectedFolderText}";

		public NoxUI.ViewModelCommand RefreshCommand => _RefreshCommand ??= new(Refresh);
		public NoxUI.ViewModelCommand OpenInExplorerCommand => _OpenInExplorerCommand ??= new(OpenInExplorer, CanOpenInExplorer);
		public NoxUI.ViewModelCommand RenameAssetCommand => _RenameAssetCommand ??= new(BeginRenameAsset, CanRenameAsset);
		public NoxUI.ViewModelCommand RenameFolderCommand => _RenameFolderCommand ??= new(BeginRenameFolder, CanRenameFolder);
		public NoxUI.ViewModelCommand CreateFolderCommand => _CreateFolderCommand ??= new(CreateFolder);
		public NoxUI.ViewModelCommand CreateScriptCommand => _CreateScriptCommand ??= new(CreateScript, CanCreateScript);
		#endregion

		public AssetBrowserViewModel()
		{
			_Workspace = Core.StudioManager.Instance.Workspace;
			_AssetManager = _Workspace.AssetManager;
			_AssetManager.Changed += OnAssetManagerChanged;
			SelectedViewMode = ViewModes.FirstOrDefault(static option => option.Value == AssetBrowserContentViewMode.Details);
			Refresh();
		}

		#region Non-public methods
		private void Refresh()
		{
			_AssetManager.Refresh();
		}

		private void RefreshTree()
		{
			string selectedPath = _PendingTreeSelectionRelativePath ?? SelectedTreeNode?.RelativePath ?? string.Empty;
			_PendingTreeSelectionRelativePath = null;

			RootNodes.Clear();
			AssetTreeNodeViewModel rootNode = new(_AssetManager.RootNode, isRoot: true);
			RootNodes.Add(rootNode);

			AssetTreeNodeViewModel selectedNode = FindTreeNode(rootNode, selectedPath) ?? rootNode;
			UpdateSelectedTreeNode(selectedNode);
			if (string.Equals(_PendingFolderRenameRelativePath, selectedNode.RelativePath, StringComparison.OrdinalIgnoreCase))
			{
				selectedNode.BeginRename();
			}

			_PendingFolderRenameRelativePath = null;
		}

		private void RefreshAssetList()
		{
			string? selectedAssetPath = _PendingAssetSelectionRelativePath ?? SelectedAsset?.RelativePath;
			_PendingAssetSelectionRelativePath = null;

			Assets.Clear();
			foreach (Core.ProjectAsset asset in _AssetManager.Search(SearchKeyword).Where(IsInSelectedFolder))
			{
				Assets.Add(new ProjectAssetViewModel(asset));
			}

			SelectedAsset = selectedAssetPath == null
				? null
				: Assets.FirstOrDefault(asset => string.Equals(asset.RelativePath, selectedAssetPath, StringComparison.OrdinalIgnoreCase));
			RaisePropertyChanged(nameof(StatusText));
			_RenameAssetCommand?.RaiseCanExecuteChanged();
		}

		private void OnAssetManagerChanged(object? sender, EventArgs e)
		{
			System.Windows.Threading.Dispatcher? dispatcher = System.Windows.Application.Current?.Dispatcher;
			if (dispatcher != null && dispatcher.CheckAccess() == false)
			{
				NoxUI.DispatcherHelper.TryBeginInvoke(dispatcher, () =>
				{
					RefreshTree();
				});
				return;
			}

			RefreshTree();
		}

		private bool CanOpenInExplorer()
		{
			return SelectedAsset != null || SelectedTreeNode != null;
		}

		private bool CanRenameAsset()
		{
			return SelectedAsset != null;
		}

		private bool CanRenameFolder()
		{
			return SelectedTreeNode?.CanRename == true;
		}

		private bool CanCreateScript()
		{
			return Directory.Exists(GetAppRootPath());
		}

		private void BeginRenameAsset()
		{
			if (SelectedAsset == null)
			{
				return;
			}

			CancelAllRenameStates();
			SelectedAsset.BeginRename();
		}

		private void BeginRenameFolder()
		{
			if (SelectedTreeNode?.CanRename != true)
			{
				return;
			}

			CancelAllRenameStates();
			SelectedTreeNode.BeginRename();
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

		private void CreateFolder()
		{
			try
			{
				string newRelativePath = _AssetManager.CreateFolder(SelectedTreeNode?.RelativePath ?? string.Empty, "New Folder");
				_PendingTreeSelectionRelativePath = newRelativePath;
				_PendingAssetSelectionRelativePath = null;
				_PendingFolderRenameRelativePath = newRelativePath;
				_AssetManager.Refresh();
			}
			catch (Exception ex) when (ex is IOException or UnauthorizedAccessException or InvalidOperationException or NotSupportedException)
			{
				Nox.LogTrace.ErrorLine<Core.LogId.Runtime>($"Create folder failed: {SelectedTreeNode?.RelativePath}, {ex}");
				Core.UI.MessageBox.ShowDialog(
					$"Failed to create folder.\n{ex.Message}",
					"Asset Browser",
					System.Windows.MessageBoxImage.Error);
			}
		}

		private void CreateScript()
		{
			try
			{
				string appRootPath = GetAppRootPath();
				string relativeDirectory = SelectedTreeNode?.RelativePath ?? string.Empty;
				string targetDirectory = string.IsNullOrWhiteSpace(relativeDirectory)
					? appRootPath
					: Path.Combine(appRootPath, relativeDirectory);
				Directory.CreateDirectory(targetDirectory);

				string scriptBaseName = GetUniqueScriptBaseName(targetDirectory, "new_behavior");
				string headerFileName = scriptBaseName + ".h";
				string sourceFileName = scriptBaseName + ".cpp";
				string headerPath = Path.Combine(targetDirectory, headerFileName);
				string sourcePath = Path.Combine(targetDirectory, sourceFileName);
				string includePath = string.IsNullOrWhiteSpace(relativeDirectory)
					? headerFileName
					: NormalizeIncludePath(Path.Combine(relativeDirectory, headerFileName));
				string className = ToPascalCase(scriptBaseName);

				File.WriteAllText(headerPath, CreateScriptHeaderContent(headerFileName, scriptBaseName, className));
				File.WriteAllText(sourcePath, CreateScriptSourceContent(sourceFileName, headerFileName));

				string projectRelativeHeaderPath = string.IsNullOrWhiteSpace(relativeDirectory)
					? headerFileName
					: Path.Combine(relativeDirectory, headerFileName);
				string projectRelativeSourcePath = string.IsNullOrWhiteSpace(relativeDirectory)
					? sourceFileName
					: Path.Combine(relativeDirectory, sourceFileName);
				AddFileToAppProject(projectRelativeHeaderPath, projectRelativeSourcePath);
				AddIncludeToAllIncludeHeader(includePath);
				Nox.LogTrace.InfoLine<Core.LogId.Runtime>($"Created app script: {sourcePath}");
			}
			catch (Exception ex) when (ex is IOException or UnauthorizedAccessException or InvalidOperationException or NotSupportedException or System.Xml.XmlException)
			{
				Nox.LogTrace.ErrorLine<Core.LogId.Runtime>($"Create script failed: {ex}");
				Core.UI.MessageBox.ShowDialog(
					$"Failed to create C++ script.\n{ex.Message}",
					"Asset Browser",
					System.Windows.MessageBoxImage.Error);
			}
		}

		private bool IsInSelectedFolder(Core.ProjectAsset asset)
		{
			string selectedPath = SelectedTreeNode?.RelativePath ?? string.Empty;
			return string.Equals(GetParentRelativePath(asset.RelativePath), selectedPath, StringComparison.OrdinalIgnoreCase);
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

		private string GetAppRootPath()
		{
			return Path.Combine(_Workspace.RuntimeRootPath, "app");
		}

		private void UpdateSelectedTreeNode(AssetTreeNodeViewModel? value)
		{
			AssetTreeNodeViewModel? previous = SelectedTreeNode;
			if (ReferenceEquals(previous, value))
			{
				return;
			}

			if (previous != null)
			{
				previous.IsSelected = false;
			}

			SetProperty(ref _SelectedTreeNode, value, nameof(SelectedTreeNode));
			if (value != null)
			{
				value.IsSelected = true;
			}

			RefreshAssetList();
			RaisePropertyChanged(nameof(SelectedFolderText));
			_OpenInExplorerCommand?.RaiseCanExecuteChanged();
			_RenameFolderCommand?.RaiseCanExecuteChanged();
			_CreateScriptCommand?.RaiseCanExecuteChanged();
		}

		private static AssetTreeNodeViewModel? FindTreeNode(AssetTreeNodeViewModel root, string relativePath)
		{
			if (string.Equals(root.RelativePath, relativePath, StringComparison.OrdinalIgnoreCase))
			{
				return root;
			}

			foreach (AssetTreeNodeViewModel child in root.Children)
			{
				AssetTreeNodeViewModel? found = FindTreeNode(child, relativePath);
				if (found != null)
				{
					return found;
				}
			}

			return null;
		}

		private static string GetParentRelativePath(string relativePath)
		{
			string? parentPath = Path.GetDirectoryName(relativePath);
			if (string.IsNullOrWhiteSpace(parentPath) || parentPath == ".")
			{
				return string.Empty;
			}

			return parentPath;
		}

		private static string GetUniqueScriptBaseName(string directoryPath, string baseName)
		{
			string candidate = baseName;
			int suffix = 1;
			while (File.Exists(Path.Combine(directoryPath, candidate + ".h")) ||
				File.Exists(Path.Combine(directoryPath, candidate + ".cpp")))
			{
				candidate = $"{baseName}_{suffix.ToString(System.Globalization.CultureInfo.InvariantCulture)}";
				++suffix;
			}

			return candidate;
		}

		private static string NormalizeIncludePath(string path)
		{
			return path.Replace(Path.DirectorySeparatorChar, '/').Replace(Path.AltDirectorySeparatorChar, '/');
		}

		private static string ToPascalCase(string value)
		{
			string[] parts = value.Split(['_', '-', ' '], StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries);
			return string.Concat(parts.Select(static part =>
				part.Length == 0
					? string.Empty
					: char.ToUpperInvariant(part[0]) + part[1..]));
		}

		private static string CreateScriptHeaderContent(string fileName, string baseName, string className)
		{
			return
				$"//\tCopyright (C) 2023 NOX ENGINE All Rights Rserved.{Environment.NewLine}{Environment.NewLine}" +
				$"///\t@file\t{fileName}{Environment.NewLine}" +
				$"///\t@brief\t{baseName}{Environment.NewLine}" +
				$"#pragma once{Environment.NewLine}{Environment.NewLine}" +
				$"namespace app{Environment.NewLine}" +
				$"{{{Environment.NewLine}" +
				$"\tclass {className} : public nox::Behavior{Environment.NewLine}" +
				$"\t{{{Environment.NewLine}" +
				$"\t\tNOX_DECLARE_MANAGED_OBJECT({className}, nox::Behavior);{Environment.NewLine}" +
				$"\tpublic:{Environment.NewLine}" +
				$"\t\t{Environment.NewLine}" +
				$"\t}};{Environment.NewLine}" +
				$"}}{Environment.NewLine}";
		}

		private static string CreateScriptSourceContent(string fileName, string headerFileName)
		{
			return
				$"//\tCopyright (C) 2023 NOX ENGINE All Rights Rserved.{Environment.NewLine}{Environment.NewLine}" +
				$"///\t@file\t{fileName}{Environment.NewLine}" +
				$"///\t@brief\t{Path.GetFileNameWithoutExtension(fileName)}{Environment.NewLine}" +
				$"#include\t\"pch.h\"{Environment.NewLine}" +
				$"#include\t\"{headerFileName}\"{Environment.NewLine}";
		}

		private void AddFileToAppProject(string headerRelativePath, string sourceRelativePath)
		{
			string projectPath = Path.Combine(GetAppRootPath(), "app.vcxproj");
			string filterPath = projectPath + ".filters";
			XNamespace ns = "http://schemas.microsoft.com/developer/msbuild/2003";

			XDocument projectDocument = XDocument.Load(projectPath);
			AppendItem(projectDocument, ns, "ClInclude", headerRelativePath);
			AppendItem(projectDocument, ns, "ClCompile", sourceRelativePath);
			projectDocument.Save(projectPath);

			XDocument filterDocument = XDocument.Load(filterPath);
			AppendFilterItem(filterDocument, ns, "ClInclude", headerRelativePath, "ヘッダー ファイル");
			AppendFilterItem(filterDocument, ns, "ClCompile", sourceRelativePath, "ソース ファイル");
			filterDocument.Save(filterPath);
		}

		private static void AppendItem(XDocument document, XNamespace ns, string itemName, string includePath)
		{
			if (document.Descendants(ns + itemName).Any(element => string.Equals((string?)element.Attribute("Include"), includePath, StringComparison.OrdinalIgnoreCase)))
			{
				return;
			}

			XElement? itemGroup = document.Root?.Elements(ns + "ItemGroup").FirstOrDefault(group => group.Elements(ns + itemName).Any());
			if (itemGroup == null)
			{
				itemGroup = new XElement(ns + "ItemGroup");
				document.Root?.Add(itemGroup);
			}

			itemGroup.Add(new XElement(ns + itemName, new XAttribute("Include", includePath)));
		}

		private static void AppendFilterItem(XDocument document, XNamespace ns, string itemName, string includePath, string filterName)
		{
			if (document.Descendants(ns + itemName).Any(element => string.Equals((string?)element.Attribute("Include"), includePath, StringComparison.OrdinalIgnoreCase)))
			{
				return;
			}

			XElement? itemGroup = document.Root?.Elements(ns + "ItemGroup").FirstOrDefault(group => group.Elements(ns + itemName).Any());
			if (itemGroup == null)
			{
				itemGroup = new XElement(ns + "ItemGroup");
				document.Root?.Add(itemGroup);
			}

			itemGroup.Add(
				new XElement(ns + itemName,
					new XAttribute("Include", includePath),
					new XElement(ns + "Filter", filterName)));
		}

		private void AddIncludeToAllIncludeHeader(string includePath)
		{
			string allIncludeHeaderPath = Path.Combine(GetAppRootPath(), "all_include.h");
			string includeLine = $"#include\t\"{includePath}\"";
			string content = File.ReadAllText(allIncludeHeaderPath);
			if (content.Contains(includeLine, StringComparison.Ordinal))
			{
				return;
			}

			string updatedContent = content.TrimEnd() + Environment.NewLine + includeLine + Environment.NewLine;
			File.WriteAllText(allIncludeHeaderPath, updatedContent);
		}

		private void CancelAllRenameStates()
		{
			foreach (AssetTreeNodeViewModel rootNode in RootNodes)
			{
				CancelRenameRecursive(rootNode);
			}

			foreach (ProjectAssetViewModel asset in Assets)
			{
				asset.CancelRename();
			}
		}

		private static void CancelRenameRecursive(AssetTreeNodeViewModel node)
		{
			node.CancelRename();
			foreach (AssetTreeNodeViewModel child in node.Children)
			{
				CancelRenameRecursive(child);
			}
		}

		private bool CompleteRenamingTreeNode(AssetTreeNodeViewModel node)
		{
			if (node.IsRenaming)
			{
				CommitFolderRename(node, keepEditingOnFailure: false);
				return true;
			}

			foreach (AssetTreeNodeViewModel child in node.Children)
			{
				if (CompleteRenamingTreeNode(child))
				{
					return true;
				}
			}

			return false;
		}

		public void CancelRename(object item)
		{
			switch (item)
			{
				case ProjectAssetViewModel asset:
					asset.CancelRename();
					break;
				case AssetTreeNodeViewModel node:
					node.CancelRename();
					break;
			}
		}

		public void CommitRename(object item)
		{
			switch (item)
			{
				case ProjectAssetViewModel asset:
					CommitAssetRename(asset, keepEditingOnFailure: true);
					break;
				case AssetTreeNodeViewModel node:
					CommitFolderRename(node, keepEditingOnFailure: true);
					break;
			}
		}

		public void CompleteRenameOnFocusLoss(object item)
		{
			switch (item)
			{
				case ProjectAssetViewModel asset:
					CommitAssetRename(asset, keepEditingOnFailure: false);
					break;
				case AssetTreeNodeViewModel node:
					CommitFolderRename(node, keepEditingOnFailure: false);
					break;
			}
		}

		public void CompleteActiveRenameOnFocusLoss()
		{
			foreach (ProjectAssetViewModel asset in Assets)
			{
				if (asset.IsRenaming)
				{
					CommitAssetRename(asset, keepEditingOnFailure: false);
					return;
				}
			}

			foreach (AssetTreeNodeViewModel rootNode in RootNodes)
			{
				if (CompleteRenamingTreeNode(rootNode))
				{
					return;
				}
			}
		}

		private void CommitAssetRename(ProjectAssetViewModel asset, bool keepEditingOnFailure)
		{
			if (asset.IsRenaming == false)
			{
				return;
			}

			if (string.IsNullOrWhiteSpace(asset.EditingName))
			{
				asset.CancelRename();
				return;
			}

			try
			{
				string newRelativePath = _AssetManager.Rename(asset.Asset, asset.EditingName);
				asset.IsRenaming = false;
				_PendingTreeSelectionRelativePath = GetParentRelativePath(newRelativePath);
				_PendingAssetSelectionRelativePath = newRelativePath;
				_AssetManager.Refresh();
			}
			catch (Exception ex) when (ex is ArgumentException or IOException or UnauthorizedAccessException or InvalidOperationException or NotSupportedException)
			{
				Nox.LogTrace.ErrorLine<Core.LogId.Runtime>($"Asset rename failed: {asset.RelativePath}, {ex}");
				Core.UI.MessageBox.ShowDialog(
					$"Failed to rename asset.\n{ex.Message}",
					"Asset Browser",
					System.Windows.MessageBoxImage.Error);
				if (keepEditingOnFailure)
				{
					asset.IsRenaming = true;
					return;
				}

				asset.CancelRename();
			}
		}

		private void CommitFolderRename(AssetTreeNodeViewModel node, bool keepEditingOnFailure)
		{
			if (node.IsRenaming == false || node.Model.Asset == null)
			{
				return;
			}

			if (string.IsNullOrWhiteSpace(node.EditingName))
			{
				node.CancelRename();
				return;
			}

			try
			{
				string newRelativePath = _AssetManager.Rename(node.Model.Asset, node.EditingName);
				node.IsRenaming = false;
				_PendingTreeSelectionRelativePath = newRelativePath;
				_PendingAssetSelectionRelativePath = null;
				SelectedAsset = null;
				_AssetManager.Refresh();
			}
			catch (Exception ex) when (ex is ArgumentException or IOException or UnauthorizedAccessException or InvalidOperationException or NotSupportedException)
			{
				Nox.LogTrace.ErrorLine<Core.LogId.Runtime>($"Folder rename failed: {node.RelativePath}, {ex}");
				Core.UI.MessageBox.ShowDialog(
					$"Failed to rename folder.\n{ex.Message}",
					"Asset Browser",
					System.Windows.MessageBoxImage.Error);
				if (keepEditingOnFailure)
				{
					node.IsRenaming = true;
					return;
				}

				node.CancelRename();
			}
		}

		public override void Dispose()
		{
			if (_Disposed)
			{
				return;
			}

			_AssetManager.Changed -= OnAssetManagerChanged;
			_Disposed = true;
		}
		#endregion
	}
}
