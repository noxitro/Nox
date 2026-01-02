using System;
using System.Collections.Generic;
using System.Text;

namespace Core.LogId
{
	public readonly struct Runtime : Nox.LogId.ILogId<Runtime>
	{
		public Runtime() { }
		string Nox.LogId.ILogId<Runtime>.Tag { get; } = "Runtime";
	}

	public readonly struct Net : Nox.LogId.ILogId<Net> 
	{
		public Net() { }
		string Nox.LogId.ILogId<Net>.Tag { get; } = "Develop.Net";
	}
}
