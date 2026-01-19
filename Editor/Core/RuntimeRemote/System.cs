using System;
using System.Collections.Generic;
using System.Text;

namespace Core.RuntimeRemote
{
	[Core.RuntimeRemote.Attr.RuntimeRemoteCode("core/dev/remote/system")]
	public sealed class ResourceConvertQuery : Core.RuntimeRemote.Query
	{
		public ResourceConvertQuery(ReadOnlySpan<char> path)
		{
			if (path.Length >= NativePath.Length)
			{
				Nox.Util.Assert(false, "Path length exceeds the maximum allowed length.");
			}
			path.CopyTo(NativePath);
		}

		private const uint NativePathSize = 256;

		[Core.RuntimeRemote.Attr.FixedString(NativePathSize)]
		public char[] NativePath { get; } = new char[NativePathSize];
	}
}
