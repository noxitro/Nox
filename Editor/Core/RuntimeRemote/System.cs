using System;
using System.Collections.Generic;
using System.Text;

namespace Core.RuntimeRemote
{
	[Core.RuntimeRemote.Attr.RuntimeRemoteCode("core/dev/remote/system", comment: "リソースコンバートリクエスト")]
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

		[Core.RuntimeRemote.Attr.FixedString(NativePathSize)]
		public string NativePath { get; set; } = string.Empty;
	}

	[Core.RuntimeRemote.Attr.RuntimeRemoteCode("core/dev/remote/system", comment: "MainSceneViewを取得する")]
	public class GetMainSceneView : Core.RuntimeRemote.Query
	{
		
	}

	[Core.RuntimeRemote.Attr.RuntimeRemoteCode("core/dev/remote/system", comment: "SceneView情報")]
	public class SceneViewInfo : Core.RuntimeRemote.Response
	{
		public long MainWindowHandle { get; set; } = 0;
		public Core.RuntimeWrapper.SceneView? SceneView { get; set; } = null;
	}
}
