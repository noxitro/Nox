// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Threading;

namespace Core;

public sealed class SceneHierarchyManager : EngineSystem
	{
		private const string DefaultSceneName = "Main Scene";
		private const string SceneFileExtension = ".noxscene";
		private const string TransformRuntimeFqn = "nox::LocalTransform";

		#region 公開フィールド
		public static readonly SystemPhaseInit<SceneHierarchyManager> InitPhase = new(nameof(InitializeDefaultScene), static engineSystem => engineSystem.InitializeDefaultScene());
    public static readonly SystemPhaseTerminate<SceneHierarchyManager> TerminatePhase = new(nameof(Clear), static engineSystem => engineSystem.Clear());
		#endregion

		#region 非公開フィールド
		private readonly List<SceneHierarchyNode> _RootNodeList = new();
		private readonly Lock _StructuralChangeLock = new();
		private readonly List<StructuralChange> _StructuralChanges = new();
		private readonly SynchronizationContext? _OwnerSynchronizationContext = SynchronizationContext.Current;
		private readonly int _OwnerThreadId = Environment.CurrentManagedThreadId;
		private EventHandler? _Changed;
		private int _StructuralChangeBatchDepth;
		private bool _IsFlushingStructuralChanges;
		#endregion

		#region 公開プロパティ
		public SceneHierarchyNode SceneRoot => EnsureSceneRoot();
		public IReadOnlyList<SceneHierarchyNode> RootNodes => _RootNodeList;
		public IReadOnlyList<SceneHierarchyNode> SceneNodes => SceneRoot.Children;
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
			return AddNode(node, parent);
		}

		public SceneHierarchyNode AddGroupNode(string name, SceneHierarchyNode? parent = null)
		{
			SceneHierarchyNode node = new(name, SceneHierarchyNodeKind.GroupNode);
			return AddNode(node, parent);
		}

		private SceneHierarchyNode AddNode(SceneHierarchyNode node, SceneHierarchyNode? parent)
		{
			parent ??= SceneRoot;
			if (parent == null)
			{
				_RootNodeList.Add(node);
			}
			else
			{
				parent.AddChild(node);
			}

			QueueSyncRuntimeObject(node);
			EnsureDefaultComponents(node);
			FlushStructuralChangesIfReady();
			_Changed?.Invoke(this, EventArgs.Empty);
			return node;
		}

		public bool Remove(SceneHierarchyNode node, bool syncRuntime = true)
		{
			if (node.Kind == SceneHierarchyNodeKind.SceneNode)
			{
				return false;
			}

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
				if (syncRuntime)
				{
					QueueStructuralChange(StructuralChange.Destroy(node));
				}

				QueueStructuralChange(StructuralChange.Unregister(node));
				FlushStructuralChangesIfReady();
				_Changed?.Invoke(this, EventArgs.Empty);
			}

			return removed;
		}

		public bool RemoveByRemoteInstanceId(long remoteInstanceId, bool syncRuntime)
		{
			if (TryPostRemoveByRemoteInstanceId(remoteInstanceId, syncRuntime, out bool removed))
			{
				return removed;
			}

			if (remoteInstanceId == 0)
			{
				return false;
			}

			SceneHierarchyNode? node = SceneRoot.FindByRemoteInstanceId(remoteInstanceId);
			return node != null && Remove(node, syncRuntime);
		}

		public void Rename(SceneHierarchyNode node, string name)
		{
			if (node.Name == name)
			{
				return;
			}

			node.Name = name;
			if (node.Kind != SceneHierarchyNodeKind.SceneNode)
			{
				QueueSyncRuntimeObject(node);
				FlushStructuralChangesIfReady();
			}
			_Changed?.Invoke(this, EventArgs.Empty);
		}

		public RuntimeObject? AddComponent(SceneHierarchyNode node, string componentRuntimeFqn)
		{
			if (node.Kind == SceneHierarchyNodeKind.SceneNode || string.IsNullOrWhiteSpace(componentRuntimeFqn))
			{
				return null;
			}

			RuntimeObject? existingComponent = node.FindComponent(componentRuntimeFqn);
			if (existingComponent != null)
			{
				return existingComponent;
			}

			RuntimeObject? component = StudioManager.Instance.GetEngineSystem<Runtime>().CreateRuntimeObject(componentRuntimeFqn);
			if (component == null)
			{
				Nox.LogTrace.WarningLine<LogId.RuntimeRemote>("Runtime wrapper was not found for component. fqn:{0}", componentRuntimeFqn);
				return null;
			}

			node.AddComponent(component);
			QueueSyncRuntimeObject(node);
			QueueStructuralChange(StructuralChange.AddComponent(node, component));
			FlushStructuralChangesIfReady();
			_Changed?.Invoke(this, EventArgs.Empty);
			return component;
		}

		public string SaveScene(string assetRootPath)
		{
			Nox.Util.Assert(string.IsNullOrWhiteSpace(assetRootPath) == false, "AssetRootPath is empty.");

			Directory.CreateDirectory(assetRootPath);
			SceneHierarchyNode scene = SceneRoot;
			string filePath = Path.Combine(assetRootPath, CreateSceneFileName(scene.Name));
			SaveSceneToFile(filePath, scene);
			return filePath;
		}

		public void SaveSceneAs(string filePath)
		{
			Nox.Util.Assert(string.IsNullOrWhiteSpace(filePath) == false, "Scene file path is empty.");
			SaveSceneToFile(filePath, SceneRoot);
		}

		public void InitializeDefaultScene()
		{
			EnsureSceneRoot();
			SyncRuntimeObjects();
			_Changed?.Invoke(this, EventArgs.Empty);
		}

		public void NewScene()
		{
			_RootNodeList.Clear();
			EnsureSceneRoot();
			SyncRuntimeObjects();
			_Changed?.Invoke(this, EventArgs.Empty);
		}

		public void Clear()
		{
			lock (_StructuralChangeLock)
			{
				if (_StructuralChangeBatchDepth != 0)
				{
					throw new InvalidOperationException("Cannot clear a scene while a structural change batch is active.");
				}

				_StructuralChanges.Clear();
			}

			_RootNodeList.Clear();
			_Changed?.Invoke(this, EventArgs.Empty);
		}

		public void SyncRuntimeObjects()
		{
			if (TryPostToOwnerContext(SyncRuntimeObjects))
			{
				return;
			}

			using StructuralChangeBatch _ = BeginStructuralChangeBatch();
			foreach (SceneHierarchyNode node in SceneNodes)
			{
				QueueSyncRuntimeObjectRecursive(node);
			}
		}

		/// <summary>
		/// Groups structural ECS changes so their remote representation is emitted at one safe point.
		/// Nested batches flush only when the outermost scope completes.
		/// </summary>
		public StructuralChangeBatch BeginStructuralChangeBatch()
		{
			lock (_StructuralChangeLock)
			{
				++_StructuralChangeBatchDepth;
			}
			return new StructuralChangeBatch(this);
		}

		private SceneHierarchyNode EnsureSceneRoot()
		{
			SceneHierarchyNode? scene = _RootNodeList.FirstOrDefault(static node => node.Kind == SceneHierarchyNodeKind.SceneNode);
			bool shouldAddDefaultChildren = scene == null && _RootNodeList.Count == 0;
			scene ??= new SceneHierarchyNode(DefaultSceneName, SceneHierarchyNodeKind.SceneNode);

			if (shouldAddDefaultChildren)
			{
				scene.AddChild(new SceneHierarchyNode("Main Camera", SceneHierarchyNodeKind.EntityNode));
				scene.AddChild(new SceneHierarchyNode("Directional Light", SceneHierarchyNodeKind.EntityNode));
			}

			if (_RootNodeList.Count == 1 && ReferenceEquals(_RootNodeList[0], scene))
			{
				return scene;
			}

			SceneHierarchyNode[] movedRoots = _RootNodeList
				.Where(node => ReferenceEquals(node, scene) == false)
				.ToArray();
			_RootNodeList.Clear();
			_RootNodeList.Add(scene);

			foreach (SceneHierarchyNode movedRoot in movedRoots)
			{
				scene.AddChild(movedRoot);
			}

			return scene;
		}

		private void QueueSyncRuntimeObjectRecursive(SceneHierarchyNode node)
		{
			QueueSyncRuntimeObject(node);
			EnsureDefaultComponents(node);
			foreach (SceneHierarchyNode child in node.Children)
			{
				QueueSyncRuntimeObjectRecursive(child);
			}
		}

		private void QueueSyncRuntimeObject(SceneHierarchyNode node)
		{
			if (node.Kind != SceneHierarchyNodeKind.SceneNode)
			{
				QueueStructuralChange(StructuralChange.Sync(node));
			}
		}

		private static void SyncRuntimeObject(SceneHierarchyNode node)
		{
			if (node.Parent is { Kind: not SceneHierarchyNodeKind.SceneNode })
			{
				SyncRuntimeObject(node.Parent);
			}

			RuntimeObject? remoteObject = node.EnsureRemoteObject();
			if (remoteObject == null)
			{
				return;
			}

			Core.Net.RuntimeRemoteClient remoteClient = remoteObject.RemoteClient ??
				StudioManager.Instance.Workspace.RuntimeSessions.GetActiveOrMainSession().RemoteClient;
			if (remoteObject.RemoteInstanceId == 0)
			{
				remoteClient.RegisterRemoteObject(remoteObject);
			}

			remoteClient.SendQuery(new Core.RuntimeRemote.AddEntityNodeQuery
			{
				RemoteInstanceId = remoteObject.RemoteInstanceId,
				ParentRemoteInstanceId = node.Parent?.RemoteInstanceId ?? 0,
				Name = node.Name,
			}, response =>
			{
				Core.RuntimeRemote.AddEntityNodeResponse addResponse = Nox.Util.Cast<Core.RuntimeRemote.AddEntityNodeResponse>(response);
				if (addResponse.Attached == false)
				{
					Nox.LogTrace.WarningLine<Core.LogId.RuntimeRemote>("Runtime EntityNode attach failed. id:{0}", addResponse.RemoteInstanceId);
				}
			});
			remoteObject.Sync(remoteClient, Core.Net.SyncMode.TwoWay);
		}

		private void EnsureDefaultComponents(SceneHierarchyNode node)
		{
			if (node.Kind == SceneHierarchyNodeKind.SceneNode || node.HasComponent(TransformRuntimeFqn))
			{
				return;
			}

			RuntimeObject? transform = StudioManager.Instance.GetEngineSystem<Runtime>().CreateRuntimeObject(TransformRuntimeFqn);
			if (transform == null)
			{
				Nox.LogTrace.WarningLine<Core.LogId.RuntimeRemote>("Runtime wrapper was not found for {0}.", TransformRuntimeFqn);
				return;
			}

			node.AddComponent(transform);
			QueueStructuralChange(StructuralChange.AddComponent(node, transform));
		}

		private static void SendAddComponentQuery(SceneHierarchyNode node, RuntimeObject component)
		{
			RuntimeObject? nodeRemoteObject = node.EnsureRemoteObject();
			if (nodeRemoteObject == null)
			{
				return;
			}

			Core.Net.RuntimeRemoteClient remoteClient = nodeRemoteObject.RemoteClient ??
				StudioManager.Instance.Workspace.RuntimeSessions.GetActiveOrMainSession().RemoteClient;
			if (nodeRemoteObject.RemoteInstanceId == 0)
			{
				remoteClient.RegisterRemoteObject(nodeRemoteObject);
			}
			if (component.RemoteInstanceId == 0)
			{
				remoteClient.RegisterRemoteObject(component);
			}

			Span<byte> propertyBuffer = stackalloc byte[2048];
			ReadOnlySpan<byte> propertyBytes = Core.RuntimeRemote.Util.GetPropertiesBytes(propertyBuffer, component);
			remoteClient.SendQuery(new Core.RuntimeRemote.AddComponentQuery
			{
				RemoteInstanceId = component.RemoteInstanceId,
				EntityNodeRemoteInstanceId = nodeRemoteObject.RemoteInstanceId,
				ComponentTypeFqn = component.RuntimeFqn,
				PropertyByteBuffer = propertyBytes.ToArray(),
			}, response =>
			{
				Core.RuntimeRemote.AddComponentResponse addResponse = Nox.Util.Cast<Core.RuntimeRemote.AddComponentResponse>(response);
				if (addResponse.Added == false)
				{
					Nox.LogTrace.WarningLine<Core.LogId.RuntimeRemote>("Runtime Component attach failed. entity:{0}, component:{1}, type:{2}",
						nodeRemoteObject.RemoteInstanceId,
						addResponse.RemoteInstanceId,
						component.RuntimeFqn);
				}
			});
		}

		private static void SendDestroyEntityNodeQuery(long remoteInstanceId)
		{
			Core.Net.RuntimeRemoteClient remoteClient =
				StudioManager.Instance.Workspace.RuntimeSessions.GetActiveOrMainSession().RemoteClient;
			remoteClient.SendQuery(new Core.RuntimeRemote.DestroyEntityNodeQuery
			{
				RemoteInstanceId = remoteInstanceId,
			}, null);
		}

		private static void SendDestroyEntityNodeQueryRecursive(SceneHierarchyNode node)
		{
			foreach (SceneHierarchyNode child in node.Children)
			{
				SendDestroyEntityNodeQueryRecursive(child);
			}

			if (node.RemoteInstanceId != 0)
			{
				SendDestroyEntityNodeQuery(node.RemoteInstanceId);
			}
		}

		private static void UnregisterRemoteObjectRecursive(SceneHierarchyNode node)
		{
			if (node.RemoteInstanceId != 0 && node.RemoteObject?.RemoteClient != null)
			{
				node.RemoteObject.RemoteClient.UnregisterRemoteObject(node.RemoteInstanceId);
			}

			foreach (RuntimeObject component in node.Components)
			{
				if (component.RemoteInstanceId != 0 && component.RemoteClient != null)
				{
					component.RemoteClient.UnregisterRemoteObject(component.RemoteInstanceId);
				}
			}

			foreach (SceneHierarchyNode child in node.Children)
			{
				UnregisterRemoteObjectRecursive(child);
			}
		}

		private static void SaveSceneToFile(string filePath, SceneHierarchyNode scene)
		{
			Directory.CreateDirectory(Path.GetDirectoryName(filePath) ?? string.Empty);
			using FileStream stream = File.Create(filePath);
			JsonSerializer.Serialize(stream, new SceneDocument
			{
				Scene = CreateSceneNodeData(scene),
			}, CreateSceneJsonOptions());
		}

		private static SceneNodeData CreateSceneNodeData(SceneHierarchyNode node)
		{
			return new SceneNodeData
			{
				Id = node.Id,
				Name = node.Name,
				Kind = node.Kind,
				ComponentRuntimeFqns = node.Components.Select(static component => component.RuntimeFqn).ToArray(),
				Children = node.Children.Select(CreateSceneNodeData).ToArray(),
			};
		}

		private void QueueStructuralChange(StructuralChange change)
		{
			lock (_StructuralChangeLock)
			{
				_StructuralChanges.Add(change);
			}
		}

		private void FlushStructuralChangesIfReady()
		{
			lock (_StructuralChangeLock)
			{
				if (_StructuralChangeBatchDepth != 0)
				{
					return;
				}
			}

			FlushStructuralChanges();
		}

		private void FlushStructuralChanges()
		{
			lock (_StructuralChangeLock)
			{
				if (_StructuralChangeBatchDepth != 0 || _IsFlushingStructuralChanges)
				{
					return;
				}

				_IsFlushingStructuralChanges = true;
			}

			try
			{
				while (true)
				{
					StructuralChange[] changes;
					lock (_StructuralChangeLock)
					{
						if (_StructuralChanges.Count == 0)
						{
							return;
						}

						changes = _StructuralChanges.ToArray();
						_StructuralChanges.Clear();
					}

					for (int index = 0; index < changes.Length; ++index)
					{
						try
						{
							ExecuteStructuralChange(changes[index]);
						}
						catch (Exception ex)
						{
							Nox.LogTrace.ErrorLine<Core.LogId.RuntimeRemote>(
								"Structural change failed and was discarded. kind:{0} node:{1} error:{2}",
								changes[index].Kind,
								changes[index].Node.Name,
								ex);
						}
					}
				}
			}
			finally
			{
				lock (_StructuralChangeLock)
				{
					_IsFlushingStructuralChanges = false;
				}
			}
		}

		private static void ExecuteStructuralChange(StructuralChange change)
		{
			switch (change.Kind)
			{
				case StructuralChangeKind.Sync:
					SyncRuntimeObject(change.Node);
					break;
				case StructuralChangeKind.AddComponent:
					SendAddComponentQuery(change.Node, change.Component!);
					break;
				case StructuralChangeKind.Destroy:
					SendDestroyEntityNodeQueryRecursive(change.Node);
					break;
				case StructuralChangeKind.Unregister:
					UnregisterRemoteObjectRecursive(change.Node);
					break;
				default:
					throw new InvalidOperationException($"Unknown structural change: {change.Kind}");
			}
		}

		private void EndStructuralChangeBatch()
		{
			lock (_StructuralChangeLock)
			{
				if (_StructuralChangeBatchDepth <= 0)
				{
					throw new InvalidOperationException("Structural change batch underflow.");
				}

				--_StructuralChangeBatchDepth;
			}

			FlushStructuralChangesIfReady();
		}

		private bool TryPostToOwnerContext(Action callback)
		{
			if (Environment.CurrentManagedThreadId == _OwnerThreadId)
			{
				return false;
			}

			if (_OwnerSynchronizationContext == null)
			{
				throw new InvalidOperationException("Scene hierarchy was created without an owner synchronization context.");
			}

			_OwnerSynchronizationContext.Post(static state => ((Action)state!).Invoke(), callback);
			return true;
		}

		private bool TryPostRemoveByRemoteInstanceId(long remoteInstanceId, bool syncRuntime, out bool result)
		{
			if (Environment.CurrentManagedThreadId == _OwnerThreadId)
			{
				result = false;
				return false;
			}

			if (_OwnerSynchronizationContext == null)
			{
				throw new InvalidOperationException("Scene hierarchy was created without an owner synchronization context.");
			}

			_OwnerSynchronizationContext.Post(static state =>
			{
				(SceneHierarchyManager owner, long remoteInstanceId, bool syncRuntime) = ((SceneHierarchyManager, long, bool))state!;
				owner.RemoveByRemoteInstanceId(remoteInstanceId, syncRuntime);
			}, (this, remoteInstanceId, syncRuntime));
			result = true;
			return true;
		}

		public sealed class StructuralChangeBatch : IDisposable
		{
			private SceneHierarchyManager? _Owner;

			internal StructuralChangeBatch(SceneHierarchyManager owner)
			{
				_Owner = owner;
			}

			public void Dispose()
			{
				SceneHierarchyManager? owner = Interlocked.Exchange(ref _Owner, null);
				owner?.EndStructuralChangeBatch();
			}
		}

		private enum StructuralChangeKind : byte
		{
			Sync,
			AddComponent,
			Destroy,
			Unregister,
		}

		private readonly record struct StructuralChange(
			StructuralChangeKind Kind,
			SceneHierarchyNode Node,
			RuntimeObject? Component)
		{
			public static StructuralChange Sync(SceneHierarchyNode node) => new(StructuralChangeKind.Sync, node, null);
			public static StructuralChange AddComponent(SceneHierarchyNode node, RuntimeObject component) => new(StructuralChangeKind.AddComponent, node, component);
			public static StructuralChange Destroy(SceneHierarchyNode node) => new(StructuralChangeKind.Destroy, node, null);
			public static StructuralChange Unregister(SceneHierarchyNode node) => new(StructuralChangeKind.Unregister, node, null);
		}

		private static JsonSerializerOptions CreateSceneJsonOptions()
		{
			JsonSerializerOptions options = new()
			{
				WriteIndented = true,
			};
			options.Converters.Add(new JsonStringEnumConverter());
			return options;
		}

		private static string CreateSceneFileName(string sceneName)
		{
			char[] invalidChars = Path.GetInvalidFileNameChars();
			string sanitizedName = new(sceneName.Select(ch => invalidChars.Contains(ch) ? '_' : ch).ToArray());
			if (string.IsNullOrWhiteSpace(sanitizedName))
			{
				sanitizedName = DefaultSceneName;
			}

			return sanitizedName + SceneFileExtension;
		}

		private sealed class SceneDocument
		{
			public int Version { get; init; } = 1;
			public required SceneNodeData Scene { get; init; }
		}

		private sealed class SceneNodeData
		{
			public Guid Id { get; init; }
			public required string Name { get; init; }
			public SceneHierarchyNodeKind Kind { get; init; }
			public required string[] ComponentRuntimeFqns { get; init; }
			public required SceneNodeData[] Children { get; init; }
		}

	}
