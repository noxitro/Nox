using System;
using System.Collections.Generic;

namespace Core;

	public enum SceneHierarchyNodeKind : byte
	{
		SceneNode,
		EntityNode,
		GroupNode,
	}

	public sealed class SceneHierarchyNode
	{
		#region 非公開フィールド
		private readonly List<SceneHierarchyNode> _Children = new();
		private readonly List<RuntimeObject> _Components = new();
		#endregion

		#region 公開プロパティ
		public Guid Id { get; } = Guid.NewGuid();
		public string Name { get; set; }
		public SceneHierarchyNodeKind Kind { get; set; }
		public SceneHierarchyNode? Parent { get; private set; }
		public IReadOnlyList<SceneHierarchyNode> Children => _Children;
		public IReadOnlyList<RuntimeObject> Components => _Components;
		public RuntimeObject? RemoteObject { get; private set; }
		public long RemoteInstanceId => RemoteObject?.RemoteInstanceId ?? 0;
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

		public SceneHierarchyNode? FindByRemoteInstanceId(long remoteInstanceId)
		{
			if (remoteInstanceId != 0 && RemoteInstanceId == remoteInstanceId)
			{
				return this;
			}

			foreach (SceneHierarchyNode child in _Children)
			{
				SceneHierarchyNode? found = child.FindByRemoteInstanceId(remoteInstanceId);
				if (found != null)
				{
					return found;
				}
			}

			return null;
		}

		internal RuntimeObject? EnsureRemoteObject()
		{
			if (Kind == SceneHierarchyNodeKind.SceneNode)
			{
				return null;
			}

			if (RemoteObject != null)
			{
				return RemoteObject;
			}

			RuntimeObject? remoteObject = StudioManager.Instance.GetEngineSystem<Runtime>().CreateRuntimeObject("nox::EntityNode");
			if (remoteObject == null)
			{
				Nox.LogTrace.WarningLine<LogId.RuntimeRemote>("Runtime wrapper was not found for nox::EntityNode.");
				return null;
			}

			RemoteObject = remoteObject;
			return remoteObject;
		}

		public bool HasComponent(ReadOnlySpan<char> runtimeFqn)
		{
			foreach (RuntimeObject component in _Components)
			{
				if (component.RuntimeFqn.AsSpan().SequenceEqual(runtimeFqn))
				{
					return true;
				}
			}

			return false;
		}

		internal RuntimeObject? FindComponent(ReadOnlySpan<char> runtimeFqn)
		{
			foreach (RuntimeObject component in _Components)
			{
				if (component.RuntimeFqn.AsSpan().SequenceEqual(runtimeFqn))
				{
					return component;
				}
			}

			return null;
		}

		internal void AddComponent(RuntimeObject component)
		{
			if (_Components.Contains(component))
			{
				return;
			}

			_Components.Add(component);
		}
	}
