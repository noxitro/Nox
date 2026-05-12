using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.Json;
using System.Text.Json.Serialization;

namespace Core
{
    public sealed class SceneHierarchyManager : EngineSystem
	{
		private const string DefaultSceneName = "Main Scene";
		private const string SceneFileExtension = ".noxscene";
		private const string TransformRuntimeFqn = "nox::Transform";

		#region 公開フィールド
		public static readonly SystemPhaseInit<SceneHierarchyManager> InitPhase = new(nameof(InitializeDefaultScene), static engineSystem => engineSystem.InitializeDefaultScene());
        public static readonly SystemPhaseTerminate<SceneHierarchyManager> TerminatePhase = new(nameof(Clear), static engineSystem => engineSystem.Clear());
		#endregion

		#region 非公開フィールド
		private readonly List<SceneHierarchyNode> _RootNodeList = new();
		private EventHandler? _Changed;
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

			SyncRuntimeObject(node);
			EnsureDefaultComponents(node);
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
					SendDestroyEntityNodeQueryRecursive(node);
				}

				UnregisterRemoteObjectRecursive(node);
				_Changed?.Invoke(this, EventArgs.Empty);
			}

			return removed;
		}

		public bool RemoveByRemoteInstanceId(long remoteInstanceId, bool syncRuntime)
		{
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
				SyncRuntimeObject(node);
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

			SyncRuntimeObject(node);
			SendAddComponentQuery(node, component);
			node.AddComponent(component);
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
			_RootNodeList.Clear();
			_Changed?.Invoke(this, EventArgs.Empty);
		}

		public void SyncRuntimeObjects()
		{
			foreach (SceneHierarchyNode node in SceneNodes)
			{
				SyncRuntimeObjectRecursive(node);
			}
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

		private static void SyncRuntimeObjectRecursive(SceneHierarchyNode node)
		{
			SyncRuntimeObject(node);
			EnsureDefaultComponents(node);
			foreach (SceneHierarchyNode child in node.Children)
			{
				SyncRuntimeObjectRecursive(child);
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

		private static void EnsureDefaultComponents(SceneHierarchyNode node)
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

			SendAddComponentQuery(node, transform);
			node.AddComponent(transform);
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
				Children = node.Children.Select(CreateSceneNodeData).ToArray(),
			};
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
			public required SceneNodeData[] Children { get; init; }
		}

	}
}
