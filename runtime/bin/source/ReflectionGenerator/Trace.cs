// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ReflectionGenerator;

public static class Trace
{
    private enum LogLevel : byte
    {
        Info,
        Warning,
        Error
    }

    private static string GetLogLevelTag(LogLevel level)
    {
        return level switch
        {
            LogLevel.Info => "[Info]",
            LogLevel.Warning => "[Warning]",
            LogLevel.Error => "[Error]",
            _ => string.Empty
        };
    }

    private static void LogLine(LogLevel logLevel, object? obj, ReadOnlySpan<char> log)
    {
        string tag = obj == null ? string.Empty : $"[{obj.ToString()}]";

        var temp = Console.ForegroundColor;
        Console.ForegroundColor = logLevel switch
        {
            LogLevel.Info => ConsoleColor.White,
            LogLevel.Warning => ConsoleColor.Yellow,
            LogLevel.Error => ConsoleColor.Red,
            _ => ConsoleColor.White
        };
        Console.WriteLine($"{GetLogLevelTag(logLevel)} {log}");
        Console.ForegroundColor = temp;
    }

    private static void Log(LogLevel logLevel, object? obj, ReadOnlySpan<char> log)
    {
        string tag = obj == null ? string.Empty : $"[{obj.ToString()}]";
        var temp = Console.ForegroundColor;
        Console.ForegroundColor = logLevel switch
        {
            LogLevel.Info => ConsoleColor.White,
            LogLevel.Warning => ConsoleColor.Yellow,
            LogLevel.Error => ConsoleColor.Red,
            _ => ConsoleColor.White
        };
        Console.Write(log.ToString());
        Console.ForegroundColor = temp;
    }

    public static void InfoLine(object? obj, string log)
    {
        LogLine(LogLevel.Info, obj, log);
    }

    public static void ErrorLine(object? obj, string log)
    {
        LogLine(LogLevel.Error, obj, log);
    }

    public static void WarningLine(object? obj, string log)
    {
        LogLine(LogLevel.Warning, obj, log);
    }

    public static void Info(object? obj, string log)
    {
        Log(LogLevel.Info, obj, log);
    }

		public static void Info(object? obj, ReadOnlySpan<char> log)
		{
			Log(LogLevel.Info, obj, log);
		}

		public static void Error(object? obj, string log)
    {
        Log(LogLevel.Error, obj, log);
    }

    public static void Warning(object? obj, string log)
    {
        Log(LogLevel.Warning, obj, log);
    }
}
