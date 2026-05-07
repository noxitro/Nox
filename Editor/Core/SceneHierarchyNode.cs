using System;
using System.Collections.Generic;

namespace Core
{
	public enum SceneHierarchyNodeKind : byte
	{
		Scene,
		EntityNode,
		Camera,
		Light,
		Folder,
	}

	public sealed class SceneHierarchyNode
	{
		#region 非公開フィールド
		private readonly List<SceneHierarchyNode> _Children = new();
		#endregion

		#region 公開プロパティ
		public Guid Id { get; } = Guid.NewGuid();
		public string Name { get; set; }
		public SceneHierarchyNodeKind Kind { get; set; }
		public SceneHierarchyNode? Parent { get; private set; }
		public IReadOnlyList<SceneHierarchyNode> Children => _Children;
		#endregion

		public SceneHierarchyNode(string name, SceneHierarchyNodeKind kind)
		{
			Name = name;
			Kind = kind;
		}

		public SceneHierarchyNode AddChild(SceneHierarchyNode child)
		{
			child.Parent = this;
			_Children.Add(child);
			return child;
		}

		public bool RemoveFromParent()
		{
			if (Parent == null)
			{
				return false;
			}

			return Parent.RemoveChild(this);
		}

		public bool RemoveChild(SceneHierarchyNode child)
		{
			if (_Children.Remove(child) == false)
			{
				return false;
			}

			child.Parent = null;
			return true;
		}
	}
}
