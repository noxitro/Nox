using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ReflectionGenerator
{
    public static class Trace
    {
        public static void LogLine(ConsoleColor color, string log)
        {
            var temp = Console.ForegroundColor;
            Console.ForegroundColor = color;
            Console.WriteLine(log);
            Console.ForegroundColor = temp;
        }

        public static void Log(ConsoleColor color, string log)
        {
            var temp = Console.ForegroundColor;
            Console.ForegroundColor = color;
            Console.Write(log);
            Console.ForegroundColor = temp;
        }

        public static void Log(ConsoleColor color, object? obj, string log)
        {
            string tag = obj == null ? string.Empty : $"[{obj.ToString()}]";
            Log(color, $"{tag}{log}");
        }

        public static void LogLine(ConsoleColor color, object? obj, string log)
        {
            string tag = obj == null ? string.Empty : $"[{obj.ToString()}]";
            LogLine(color, $"{tag}{log}");
        }

        public static void InfoLine(object? obj, string log)
        {
            LogLine(ConsoleColor.White,obj, log);
        }

        public static void ErrorLine(object? obj, string log)
        {
            LogLine(ConsoleColor.Red, obj, log);
        }

        public static void WarningLine(object? obj, string log)
        {
            LogLine(ConsoleColor.Yellow, obj, log);
        }

        public static void Info(object? obj, string log)
        {
            Log(ConsoleColor.White, obj, log);
        }

        public static void Error(object? obj, string log)
        {
            Log(ConsoleColor.Red, obj, log);
        }

        public static void Warning(object? obj, string log)
        {
            Log(ConsoleColor.Yellow, obj, log);
        }
    }
}
