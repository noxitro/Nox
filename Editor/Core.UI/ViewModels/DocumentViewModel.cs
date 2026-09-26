// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;
using System.Collections.Generic;
using System.Text;

namespace Core.UI.ViewModels;

	public abstract class DocumentViewModel : NoxUI.ViewModelBase
	{
		#region 非公開フィールド
		#endregion

		#region 公開プロパティ
		public string Title
		{
			get => field;
			init => SetProperty(ref field, value);
		} = string.Empty;

		public virtual bool CanClose { get; } = true;
		#endregion
	}
