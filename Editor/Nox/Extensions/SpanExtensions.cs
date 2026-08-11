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
