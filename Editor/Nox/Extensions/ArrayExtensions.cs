// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;
using System.Collections.Generic;
using System.Text;

namespace Nox.Extensions;

	public static class ArrayExtensions
	{
		public delegate bool InPredicate<T>(in T element) where T : struct, allows ref struct;

		public static T? Find<T>(this T[] self, System.Predicate<T> match) where T : class
		{
			return Array.Find(self, match);
		}

		public static T? Find<T>(this T[] self, InPredicate<T> match) where T : struct
		{
			for (int i = 0; i < self.Length; ++i)
			{
				if (match(in self[i]))
				{
					return self[i];
				}
			}
			return null;
		}

		public static int FindIndex<T>(this T[] self, System.Predicate<T> match) where T : class
		{
			return Array.FindIndex(self, match);
		}

		public static int FindIndex<T>(this T[] self, InPredicate<T> match) where T : struct
		{
			for (int i = 0; i < self.Length; ++i)
			{
				if (match(in self[i]))
				{
					return i;
				}
			}
			return -1;
		}
	}
