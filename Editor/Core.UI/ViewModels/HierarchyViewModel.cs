using System;
using System.Collections.ObjectModel;
using System.Linq;

namespace Core.UI.ViewModels
{
	public sealed class HierarchyNodeViewModel : NoxUI.ViewModelBase
	{
		#region 公開プロパティ
		public Core.SceneHierarchyNode Model { get; }
		public ObservableCollection<HierarchyNodeViewModel> Children { get; } = new();
		public string Name => Model.Name;
		public string Kind => Model.Kind.ToString();
		public int ChildCount => Model.Children.Count;
		public string Icon => Model.Kind switch
		{
			Core.SceneHierarchyNodeKind.Scene => "",
			Core.SceneHierarchyNodeKind.Camera => "",
			Core.SceneHierarchyNodeKind.Light => "",
			Core.SceneHierarchyNodeKind.Folder => "",
			_ => "",
		};
		#endregion

		public HierarchyNodeViewModel(Core.SceneHierarchyNode model, string keyword = "")
		{
			Model = model;
			foreach (Core.SceneHierarchyNode child in model.Children.Where(child => MatchesFilter(child, keyword)))
			{
				Children.Add(new HierarchyNodeViewModel(child, keyword));
			}
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

	public class HierarchyViewModel : NoxUI.ViewModelBase
	{
		#region 非公開フィールド
		private readonly Core.SceneHierarchyManager _SceneHierarchy;
		private readonly Core.SelectionService _Selection;
		private NoxUI.ViewModelCommand? _AddRootEntityNodeCommand;
		private NoxUI.ViewModelCommand? _AddChildEntityNodeCommand;
		private NoxUI.ViewModelCommand? _DeleteSelectedCommand;
		private NoxUI.ViewModelCommand? _RefreshCommand;
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
					_AddChildEntityNodeCommand?.RaiseCanExecuteChanged();
					_DeleteSelectedCommand?.RaiseCanExecuteChanged();
				}
			}
		}

		public string SelectedNodeSummary => SelectedNode == null
			? "Select a scene object to inspect it."
			: $"{SelectedNode.Name} ({SelectedNode.Kind}) / Children: {SelectedNode.ChildCount}";
		public string SelectedNodePath => SelectedNode == null ? string.Empty : BuildNodePath(SelectedNode.Model);
		public string NodeCountText => $"{CountVisibleNodes()} visible";

		public NoxUI.ViewModelCommand AddRootEntityNodeCommand => _AddRootEntityNodeCommand ??= new(AddRootEntityNode);
		public NoxUI.ViewModelCommand AddChildEntityNodeCommand => _AddChildEntityNodeCommand ??= new(AddChildEntityNode, CanAddChildEntityNode);
		public NoxUI.ViewModelCommand DeleteSelectedCommand => _DeleteSelectedCommand ??= new(DeleteSelected, CanDeleteSelected);
		public NoxUI.ViewModelCommand RefreshCommand => _RefreshCommand ??= new(Refresh);
		#endregion

		public HierarchyViewModel()
		{
			Core.Workspace workspace = Core.StudioManager.Instance.Workspace;
			_SceneHierarchy = workspace.SceneHierarchy;
			_Selection = workspace.Selection;
			_SceneHierarchy.Changed += OnSceneHierarchyChanged;
			Refresh();
		}

		#region 非公開メソッド
		private void AddRootEntityNode()
		{
			SelectedNode = new HierarchyNodeViewModel(_SceneHierarchy.AddEntityNode(CreateUniqueName("EntityNode")));
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

		private bool CanDeleteSelected()
		{
			return SelectedNode != null && SelectedNode.Model.Kind != Core.SceneHierarchyNodeKind.Scene;
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
				SelectedNode = parent == null ? null : new HierarchyNodeViewModel(parent);
			}
		}

		private void Refresh()
		{
			RootNodes.Clear();
			foreach (Core.SceneHierarchyNode node in _SceneHierarchy.RootNodes.Where(node => HierarchyNodeViewModel.MatchesFilter(node, SearchKeyword)))
			{
				RootNodes.Add(new HierarchyNodeViewModel(node, SearchKeyword));
			}

			RaisePropertyChanged(nameof(NodeCountText));
		}

		private void OnSceneHierarchyChanged(object? sender, EventArgs e)
		{
			System.Windows.Threading.Dispatcher? dispatcher = System.Windows.Application.Current?.Dispatcher;
			if (dispatcher != null && dispatcher.CheckAccess() == false)
			{
				dispatcher.BeginInvoke((Action)Refresh);
				return;
			}

			Refresh();
		}

		private string CreateUniqueName(string baseName)
		{
			int existingCount = _SceneHierarchy.RootNodes.Sum(CountNodes);
			return $"{baseName} {existingCount + 1}";
		}

		private static int CountNodes(Core.SceneHierarchyNode node)
		{
			return 1 + node.Children.Sum(CountNodes);
		}

		private int CountVisibleNodes()
		{
			return RootNodes.Sum(CountNodeViewModels);
		}

		private static int CountNodeViewModels(HierarchyNodeViewModel node)
		{
			return 1 + node.Children.Sum(CountNodeViewModels);
		}

		private static string BuildNodePath(Core.SceneHierarchyNode node)
		{
			if (node.Parent == null)
			{
				return node.Name;
			}

			return $"{BuildNodePath(node.Parent)} / {node.Name}";
		}
		#endregion
	}
}
