using System;
using System.Collections.Generic;

namespace Nox
{
	

	public static partial class Util
	{
		#region 内部クラス定義
		/// <summary>
		/// Assert 用 Interpolated String Handler。
		/// condition=true のとき文字列を一切構築しない。
		/// </summary>
		[System.Runtime.CompilerServices.InterpolatedStringHandler]
		public ref struct AssertInterpolatedStringHandler
		{
			private System.Runtime.CompilerServices.DefaultInterpolatedStringHandler _inner;
			private readonly bool _enabled;

			public AssertInterpolatedStringHandler(
				int literalLength,
				int formattedCount,
				bool condition,
				out bool shouldAppend)
			{
				_enabled = !condition;
				shouldAppend = _enabled;
				if (_enabled)
					_inner = new System.Runtime.CompilerServices.DefaultInterpolatedStringHandler(literalLength, formattedCount);
			}

			public void AppendLiteral(string s)
			{
				if (_enabled) _inner.AppendLiteral(s);
			}

			public void AppendFormatted<T>(T value)
			{
				if (_enabled) _inner.AppendFormatted(value);
			}

			public void AppendFormatted<T>(T value, string? format)
			{
				if (_enabled) _inner.AppendFormatted(value, format);
			}

			// ⭐ ReadOnlySpan<char> 専用オーバーロード（ref struct はジェネリクス不可）
			public void AppendFormatted(ReadOnlySpan<char> value)
			{
				if (_enabled) _inner.AppendFormatted(value);
			}

			internal readonly string GetText() => _enabled ? _inner.ToStringAndClear() : string.Empty;
		}
		#endregion

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

		/// <summary>補間文字列オーバーロード（condition=true のとき文字列構築しない）</summary>
		[System.Diagnostics.Conditional("DEBUG")]
		public static void Assert(
			[System.Diagnostics.CodeAnalysis.DoesNotReturnIf(false)] bool condition,
			[System.Runtime.CompilerServices.InterpolatedStringHandlerArgument(nameof(condition))]
			scoped ref AssertInterpolatedStringHandler message)
		{
			if (condition)
			{
				return;
			}
			System.Diagnostics.Debug.Assert(false, message.GetText());
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