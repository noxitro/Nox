using System;
using System.Collections.Generic;
using System.Text;

namespace Nox.Diagnostics;

	public static class Debug
	{
		public static void DebugBreak()
		{
			if (System.Diagnostics.Debugger.IsAttached == true)
			{
				System.Diagnostics.Debugger.Break();
			}
		}

		[System.Diagnostics.Conditional("DEBUG")]
		public static void Assert([System.Diagnostics.CodeAnalysis.DoesNotReturnIf(false)] bool condition, string message, params object[] args)
		{
			if (condition == true)
			{
				return;
			}

			System.Diagnostics.Debug.Assert(condition, string.Format(message, args));
		}
	}
