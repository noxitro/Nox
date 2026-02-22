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
		[System.Runtime.CompilerServices.InterpolatedStringHandler]
		public ref struct LogInterpolatedStringHandler
		{
			private System.Runtime.CompilerServices.DefaultInterpolatedStringHandler _handler;

			public LogInterpolatedStringHandler(int literalLength, int formattedCount)
			{
				_handler = new System.Runtime.CompilerServices.DefaultInterpolatedStringHandler(literalLength, formattedCount);
			}

			public void AppendLiteral(string s) => _handler.AppendLiteral(s);

			public void AppendFormatted<T>(T value) => _handler.AppendFormatted(value);

			public void AppendFormatted(ReadOnlySpan<char> value) => _handler.AppendFormatted(value);

			public void AppendFormatted<T>(T value, string? format) => _handler.AppendFormatted(value, format);

			public void AppendFormatted(ReadOnlySpan<char> value, int alignment = 0, string? format = null)
				=> _handler.AppendFormatted(value, alignment, format);

			internal string ToStringAndClear() => _handler.ToStringAndClear();
		}

		private enum LogLevel : byte
		{
			Info,
			Warning,
			Error,
		}

		private const string LogLevelInfo = "Info";
		private const string LogLevelWarning = "Warning";
		private const string LogLevelError = "Error";

		#region 非公開フィールド
		private static readonly System.Threading.Channels.Channel<string> _Channel = System.Threading.Channels.Channel.CreateUnbounded<string>(
	new System.Threading.Channels.UnboundedChannelOptions { SingleReader = true });

		#endregion

		#region 公開メソッド
		public static void InfoLine<LogId>(string message, params scoped ReadOnlySpan<object> arg) where LogId : struct, Nox.LogId.ILogId<LogId>
		{
			TraceLine(LogLevel.Info, GetTag<LogId>(), message, arg);
		}

		public static void WarningLine<LogId>(string message, params scoped ReadOnlySpan<object> arg) where LogId : struct, Nox.LogId.ILogId<LogId>
		{
			TraceLine(LogLevel.Warning, GetTag<LogId>(), message, arg);
		}

		public static void WarningLine<LogId>(ref LogInterpolatedStringHandler message) where LogId : struct, Nox.LogId.ILogId<LogId>
		{
			TraceLineFormatted(LogLevel.Warning, GetTag<LogId>(), message.ToStringAndClear());
		}

		public static void ErrorLine<LogId>(string message, params scoped ReadOnlySpan<object> arg) where LogId : struct, Nox.LogId.ILogId<LogId>
		{
			TraceLine(LogLevel.Error, GetTag<LogId>(), message, arg);
		}

		static LogTrace()
		{
			// バックグラウンドスレッドで一括書き込み
			//System.Threading.Thread thread = new(static () => ConsumeLoop())
			//{
			//	IsBackground = true,
			//	Name = "LogTrace"
			//};
			//thread.Start();
		}
		#endregion

		#region 非公開メソッド
		private static void TraceLine(LogLevel logLevel, string tag, string message, params scoped ReadOnlySpan<object> arg)
		{
			string s = string.Format("[{0}][{1}]{2}", GetLogLevelString(logLevel), tag, string.Format(message, arg));

			System.Diagnostics.Trace.WriteLine(s);
			// 呼び出し元をブロックしない非同期書き込み
		//	_Channel.Writer.TryWrite(s);
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

		private static void TraceLineFormatted(LogLevel logLevel, string tag, string formattedMessage)
		{
			string s = $"[{GetLogLevelString(logLevel)}][{tag}]{formattedMessage}";
			System.Diagnostics.Trace.WriteLine(s);
		}

		private static void ConsumeLoop()
		{
			var reader = _Channel.Reader;
			while (true)
			{
				// メッセージが届くまでブロック
				while (reader.TryRead(out string? msg))
				{
					System.Diagnostics.Trace.WriteLine(msg);
				}
				// チャネルにデータが来るまで待機
				reader.WaitToReadAsync().AsTask().GetAwaiter().GetResult();
			}
		}
		#endregion
	}
}
