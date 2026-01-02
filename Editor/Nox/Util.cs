using System;
using System.Collections.Generic;
using System.Text;

namespace Nox
{
	public static partial class Util
	{
		[System.Diagnostics.Conditional("DEBUG")]
		public static void Assert([System.Diagnostics.CodeAnalysis.DoesNotReturnIf(false)] bool condition, string message, params object[] args)
		{
			if(condition==true)
			{
				return;
			}

			System.Diagnostics.Debug.Assert(condition, string.Format(message, args));
		}
	}
}
