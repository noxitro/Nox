using System;
using System.Collections.Generic;
using System.Text;

namespace Nox
{
	public class LogSystem
	{

	}

	namespace LogId
	{
		public interface ILogId<T> where T : struct, ILogId<T>
		{
			public string Tag => typeof(T).Name;
		}

		public readonly struct Unknown : ILogId<Unknown> 
		{
			public Unknown() { }
			public string Tag { get; } = nameof(Unknown);
		}

		public readonly struct Kernel : ILogId<Kernel>
		{
			public Kernel() { }
			public string Tag { get; } = nameof(Kernel);
		}
	}

	public static class LogTrace
	{
		private enum LogLevel : byte
		{
			Info,
			Warning,
			Error,
		}

		private const string LogLevelInfo = "Info";
		private const string LogLevelWarning = "Warning";
		private const string LogLevelError = "Error";

		#region 公開メソッド
		public static void InfoLine<LogId>(string message, params scoped ReadOnlySpan<object> arg) where LogId : struct, Nox.LogId.ILogId<LogId>
		{
			TraceLine(LogLevel.Info, GetTag<LogId>(), message, arg);
		}

		public static void WarningLine<LogId>(string message, params scoped ReadOnlySpan<object> arg) where LogId : struct, Nox.LogId.ILogId<LogId>
		{
			TraceLine(LogLevel.Warning, GetTag<LogId>(), message, arg);
		}

		public static void ErrorLine<LogId>(string message, params scoped ReadOnlySpan<object> arg) where LogId : struct, Nox.LogId.ILogId<LogId>
		{
			TraceLine(LogLevel.Error, GetTag<LogId>(), message, arg);
		}
		#endregion

		#region 非公開メソッド
		private static void TraceLine(LogLevel logLevel, string tag, string message, params scoped ReadOnlySpan<object> arg)
		{
			string s = string.Format("[{0}][{1}]{2}", GetLogLevelString(logLevel), tag, string.Format(message, arg));

			System.Diagnostics.Trace.WriteLine(s);
		}

		private static string GetLogLevelString(LogLevel logLevel)
		{
			return logLevel switch
				{
				LogLevel.Info => "Info",
				LogLevel.Warning => "Warning",
				LogLevel.Error => "Error",
				_ => "Unknown",
			};
		}

		private static string GetTag<LogId>() where LogId : struct, Nox.LogId.ILogId<LogId>
		{
			return new LogId().Tag;
		}

		#endregion
	}
}
