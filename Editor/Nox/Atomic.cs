// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;
using System.Threading;

namespace Nox;

	public interface IAtomic<T> where T : struct, System.Numerics.INumber<T>
	{
		public T Increment();
		public T RawValue { get; }
	}

	/// <summary>
	/// IBinaryInteger 型のアトミック操作ラッパー。
	/// struct のため、フィールドとしてのみ使用すること。コピーすると独立したインスタンスになる。
	/// </summary>
	public struct AtomicInt32 : IAtomic<int>
	{
		public int Increment()
		{
			return Interlocked.Increment(ref _Value);
		}

		private int _Value;
		public readonly int RawValue => _Value;
	}
