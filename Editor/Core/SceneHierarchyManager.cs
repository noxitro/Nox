using System;
using System.Collections.Generic;

namespace Core
{
   public sealed class SceneHierarchyManager : EngineSystem
	{
		#region 公開フィールド
		public static readonly SystemPhaseInit<SceneHierarchyManager> InitPhase = new(nameof(InitializeDefaultScene), static engineSystem => engineSystem.InitializeDefaultScene());
        public static readonly SystemPhaseTerminate<SceneHierarchyManager> TerminatePhase = new(nameof(Clear), static engineSystem => engineSystem.Clear());
		#endregion

		#region 非公開フィールド
		private readonly List<SceneHierarchyNode> _RootNodeList = new();
		private EventHandler? _Changed;
		#endregion

		#region 公開プロパティ
		public IReadOnlyList<SceneHierarchyNode> RootNodes => _RootNodeList;
		public event EventHandler? Changed
		{
			add => _Changed += value;
			remove => _Changed -= value;
		}
		#endregion

		public override PhaseRegister[] GetPhaseRegisterList()
		{
			return
			[
                PhaseRegister.Create(InitPhase, this, [AssetManager.InitPhase]),
				PhaseRegister.Create(TerminatePhase, this),
			];
		}

		public SceneHierarchyNode AddEntityNode(string name, SceneHierarchyNode? parent = null)
		{
			SceneHierarchyNode node = new(name, SceneHierarchyNodeKind.EntityNode);
			if (parent == null)
			{
				_RootNodeList.Add(node);
			}
			else
			{
				parent.AddChild(node);
			}

			_Changed?.Invoke(this, EventArgs.Empty);
			return node;
		}

		public bool Remove(SceneHierarchyNode node)
		{
			bool removed;
			if (node.Parent == null)
			{
				removed = _RootNodeList.Remove(node);
			}
			else
			{
				removed = node.RemoveFromParent();
			}

			if (removed)
			{
				_Changed?.Invoke(this, EventArgs.Empty);
			}

			return removed;
		}

		public void InitializeDefaultScene()
		{
			if (_RootNodeList.Count > 0)
			{
				return;
			}

			SceneHierarchyNode scene = new("Main Scene", SceneHierarchyNodeKind.Scene);
			scene.AddChild(new SceneHierarchyNode("Main Camera", SceneHierarchyNodeKind.Camera));
			scene.AddChild(new SceneHierarchyNode("Directional Light", SceneHierarchyNodeKind.Light));
			_RootNodeList.Add(scene);
			_Changed?.Invoke(this, EventArgs.Empty);
		}

		public void Clear()
		{
			_RootNodeList.Clear();
			_Changed?.Invoke(this, EventArgs.Empty);
		}

	}
}
