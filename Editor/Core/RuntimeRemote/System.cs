using System;
using System.Collections.Generic;
using System.Text;

namespace Core.RuntimeRemote
{
	[Core.RuntimeRemote.RuntimeRemoteCode("core/dev/remote/system")]
	public sealed class ResourceConvertQuery : Core.RuntimeRemote.Query
	{
		public ResourceConvertQuery(ReadOnlySpan<char> path)
		{
			if (path.Length >= Path.Length)
			{
				Nox.Util.Assert(false, "Path length exceeds the maximum allowed length.");
			}
			path.CopyTo(Path);
		}

		[Core.RuntimeRemote.RuntimeRemoteCodeNativeFQN("std::array<nox::char16, 256>")]
		public char[] Path { get; } = new char[256];
	}
}
