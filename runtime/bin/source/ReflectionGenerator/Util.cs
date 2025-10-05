using System;
using System.Collections.Generic;
using System.Linq;

namespace ReflectionGenerator
{
    public enum CharKind : byte
    {
        Char,
        Char8,
        Char16,
        Char32,
        WChar16,
    }

	public class ScopeProfiler : IDisposable
    {
        private System.Diagnostics.Stopwatch _Stopwatch = new ();
        public string Tag { private get; init; } = "Unknown";

        public ScopeProfiler()
        {
            _Stopwatch.Start();
		}

        void IDisposable.Dispose()
        {
            Trace.InfoLine(null, $"[{Tag}]{_Stopwatch.ElapsedMilliseconds.ToString()}ms");
			_Stopwatch.Stop();

		}
	}

    public static class Util
    {
        #region 公開メソッド
        public static char GetCharPrefix(CharKind kind)
        {
            return kind switch
            {
                CharKind.Char => '\"',
                CharKind.Char8 => 'u',
                CharKind.Char16 => 'u',
                CharKind.Char32 => 'U',
                CharKind.WChar16 => 'L',
                _ => throw new NotImplementedException(),
            };
        }

        public static string ToCppString(ReadOnlySpan<char> str, CharKind kind = CharKind.Char8)
        {
            return kind switch
            {
                CharKind.Char => $"\"{str.ToString()}\"",
                CharKind.Char8 => $"u8\"{str.ToString()}\"",
                CharKind.Char16 => $"u\"{str.ToString()}\"",
                CharKind.Char32 => $"U\"{str.ToString()}\"",
                CharKind.WChar16 => $"L\"{str.ToString()}\"",
                _ => throw new NotImplementedException(),
            };
        }

        public static uint BitOr(uint a, uint b) => a | b;
        public static uint BitXor(uint a, uint b) => a & ~b;

        public static uint SetBit(uint thisBits, uint bits, bool flag)  => flag == true ? BitOr(thisBits, bits) : BitXor(thisBits, bits);

        public static string GetRuntimeRootNamespaceScope() => Define.RUNTIME_ROOT_NAMESPACE_STR;
        public static string GetRuntimeReflectionNamespaceScope() => $"{Define.RUNTIME_ROOT_NAMESPACE_STR}::{Define.RUNTIME_REFLECTION_NAMESPACE_STR}";

        /// <summary>
        /// BreakPoint
        /// </summary>
        [System.Diagnostics.Conditional("DEBUG")]
        public static void BreakPoint() { }

		[System.Diagnostics.Conditional("DEBUG")]
		[System.Runtime.CompilerServices.OverloadResolutionPriority(-1)] // lower priority than (bool, string) overload so that the compiler prefers using CallerArgumentExpression
		public static void Assert([System.Diagnostics.CodeAnalysis.DoesNotReturnIf(false)] bool condition, [System.Runtime.CompilerServices.CallerArgumentExpression(nameof(condition))] string? message = null) =>
			System.Diagnostics.Debug.Assert(condition, message);

		private static int getNumMaxThreads()
        {
            System.Threading.ThreadPool.GetMaxThreads(out int maxThreads, out int completionPortThreads);
            return maxThreads;
        }

        public static readonly int MAX_THREAD_ID = getNumMaxThreads();

        public static void ParallelFor(int fromInclusive, int toExclusive, Action<int> func, int maxDegreeOfParallelism = 1)
        {
            if (maxDegreeOfParallelism <= 1)
            {
				//  シングルスレッドで実行する場合は、Parallel.Forを使用せずに通常のforループを使用します。
				//  例外が発生する可能性があるため、Parallel.Forを使用しないようにします。
				for (int i = fromInclusive; i < toExclusive; i++)
                {
                    func(i);
				}
			}
            else
            {
                System.Threading.Tasks.Parallel.For(fromInclusive, toExclusive, new System.Threading.Tasks.ParallelOptions()
                {
                    MaxDegreeOfParallelism = maxDegreeOfParallelism
                }, func);
            }
        }

        public static void ParallelForEach<T>(IEnumerable<T> source, Action<T> func, int maxDegreeOfParallelism = 1)
        {
            System.Threading.Tasks.Parallel.ForEach(source, new System.Threading.Tasks.ParallelOptions()
            {
                MaxDegreeOfParallelism = maxDegreeOfParallelism
            }, func);
		}

		public static uint Crc32(string str)
        {
			const uint CRC32POLY2 = 0xEDB88320U;/* 左右逆転 */
            const byte STRING_TYPE_BIT_COUNT = 16;// 文字列のビット数

			uint r = 0xFFFFFFFFU;
			for (int i = 0, length = str.Length; i < length ; i++)
			{
				r ^= str[i];
				for (int j = 0; j < STRING_TYPE_BIT_COUNT; j++)
				{
					if ((r & 1) != 0)
					{
						r = (r >> 1) ^ CRC32POLY2;
					}
					else
					{
						r >>= 1;
					}
				}
			}
			return r ^ 0xFFFFFFFFU;
		}
        #endregion
    }
}
