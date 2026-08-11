using System;

namespace Core;

	public enum SelectionKind : byte
	{
		None,
		HierarchyNode,
		Asset,
	}

	public readonly record struct SelectionInfo(
		SelectionKind Kind,
		object? Value,
		string DisplayName,
		string Description);

	public sealed class SelectionService
	{
		#region 非公開フィールド
		private EventHandler? _Changed;
		#endregion

		#region 公開プロパティ
		public SelectionInfo Current { get; private set; } = new(SelectionKind.None, null, "Nothing selected", string.Empty);
		public event EventHandler? Changed
		{
			add => _Changed += value;
			remove => _Changed -= value;
		}
		#endregion

		public void SelectHierarchyNode(SceneHierarchyNode? node)
		{
			if (node == null)
			{
				Clear();
				return;
			}

			Current = new SelectionInfo(
				SelectionKind.HierarchyNode,
				node,
				node.Name,
				node.Kind.ToString());
			_Changed?.Invoke(this, EventArgs.Empty);
		}

		public void SelectAsset(ProjectAsset? asset)
		{
			if (asset == null)
			{
				Clear();
				return;
			}

			Current = new SelectionInfo(
				SelectionKind.Asset,
				asset,
				asset.Name,
				asset.RelativePath);
			_Changed?.Invoke(this, EventArgs.Empty);
		}

		public void Clear()
		{
			Current = new SelectionInfo(SelectionKind.None, null, "Nothing selected", string.Empty);
			_Changed?.Invoke(this, EventArgs.Empty);
		}
	}
