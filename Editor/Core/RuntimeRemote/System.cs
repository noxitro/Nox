using System;
using System.Collections.Generic;
using System.Text;

namespace Core.RuntimeRemote
{
	[Core.RuntimeRemote.Attributes.CoreRuntimeRemoteCode("system", comment: "リソースコンバートリクエスト")]
	public sealed class ResourceConvertQuery : Core.RuntimeRemote.Query
	{
		public ResourceConvertQuery(string path)
		{
			if (path.Length >= NativePath.Length)
			{
				Nox.Util.Assert(false, "Path length exceeds the maximum allowed length.");
			}
			NativePath = path;
		}

		private const uint NativePathSize = 256;

		[Core.RuntimeRemote.Attributes.FixedString(NativePathSize)]
		public string NativePath { get; set; } = string.Empty;
	}

	[Core.RuntimeRemote.Attributes.CoreRuntimeRemoteCode("system", comment: "MainSceneViewを取得する")]
	public class GetMainSceneView : Core.RuntimeRemote.Query
	{
		
	}

	[Core.RuntimeRemote.Attributes.CoreRuntimeRemoteCode("system", comment: "SceneView情報")]
	public class SceneViewInfo : Core.RuntimeRemote.Response
	{
		public long MainWindowHandle { get; set; } = 0;
		public Core.RuntimeWrapper.SceneView? SceneView { get; set; } = null;
	}

	[Core.RuntimeRemote.Attributes.CoreRuntimeRemoteCode("system", comment: "nox::Objectの同期Query")]
	public class SyncQuery : Core.RuntimeRemote.Query
	{
		public long RemoteInstanceId { get; init; } = 0;
		public string FQN { get; init; } = string.Empty;
		public byte[] PropertyByteBuffer { get; init; } = [];
	}
}
