// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;
using System.Collections.Generic;
using System.Linq;

namespace ReflectionGenerator;

public enum CharKind : byte
{
    Char,
    Char8,
    Char16,
    Char32,
    WChar16,
}

	public readonly struct ScopeProfiler : System.IDisposable
{
    private readonly System.Diagnostics.Stopwatch _Stopwatch = new ();
    public string Tag { private get; init; } = "Unknown";

    public ScopeProfiler()
    {
        _Stopwatch.Start();
		}

    void System.IDisposable.Dispose()
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

		[System.Diagnostics.Conditional("DEBUG")]
		public static void Assert([System.Diagnostics.CodeAnalysis.DoesNotReturnIf(false)] bool condition, string message, params object[] args)
		{
			if (condition == true)
			{
				return;
			}

			System.Diagnostics.Debug.Assert(condition, string.Format(message, args));
		}

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

    /// <summary>
    /// 即時関数実行
    /// </summary>
    [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.AggressiveInlining)]
		public static T ImmediateInvoke<T>(Func<T> f) => f();

    /// <summary>
    /// 即時関数実行
    /// </summary>
    [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.AggressiveInlining)]
    public static void ImmediateInvoke(Action f) => f();
    #endregion
}

	public static class EnumUtility<T> where T : struct, System.Enum
	{
		#region 公開プロパティ
		public static ReadOnlySpan<T> ValueList => _ValueList;
		public static T[] ValueListRaw => _ValueList;

		public static ReadOnlySpan<string> NameList => _NameList;
		public static string[] NameListRaw => _NameList;

		public static int Count => _ValueList.Length;

		public static ReadOnlySpan<U> GetIntegerValueList<U>() where U : struct, System.Numerics.IBinaryInteger<U>
			=> GetIntegerValueListRaw<U>();
		public static U[] GetIntegerValueListRaw<U>() where U : struct, System.Numerics.IBinaryInteger<U>
			=> IntegerHolder<U>.IntegerValueList;

		public static ReadOnlySpan<int> ValueListInt32 => GetIntegerValueList<int>();

		public static ReadOnlySpan<char> GetName(T value)
		{
			ulong index = _IndexDict[value];
			return _NameList[index];
		}

		#endregion

		#region 内部型定義
		private unsafe static class IntegerHolder<U> where U : struct, System.Numerics.INumberBase<U>, System.Numerics.IBinaryInteger<U>
		{
			public static U[] IntegerValueList => _IntegerValueList;

			public static readonly U[] _IntegerValueList = ((Func<U[]>)(() =>
			{
				Type underlying = Enum.GetUnderlyingType(typeof(T));

				Util.Assert(typeof(U) == underlying, "ignore same underlying type");

				T[] valueList = System.Enum.GetValues<T>();
				int valueListLength = valueList.Length;
				U[] integerValueList = new U[valueListLength];

				for (int i = 0; i < valueListLength; i++)
				{
					// 基になる型のサイズに応じて読み出す
					integerValueList[i] = System.Runtime.CompilerServices.Unsafe.As<T, U>(ref valueList[i]);
				}
				return integerValueList;
			})).Invoke();
		}
		#endregion

		#region 非公開フィールド
		private static readonly T[] _ValueList = System.Enum.GetValues<T>();
		private static readonly string[] _NameList = System.Enum.GetNames<T>();
		private static readonly Dictionary<T, ulong> _IndexDict = ((Func<Dictionary<T, ulong>>)(() =>
		{
			Dictionary<T, ulong> dict = new();
			for (uint i = 0; i < _ValueList.Length; i++)
			{
				dict[_ValueList[i]] = i;
			}
			return dict;
		})).Invoke();
		#endregion
	}
