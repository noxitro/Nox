// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;
using System.Collections.Generic;
using System.Text;

namespace Core.UI.ViewModels;

	public abstract class ToolViewModel : Core.UI.ViewModels.DocumentViewModel
	{
		#region 公開プロパティ
		public override bool CanClose { get; } = false;
		#endregion
	}
