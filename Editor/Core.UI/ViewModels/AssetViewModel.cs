// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;
using System.Collections.Generic;
using System.Text;

namespace Core.UI.ViewModels;

	public abstract class AssetViewModel : NoxUI.ViewModelBase
	{

	}

	public class AssetViewModel<T> : AssetViewModel
	{
		public T Asset { get; }
		public AssetViewModel(T asset)
		{
			Asset = asset;
		}
	}
