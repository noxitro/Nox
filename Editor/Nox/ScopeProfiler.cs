using System;
using System.Collections.Generic;
using System.Text;

namespace Nox
{
	public readonly struct ScopeProfiler : System.IDisposable
	{
		private readonly System.Diagnostics.Stopwatch _Stopwatch = new();
		public string Tag { private get; init; } = "Unknown";

		public ScopeProfiler()
		{
			_Stopwatch.Start();
		}

		public ScopeProfiler(string tag) : this()
		{
			Tag = tag;
		}

		void System.IDisposable.Dispose()
		{
			Nox.LogTrace.InfoLine<Nox.LogId.Unknown>( $"[{Tag}]{_Stopwatch.ElapsedMilliseconds.ToString()}ms");
			_Stopwatch.Stop();

		}
	}

	public readonly struct ScopeProfiler<T> : System.IDisposable where T : struct, Nox.LogId.ILogId<T>
	{
		private readonly System.Diagnostics.Stopwatch _Stopwatch = new();
		public string Tag { private get; init; } = "Unknown";

		public ScopeProfiler()
		{
			_Stopwatch.Start();
		}

		public ScopeProfiler(string tag) : this()
		{
			Tag = tag;
		}

		void System.IDisposable.Dispose()
		{
			Nox.LogTrace.InfoLine<T>($"[{Tag}]{_Stopwatch.ElapsedMilliseconds.ToString()}ms"); 
			_Stopwatch.Stop();
		}
	}
}
