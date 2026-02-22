using System;
using System.Collections.Generic;

namespace Nox
{
	public static partial class Util
	{
		[System.Diagnostics.Conditional("DEBUG")]
		public static void BreakPoint() { }

		[System.Diagnostics.Conditional("DEBUG")]
		public static void Assert([System.Diagnostics.CodeAnalysis.DoesNotReturnIf(false)] bool condition, string message, params object[] args)
		{
			if (condition == true)
			{
				return;
			}

			System.Diagnostics.Debug.Assert(condition, string.Format(message, args));
		}

		public static T Cast<T>(object obj) where T : class //where U : class
		{
			T? result = obj as T;
			Nox.Util.Assert(result != null, "キャストに失敗しました {0}", typeof(T).Name);
			return result;
		}

		public static bool IsPowOf<T>(T value, T baseValue) 
			where T : struct, System.Numerics.INumber<T>, System.Numerics.IBitwiseOperators<T, T, T>
		{
			// 1, 2 のリテラルは T.One, T.CreateChecked(2) で取得
			if (baseValue < T.CreateChecked(2) || value < T.One)
				return false;

			if (baseValue == T.CreateChecked(2))
				return (value & (value - T.One)) == T.Zero;

			while (value % baseValue == T.Zero)
			{
				value /= baseValue;
			}
			return value == T.One;
		}

		/// <summary>
		/// 即時実行関数
		/// lambda式の即時実行に使う
		/// </summary>
		
		[System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.AggressiveInlining)]
		public static T Invoke<T>(Func<T> func) => func();
	}
}