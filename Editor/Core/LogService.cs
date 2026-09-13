// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;
using System.Collections.Concurrent;
using System.Threading;

namespace Core;

public sealed class LogService : System.IDisposable
{
    public readonly struct Data
    {
        public Data(System.DateTime timeStamp, string logLevel, string source, string channel, string message)
        {
            TimeStamp = timeStamp;
            LogLevel = logLevel;
            Source = source;
            Channel = channel;
            Message = message;
        }

        public System.DateTime TimeStamp { get; }
        public string LogLevel { get; }
        public string Source { get; }
        public string Channel { get; }
        public string Message { get; }
    }

    #region フィールド
    private readonly LogHubTraceListener _LogHubTraceListener;
    private readonly ConcurrentQueue<Data> _DataQueue = new();
    private int _QueuedCount;
    private int _Initialized;
    #endregion

    public LogService()
    {
        _LogHubTraceListener = new LogHubTraceListener(this);
    }

    public void Initialize()
    {
        if (Interlocked.Exchange(ref _Initialized, 1) == 1)
        {
            return;
        }

        System.Diagnostics.Trace.Listeners.Add(_LogHubTraceListener);
    }

    public void Append(string logLevel, string source, string message, string channel)
    {
        if (string.IsNullOrEmpty(message))
        {
            return;
        }

        Data data = new(System.DateTime.Now, logLevel, source, channel, message);
        _DataQueue.Enqueue(data);
        Interlocked.Increment(ref _QueuedCount);
    }

    public bool TryDequeue(out Data data)
    {
        if (_DataQueue.TryDequeue(out data))
        {
            Interlocked.Decrement(ref _QueuedCount);
            return true;
        }

        return false;
    }

    public void ClearPending()
    {
        while (_DataQueue.TryDequeue(out _))
        {
            Interlocked.Decrement(ref _QueuedCount);
        }
    }

    public bool HasPendingData => Volatile.Read(ref _QueuedCount) > 0;

    void System.IDisposable.Dispose()
    {
        if (Interlocked.Exchange(ref _Initialized, 0) == 1)
        {
            System.Diagnostics.Trace.Listeners.Remove(_LogHubTraceListener);
        }
    }
}

internal sealed class LogHubTraceListener : System.Diagnostics.TraceListener
{
    private readonly LogService _LogService;

    public LogHubTraceListener(LogService logService)
    {
        _LogService = logService;
    }

    public override void Write(string? message)
    {
        AppendTraceMessage(message);
    }

    public override void WriteLine(string? message)
    {
        AppendTraceMessage(message);
    }

    private void AppendTraceMessage(string? message)
    {
        if (string.IsNullOrEmpty(message))
        {
            return;
        }

        ParseTraceMessage(message, out string logLevel, out string source, out string body);
        _LogService.Append(logLevel, "Editor", body, NormalizeEditorChannel(source));
    }

    private static string NormalizeEditorChannel(string channel)
    {
        return string.IsNullOrWhiteSpace(channel) ? "editor.default" : $"editor.{channel}";
    }

    private static void ParseTraceMessage(string message, out string logLevel, out string source, out string body)
    {
        logLevel = "Info";
        source = "Trace";
        body = message;

        if (message.Length < 4 || message[0] != '[')
        {
            return;
        }

        int logLevelEnd = message.IndexOf(']', 1);
        if (logLevelEnd < 0 || logLevelEnd + 1 >= message.Length || message[logLevelEnd + 1] != '[')
        {
            return;
        }

        int sourceStart = logLevelEnd + 2;
        int sourceEnd = message.IndexOf(']', sourceStart);
        if (sourceEnd < 0)
        {
            return;
        }

        logLevel = message[1..logLevelEnd];
        source = message[sourceStart..sourceEnd];
        body = sourceEnd + 1 < message.Length ? message[(sourceEnd + 1)..] : string.Empty;
    }
}
