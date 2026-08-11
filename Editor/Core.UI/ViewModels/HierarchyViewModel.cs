using System;
using System.Collections.ObjectModel;
using System.Linq;

namespace Core.UI.ViewModels;

	public sealed class HierarchyNodeViewModel : NoxUI.ViewModelBase
	{
		#region 公開プロパティ
		public Core.SceneHierarchyNode Model { get; }
		public ObservableCollection<HierarchyNodeViewModel> Children { get; } = new();
		public string Name
		{
			get => field;
			private set => SetProperty(ref field, value);
		}
		public string Kind => Model.Kind.ToString();
		public int ChildCount => Model.Children.Count;
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
		public string Icon => Model.Kind switch
		{
			Core.SceneHierarchyNodeKind.SceneNode => "",
			Core.SceneHierarchyNodeKind.GroupNode => "",
			_ => "",
		};
		#endregion

		public HierarchyNodeViewModel(Core.SceneHierarchyNode model, string keyword = "")
		{
			Model = model;
			Name = model.Name;
			EditingName = Name;
			foreach (Core.SceneHierarchyNode child in model.Children.Where(child => MatchesFilter(child, keyword)))
			{
				Children.Add(new HierarchyNodeViewModel(child, keyword));
			}
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

		public static bool MatchesFilter(Core.SceneHierarchyNode node, string keyword)
		{
			if (string.IsNullOrWhiteSpace(keyword))
			{
				return true;
			}

			return node.Name.Contains(keyword, StringComparison.OrdinalIgnoreCase) ||
				node.Kind.ToString().Contains(keyword, StringComparison.OrdinalIgnoreCase) ||
				node.Children.Any(child => MatchesFilter(child, keyword));
		}
	}

	public class HierarchyViewModel : NoxUI.ViewModelBase, IDisposable
	{
		#region 非公開フィールド
		private readonly Core.Workspace _Workspace;
		private readonly Core.SceneHierarchyManager _SceneHierarchy;
		private readonly Core.SelectionService _Selection;
		private readonly InspectorSyncSettings _SyncSettings;
		private NoxUI.ViewModelCommand? _AddRootEntityNodeCommand;
		private NoxUI.ViewModelCommand? _AddRootGroupNodeCommand;
		private NoxUI.ViewModelCommand? _AddChildEntityNodeCommand;
		private NoxUI.ViewModelCommand? _AddChildGroupNodeCommand;
		private NoxUI.ViewModelCommand? _DeleteSelectedCommand;
		private NoxUI.ViewModelCommand? _RefreshCommand;
		private NoxUI.ViewModelCommand? _SaveSceneCommand;
		private NoxUI.ViewModelCommand? _ManualSyncCommand;
		private NoxUI.ViewModelCommand? _RenameSelectedCommand;
		private Core.SceneHierarchyNode? _PendingSelectedNode;
		private bool _Disposed;
		#endregion

		#region 公開プロパティ
		public ObservableCollection<HierarchyNodeViewModel> RootNodes { get; } = new();

		public string SearchKeyword
		{
			get => field;
			set
			{
				if (SetProperty(ref field, value))
				{
					Refresh();
				}
			}
		} = string.Empty;

		public HierarchyNodeViewModel? SelectedNode
		{
			get => field;
			set
			{
				if (SetProperty(ref field, value))
				{
					_Selection.SelectHierarchyNode(value?.Model);
					RaisePropertyChanged(nameof(SelectedNodeSummary));
					RaisePropertyChanged(nameof(SelectedNodePath));
					RaisePropertyChanged(nameof(SelectedNodeName));
					_AddChildEntityNodeCommand?.RaiseCanExecuteChanged();
					_DeleteSelectedCommand?.RaiseCanExecuteChanged();
					_RenameSelectedCommand?.RaiseCanExecuteChanged();
				}
			}
		}

		public string SelectedNodeName
		{
			get => SelectedNode?.Model.Name ?? string.Empty;
			set
			{
				if (SelectedNode == null)
				{
					return;
				}

				_SceneHierarchy.Rename(SelectedNode.Model, value);
				RaisePropertyChanged();
				RaisePropertyChanged(nameof(SelectedNodeSummary));
				RaisePropertyChanged(nameof(SelectedNodePath));
			}
		}

		public string SelectedNodeSummary => SelectedNode == null
			? "Select a scene object to inspect it."
			: $"{SelectedNode.Name} ({SelectedNode.Kind}) / Children: {SelectedNode.ChildCount}";
		public string SelectedNodePath => SelectedNode == null ? string.Empty : BuildNodePath(SelectedNode.Model);
		public string NodeCountText => $"{CountVisibleNodes()} visible";
		public ObservableCollection<InspectorSyncIntervalOption> AutoSyncOptions => _SyncSettings.Options;
		public InspectorSyncIntervalOption SelectedAutoSyncOption
		{
			get => _SyncSettings.SelectedOption;
			set
			{
				_SyncSettings.SelectedOption = value;
				RaisePropertyChanged();
				RaisePropertyChanged(nameof(IsManualSync));
				_ManualSyncCommand?.RaiseCanExecuteChanged();
			}
		}
		public bool IsManualSync => _SyncSettings.IsManual;

		public NoxUI.ViewModelCommand AddRootEntityNodeCommand => _AddRootEntityNodeCommand ??= new(AddRootEntityNode);
		public NoxUI.ViewModelCommand AddRootGroupNodeCommand => _AddRootGroupNodeCommand ??= new(AddRootGroupNode);
		public NoxUI.ViewModelCommand AddChildEntityNodeCommand => _AddChildEntityNodeCommand ??= new(AddChildEntityNode, CanAddChildEntityNode);
		public NoxUI.ViewModelCommand AddChildGroupNodeCommand => _AddChildGroupNodeCommand ??= new(AddChildGroupNode, CanAddChildEntityNode);
		public NoxUI.ViewModelCommand DeleteSelectedCommand => _DeleteSelectedCommand ??= new(DeleteSelected, CanDeleteSelected);
		public NoxUI.ViewModelCommand RefreshCommand => _RefreshCommand ??= new(Refresh);
		public NoxUI.ViewModelCommand SaveSceneCommand => _SaveSceneCommand ??= new(SaveScene);
		public NoxUI.ViewModelCommand ManualSyncCommand => _ManualSyncCommand ??= new(ManualSync, () => IsManualSync);
		public NoxUI.ViewModelCommand RenameSelectedCommand => _RenameSelectedCommand ??= new(BeginRenameSelected, CanRenameSelected);
		#endregion

		public HierarchyViewModel()
		{
			_Workspace = Core.StudioManager.Instance.Workspace;
			_SceneHierarchy = _Workspace.SceneHierarchy;
			_Selection = _Workspace.Selection;
			_SyncSettings = InspectorSyncSettings.Instance;
			_SyncSettings.Changed += OnSyncSettingsChanged;
			_SceneHierarchy.Changed += OnSceneHierarchyChanged;
			Refresh();
		}

		#region 非公開メソッド
		private void AddRootEntityNode()
		{
			SelectedNode = new HierarchyNodeViewModel(_SceneHierarchy.AddEntityNode(CreateUniqueName("EntityNode")));
		}

		private void AddRootGroupNode()
		{
			SelectedNode = new HierarchyNodeViewModel(_SceneHierarchy.AddGroupNode(CreateUniqueName("GroupNode")));
		}

		private bool CanAddChildEntityNode()
		{
			return SelectedNode != null;
		}

		private void AddChildEntityNode()
		{
			if (SelectedNode == null)
			{
				return;
			}

			SelectedNode = new HierarchyNodeViewModel(_SceneHierarchy.AddEntityNode(CreateUniqueName("EntityNode"), SelectedNode.Model));
		}

		private void AddChildGroupNode()
		{
			if (SelectedNode == null)
			{
				return;
			}

			SelectedNode = new HierarchyNodeViewModel(_SceneHierarchy.AddGroupNode(CreateUniqueName("GroupNode"), SelectedNode.Model));
		}

		private bool CanDeleteSelected()
		{
			return SelectedNode != null && SelectedNode.Model.Kind != Core.SceneHierarchyNodeKind.SceneNode;
		}

		private void DeleteSelected()
		{
			if (SelectedNode == null)
			{
				return;
			}

			Core.SceneHierarchyNode? parent = SelectedNode.Model.Parent;
			if (_SceneHierarchy.Remove(SelectedNode.Model))
			{
				SelectedNode = parent == null || parent.Kind == Core.SceneHierarchyNodeKind.SceneNode
					? null
					: new HierarchyNodeViewModel(parent);
			}
		}

		private void Refresh()
		{
			Core.SceneHierarchyNode? selectedNode = _PendingSelectedNode ?? SelectedNode?.Model;
			_PendingSelectedNode = null;

			RootNodes.Clear();
			HierarchyNodeViewModel? selectedViewModel = null;
			foreach (Core.SceneHierarchyNode node in _SceneHierarchy.SceneNodes.Where(node => HierarchyNodeViewModel.MatchesFilter(node, SearchKeyword)))
			{
				HierarchyNodeViewModel viewModel = new(node, SearchKeyword);
				RootNodes.Add(viewModel);
				selectedViewModel ??= selectedNode == null ? null : FindNode(viewModel, selectedNode);
			}

			SelectedNode = selectedViewModel;
			RaisePropertyChanged(nameof(NodeCountText));
		}

		private void OnSceneHierarchyChanged(object? sender, EventArgs e)
		{
			System.Windows.Threading.Dispatcher? dispatcher = System.Windows.Application.Current?.Dispatcher;
			if (dispatcher != null && dispatcher.CheckAccess() == false)
			{
				NoxUI.DispatcherHelper.TryBeginInvoke(dispatcher, Refresh);
				return;
			}

			_PendingSelectedNode ??= SelectedNode?.Model;
			Refresh();
			RaisePropertyChanged(nameof(SelectedNodeName));
			RaisePropertyChanged(nameof(SelectedNodeSummary));
			RaisePropertyChanged(nameof(SelectedNodePath));
		}

		private void OnSyncSettingsChanged(object? sender, EventArgs e)
		{
			RaisePropertyChanged(nameof(SelectedAutoSyncOption));
			RaisePropertyChanged(nameof(IsManualSync));
			_ManualSyncCommand?.RaiseCanExecuteChanged();
		}

		private void ManualSync()
		{
			_SyncSettings.RequestManualSync();
		}

		private void SaveScene()
		{
			string filePath = _SceneHierarchy.SaveScene(_Workspace.AssetRootPath);
			_Workspace.AssetManager.Refresh();
			Nox.LogTrace.InfoLine<Core.LogId.Runtime>($"Scene saved: {filePath}");
		}

		private string CreateUniqueName(string baseName)
		{
			int existingCount = _SceneHierarchy.SceneNodes.Sum(CountNodes);
			return $"{baseName} {existingCount + 1}";
		}

		private bool CanRenameSelected()
		{
			return SelectedNode != null;
		}

		private void BeginRenameSelected()
		{
			if (SelectedNode == null)
			{
				return;
			}

			CancelRenameStates();
			SelectedNode.BeginRename();
		}

		public void CancelRename(object item)
		{
			if (item is HierarchyNodeViewModel node)
			{
				node.CancelRename();
			}
		}

		public void CommitRename(object item)
		{
			if (item is not HierarchyNodeViewModel node || node.IsRenaming == false)
			{
				return;
			}

			CommitRename(node, keepEditingOnFailure: true);
		}

		public void CompleteRenameOnFocusLoss(object item)
		{
			if (item is HierarchyNodeViewModel node)
			{
				CommitRename(node, keepEditingOnFailure: false);
			}
		}

		public void CompleteActiveRenameOnFocusLoss()
		{
			foreach (HierarchyNodeViewModel rootNode in RootNodes)
			{
				if (CompleteRenamingNode(rootNode))
				{
					return;
				}
			}
		}

		private void CancelRenameStates()
		{
			foreach (HierarchyNodeViewModel rootNode in RootNodes)
			{
				CancelRenameRecursive(rootNode);
			}
		}

		private static void CancelRenameRecursive(HierarchyNodeViewModel node)
		{
			node.CancelRename();
			foreach (HierarchyNodeViewModel child in node.Children)
			{
				CancelRenameRecursive(child);
			}
		}

		private bool CompleteRenamingNode(HierarchyNodeViewModel node)
		{
			if (node.IsRenaming)
			{
				CommitRename(node, keepEditingOnFailure: false);
				return true;
			}

			foreach (HierarchyNodeViewModel child in node.Children)
			{
				if (CompleteRenamingNode(child))
				{
					return true;
				}
			}

			return false;
		}

		private static int CountNodes(Core.SceneHierarchyNode node)
		{
			return 1 + node.Children.Sum(CountNodes);
		}

		private void CommitRename(HierarchyNodeViewModel node, bool keepEditingOnFailure)
		{
			string trimmedName = node.EditingName.Trim();
			if (string.IsNullOrWhiteSpace(trimmedName))
			{
				node.CancelRename();
				return;
			}

			try
			{
				node.IsRenaming = false;
				_PendingSelectedNode = node.Model;
				_SceneHierarchy.Rename(node.Model, trimmedName);
			}
			catch (Exception ex)
			{
				Nox.LogTrace.ErrorLine<Core.LogId.Runtime>($"Hierarchy rename failed: {node.Model.Name}, {ex}");
				Core.UI.MessageBox.ShowDialog(
					$"Failed to rename hierarchy node.\n{ex.Message}",
					"Hierarchy",
					System.Windows.MessageBoxImage.Error);
				if (keepEditingOnFailure)
				{
					node.IsRenaming = true;
					return;
				}

				node.CancelRename();
			}
		}

		private int CountVisibleNodes()
		{
			return RootNodes.Sum(CountNodeViewModels);
		}

		private static int CountNodeViewModels(HierarchyNodeViewModel node)
		{
			return 1 + node.Children.Sum(CountNodeViewModels);
		}

		private static HierarchyNodeViewModel? FindNode(HierarchyNodeViewModel root, Core.SceneHierarchyNode target)
		{
			if (ReferenceEquals(root.Model, target))
			{
				return root;
			}

			foreach (HierarchyNodeViewModel child in root.Children)
			{
				HierarchyNodeViewModel? found = FindNode(child, target);
				if (found != null)
				{
					return found;
				}
			}

			return null;
		}

		private static string BuildNodePath(Core.SceneHierarchyNode node)
		{
			if (node.Parent == null || node.Parent.Kind == Core.SceneHierarchyNodeKind.SceneNode)
			{
				return node.Name;
			}

			return $"{BuildNodePath(node.Parent)} / {node.Name}";
		}

		public override void Dispose()
		{
			if (_Disposed)
			{
				return;
			}

			_SyncSettings.Changed -= OnSyncSettingsChanged;
			_SceneHierarchy.Changed -= OnSceneHierarchyChanged;
			_Disposed = true;
		}
		#endregion
	}
