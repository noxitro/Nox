using System;
using System.Collections.Generic;
using System.Text;

namespace Core.RuntimeRemote
{
	[Core.RuntimeRemote.Attributes.CoreRuntimeRemoteCode("log", comment: "ログ送信", enabledRecv:false)]
	public class SendLog : Query
	{
		public enum LogLevel : byte
		{
			Info,
			Warn, 
			Error, 
			Fatal
		}

		public LogLevel Level { get; set; }

		[Core.RuntimeRemote.Attributes.StringView]
		public string Msg { get; set; } = string.Empty;

		[Core.RuntimeRemote.Attributes.StringView]
		public string CallStack { get; set; } = string.Empty;
	}
}
