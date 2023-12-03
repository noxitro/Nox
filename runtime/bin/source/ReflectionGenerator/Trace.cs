using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ReflectionGenerator
{
    public static class Trace
    {
        public static void Log(ConsoleColor color, string log)
        {
            var temp = Console.ForegroundColor;
            Console.ForegroundColor = color;
            Console.WriteLine(log);
            Console.ForegroundColor = temp;
        }

        public static void Log(ConsoleColor color, object? obj, string log)
        {
            Log(color, string.Format("[{0}]{1}", obj != null ? obj.ToString() : string.Empty, log));
        }

        public static void Info(object? obj, string log)
        {
            Log(ConsoleColor.White,obj, log);
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
