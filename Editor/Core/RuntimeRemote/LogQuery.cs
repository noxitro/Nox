using System;
using System.Collections.Generic;
using System.Text;

namespace Core.RuntimeRemote;

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

        [Core.RuntimeRemote.Attributes.StringView]
	public string Channel { get; set; } = string.Empty;

        public override Core.RuntimeRemote.Response? Execute()
	{
		string message = string.IsNullOrEmpty(CallStack) ? Msg : $"{Msg}{Environment.NewLine}{CallStack}";
		Core.StudioManager.Instance.Workspace.LogService.Append(GetLevelName(Level), "Runtime", message, NormalizeRuntimeChannel(Channel));
		return null;
	}

	private static string NormalizeRuntimeChannel(string channel)
	{
		return string.IsNullOrWhiteSpace(channel) ? "runtime.default" : $"runtime.{channel}";
	}

	private static string GetLevelName(LogLevel level)
	{
		return level switch
		{
			LogLevel.Info => "Info",
			LogLevel.Warn => "Warning",
			LogLevel.Error => "Error",
			LogLevel.Fatal => "Fatal",
			_ => "Unknown",
		};
	}
}
