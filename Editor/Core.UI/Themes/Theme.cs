// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;
using System.Collections.Generic;
using System.Text;

namespace Core.UI.Themes;

	public class NoxTheme : AvalonDock.Themes.Theme
	{
		public override Uri GetResourceUri() =>
			new("/Core.UI;component/Themes/NoxTheme.xam", UriKind.Relative);
	}
