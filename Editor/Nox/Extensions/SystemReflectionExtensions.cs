// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;
using System.Collections.Generic;
using System.Text;

namespace Nox.Extensions;

	public static class SystemReflectionExtensions
	{
		#region 公開メソッド
		public static string GetFullName(this System.Type self)
		{
			string? fullName = self.FullName;
			Nox.Util.Assert(fullName != null, "型のFullNameがnullでした: {0}", self);
			return fullName;
		}
		#endregion
	}
