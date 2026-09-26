// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;
using System.Collections.Generic;
using System.Text;

namespace Nox.Extensions;

	public static class ReadOnlySpanExtensions
	{
		public static int GetHashCode(this scoped ReadOnlySpan<char> span, StringComparison comparisonType)
		{
			return string.GetHashCode(span, comparisonType);
		}
	}
