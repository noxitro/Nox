using System.Collections.ObjectModel;

namespace Core.UI.ViewModels
{
	public sealed class InspectorPropertyViewModel : NoxUI.ViewModelBase
	{
		#region 公開プロパティ
		public string Name
		{
			get => field;
			set => SetProperty(ref field, value);
		} = string.Empty;

		public string Value
		{
			get => field;
			set => SetProperty(ref field, value);
		} = string.Empty;
		#endregion
	}

	public sealed class InspectorViewModel : NoxUI.ViewModelBase
	{
		#region 非公開フィールド
		private readonly Core.SelectionService _Selection;
		#endregion

		#region 公開プロパティ
		public ObservableCollection<InspectorPropertyViewModel> Properties { get; } = new();

		public string Title
		{
			get => field;
			set => SetProperty(ref field, value);
		} = "Nothing selected";

		public string Description
		{
			get => field;
			set => SetProperty(ref field, value);
		} = "Select a hierarchy object or asset.";

		public string Icon
		{
			get => field;
			set => SetProperty(ref field, value);
		} = "";
		#endregion

		public InspectorViewModel()
		{
			_Selection = Core.StudioManager.Instance.Workspace.Selection;
			_Selection.Changed += OnSelectionChanged;
			Refresh(_Selection.Current);
		}

		#region 非公開メソッド
		private void OnSelectionChanged(object? sender, System.EventArgs e)
		{
			System.Windows.Threading.Dispatcher? dispatcher = System.Windows.Application.Current?.Dispatcher;
			if (dispatcher != null && dispatcher.CheckAccess() == false)
			{
				dispatcher.BeginInvoke((System.Action)(() => Refresh(_Selection.Current)));
				return;
			}

			Refresh(_Selection.Current);
		}

		private void Refresh(Core.SelectionInfo selection)
		{
			Properties.Clear();
			Title = selection.DisplayName;
			Description = selection.Description;

			switch (selection.Value)
			{
				case Core.SceneHierarchyNode node:
					Icon = GetHierarchyIcon(node.Kind);
					AddProperty("Type", "Hierarchy Node");
					AddProperty("Kind", node.Kind.ToString());
					AddProperty("Path", BuildNodePath(node));
					AddProperty("Children", node.Children.Count.ToString());
					break;
				case Core.ProjectAsset asset:
					Icon = GetAssetIcon(asset.Kind);
					AddProperty("Type", "Project Asset");
					AddProperty("GUID", asset.Guid);
					AddProperty("Kind", asset.Kind.ToString());
					AddProperty("Path", asset.RelativePath);
					AddProperty("Meta", asset.MetaPath);
					AddProperty("Extension", string.IsNullOrWhiteSpace(asset.Extension) ? "-" : asset.Extension);
					AddProperty("Size", FormatSize(asset.Size));
					AddProperty("Modified", asset.LastWriteTime.ToString("yyyy/MM/dd HH:mm:ss"));
					AddProperty("URI", asset.Uri.ToString());
					break;
				default:
					Icon = "";
					AddProperty("Type", "None");
					AddProperty("Hint", "Select an item in Hierarchy or Asset Browser.");
					break;
			}
		}

		private void AddProperty(string name, string value)
		{
			Properties.Add(new InspectorPropertyViewModel
			{
				Name = name,
				Value = value,
			});
		}

		private static string BuildNodePath(Core.SceneHierarchyNode node)
		{
			return node.Parent == null ? node.Name : $"{BuildNodePath(node.Parent)} / {node.Name}";
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

		private static string GetHierarchyIcon(Core.SceneHierarchyNodeKind kind)
		{
			return kind switch
			{
				Core.SceneHierarchyNodeKind.Scene => "",
				Core.SceneHierarchyNodeKind.Camera => "",
				Core.SceneHierarchyNodeKind.Light => "",
				Core.SceneHierarchyNodeKind.Folder => "",
				_ => "",
			};
		}

		private static string GetAssetIcon(Core.AssetKind kind)
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
				Core.AssetKind.Folder => "",
				_ => "",
			};
		}
		#endregion
	}
}
