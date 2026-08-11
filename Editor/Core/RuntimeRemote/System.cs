using System;
using System.Collections.Generic;
using System.Text;

namespace Core.RuntimeRemote;

	[Core.RuntimeRemote.Attributes.CoreRuntimeRemoteCode("system", comment: "リソースコンバートリクエスト")]
	public sealed class AssetConvertQuery : Core.RuntimeRemote.Query
	{
		public AssetConvertQuery(string uri)
		{
			if (uri.Length >= NativePathSize)
			{
				Nox.Util.Assert(false, "URI length exceeds the maximum allowed length.");
			}
			Uri = uri;
		}

		private const uint NativePathSize = 256;

		[Core.RuntimeRemote.Attributes.FixedString(NativePathSize)]
		public string Uri { get; set; } = string.Empty;

		public override Core.RuntimeRemote.Response? Execute()
		{
			try
			{
				Core.Workspace workspace = Core.StudioManager.Instance.Workspace;
				Core.NativeAssetConverter.Convert(workspace, Uri);
			}
			catch (Exception ex)
			{
				Nox.LogTrace.WarningLine<Core.LogId.Runtime>($"Asset convert failed: {Uri}, {ex}");
			}

			// NOTE: 現状の受信ディスパッチ（RuntimeRemoteClient）は Execute() の戻り値を送信しないため、
			//       AssetConvertResponse は返さない。ランタイムのロードスレッドがネイティブファイルの
			//       存在ポーリングで検知するため、書き出しのみで成立する。
			return null;
		}
	}

	public sealed class AssetConvertResponse : Core.RuntimeRemote.Response
{
    public bool Success { get; set; } = false;
    [Core.RuntimeRemote.Attributes.FixedString(512)]
    public string ConvertedUri { get; set; } = string.Empty;
}

[Core.RuntimeRemote.Attributes.CoreRuntimeRemoteCode("system", comment: "MainSceneViewを取得する")]
	public class GetMainSceneView : Core.RuntimeRemote.Query
	{
		
	}

	[Core.RuntimeRemote.Attributes.CoreRuntimeRemoteCode("system", comment: "SceneView情報")]
	public class SceneViewInfo : Core.RuntimeRemote.Response
	{
		public long MainWindowHandle { get; set; } = 0;
	}

	[Core.RuntimeRemote.Attributes.CoreRuntimeRemoteCode("system", comment: "nox::Objectの同期Query")]
	public class SyncQuery : Core.RuntimeRemote.Query
	{
		public long RemoteInstanceId { get; init; } = 0;

		[Core.RuntimeRemote.Attributes.FixedString(512)]
		public string Fqn { get; init; } = string.Empty;

		[Core.RuntimeRemote.Attributes.FixedArray(2048)]
		public byte[] PropertyByteBuffer { get; init; } = [];
	}

	[Core.RuntimeRemote.Attributes.CoreRuntimeRemoteCode("system", comment: "nox::Objectの同期結果")]
	public sealed class SyncResponse : Core.RuntimeRemote.Response
	{
		public long RemoteInstanceId { get; init; } = 0;
		public bool Applied { get; init; } = false;
	}

	[Core.RuntimeRemote.Attributes.CoreRuntimeRemoteCode("system", comment: "Hierarchy EntityNode追加Query")]
	public sealed class AddEntityNodeQuery : Core.RuntimeRemote.Query
	{
		public long RemoteInstanceId { get; init; } = 0;
		public long ParentRemoteInstanceId { get; init; } = 0;

		[Core.RuntimeRemote.Attributes.FixedString(256)]
		public string Name { get; init; } = string.Empty;
	}

	[Core.RuntimeRemote.Attributes.CoreRuntimeRemoteCode("system", comment: "Hierarchy EntityNode追加Response")]
	public sealed class AddEntityNodeResponse : Core.RuntimeRemote.Response
	{
		public long RemoteInstanceId { get; init; } = 0;
		public bool Created { get; init; } = false;
		public bool Attached { get; init; } = false;
	}

	[Core.RuntimeRemote.Attributes.CoreRuntimeRemoteCode("system", comment: "Hierarchy Component追加Query")]
	public sealed class AddComponentQuery : Core.RuntimeRemote.Query
	{
		public long RemoteInstanceId { get; init; } = 0;
		public long EntityNodeRemoteInstanceId { get; init; } = 0;

		[Core.RuntimeRemote.Attributes.FixedString(512)]
		public string ComponentTypeFqn { get; init; } = string.Empty;

		[Core.RuntimeRemote.Attributes.FixedArray(2048)]
		public byte[] PropertyByteBuffer { get; init; } = [];
	}

	[Core.RuntimeRemote.Attributes.CoreRuntimeRemoteCode("system", comment: "Hierarchy Component追加Response")]
	public sealed class AddComponentResponse : Core.RuntimeRemote.Response
	{
		public long RemoteInstanceId { get; init; } = 0;
		public bool Created { get; init; } = false;
		public bool Added { get; init; } = false;
	}

	[Core.RuntimeRemote.Attributes.CoreRuntimeRemoteCode("system", comment: "Hierarchy EntityNode破棄Query")]
	public sealed class DestroyEntityNodeQuery : Core.RuntimeRemote.Query
	{
		public long RemoteInstanceId { get; init; } = 0;
	}

	[Core.RuntimeRemote.Attributes.CoreRuntimeRemoteCode("system", comment: "Inspector Auto-Sync Query")]
	public sealed class AutoSyncQuery : Core.RuntimeRemote.Query
	{
		public long RemoteInstanceId { get; init; } = 0;
	}

	[Core.RuntimeRemote.Attributes.CoreRuntimeRemoteCode("system", comment: "Inspector Auto-Sync Response")]
	public sealed class AutoSyncResponse : Core.RuntimeRemote.Response
	{
		public long RemoteInstanceId { get; init; } = 0;
		public bool Exists { get; init; } = false;

		[Core.RuntimeRemote.Attributes.FixedArray(2048)]
		public byte[] PropertyByteBuffer { get; init; } = [];
	}

	[Core.RuntimeRemote.Attributes.CoreRuntimeRemoteCode("system", comment: "Inspector Action関数実行Query")]
	public sealed class InvokeRuntimeActionQuery : Core.RuntimeRemote.Query
	{
		public long RemoteInstanceId { get; init; } = 0;

		[Core.RuntimeRemote.Attributes.FixedString(512)]
		public string FunctionFullName { get; init; } = string.Empty;
	}

	[Core.RuntimeRemote.Attributes.CoreRuntimeRemoteCode("system", comment: "Inspector Action関数実行Response")]
	public sealed class InvokeRuntimeActionResponse : Core.RuntimeRemote.Response
	{
		public long RemoteInstanceId { get; init; } = 0;
		public bool Invoked { get; init; } = false;
	}

	[Core.RuntimeRemote.Attributes.CoreRuntimeRemoteCode("system", comment: "Runtime依存グラフ取得Query")]
	public sealed class GetRuntimeDependencyGraphQuery : Core.RuntimeRemote.Query
	{
	}

	[Core.RuntimeRemote.Attributes.CoreRuntimeRemoteCode("system", comment: "Runtime依存グラフ取得Response")]
	public sealed class RuntimeDependencyGraphResponse : Core.RuntimeRemote.Response
	{
		[Core.RuntimeRemote.Attributes.FixedString(3072)]
		public string GraphText { get; init; } = string.Empty;
	}

	[Core.RuntimeRemote.Attributes.CoreRuntimeRemoteCode("system", comment: "RemoteInstance管理状態取得Query")]
	public sealed class GetRemoteInstanceSnapshotQuery : Core.RuntimeRemote.Query
	{
	}

	[Core.RuntimeRemote.Attributes.CoreRuntimeRemoteCode("system", comment: "RemoteInstance管理状態取得Response")]
	public sealed class RemoteInstanceSnapshotResponse : Core.RuntimeRemote.Response
	{
		[Core.RuntimeRemote.Attributes.FixedString(3072)]
		public string SnapshotText { get; init; } = string.Empty;
	}

	[Core.RuntimeRemote.Attributes.CoreRuntimeRemoteCode("system", comment: "MemoryProfilerスナップショット取得Query")]
	public sealed class GetMemoryProfilerSnapshotQuery : Core.RuntimeRemote.Query
	{
	}

	[Core.RuntimeRemote.Attributes.CoreRuntimeRemoteCode("system", comment: "MemoryProfilerスナップショット取得Response")]
	public sealed class MemoryProfilerSnapshotResponse : Core.RuntimeRemote.Response
	{
		[Core.RuntimeRemote.Attributes.FixedString(3072)]
		public string SnapshotText { get; init; } = string.Empty;
	}

	[Core.RuntimeRemote.Attributes.CoreRuntimeRemoteCode("system", execute: false, enabledRecv: false, comment: "Runtime側RemoteObject破棄通知Query")]
	public sealed class RuntimeObjectDestroyedQuery : Core.RuntimeRemote.Query
	{
		public long RemoteInstanceId { get; init; } = 0;

		public override Core.RuntimeRemote.Response? Execute()
		{
			Core.StudioManager.Instance.Workspace.SceneHierarchy.RemoveByRemoteInstanceId(RemoteInstanceId, syncRuntime: false);
			return null;
		}
	}

	[Core.RuntimeRemote.Attributes.CoreRuntimeRemoteCode("system", comment: "nox::Objectの同期完了通知Query")]
public class EndSyncQuery : Core.RuntimeRemote.Query
	{
		public long RemoteInstanceId { get; init; } = 0;
}
