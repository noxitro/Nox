using System;
using System.Collections.Generic;

namespace Nox;

	public static class EnumExtensions
	{
		public static ReadOnlySpan<char> GetName<T>(this T self) where T : struct, Enum
		{
			return EnumUtility<T>.GetName(self);
		}
	}

	public static class EnumUtility<T> where T : struct, Enum
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

				Nox.Util.Assert(typeof(U) == underlying, "ignore same underlying type");

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
			Dictionary<T, ulong> dict = new ();
			for (uint i = 0; i < _ValueList.Length; i++)
			{
				dict[_ValueList[i]] = i;
			}
			return dict;
		})).Invoke();
		#endregion
	}
