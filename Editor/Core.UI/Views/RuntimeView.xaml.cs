// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;
using System.Collections.Generic;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace Core.UI.Views;

	/// <summary>
	/// RuntimeView.xaml の相互作用ロジック
	/// </summary>
	public partial class RuntimeView : System.Windows.Controls.UserControl
	{
		public RuntimeView()
		{
			#if DEBUG
			if (System.ComponentModel.DesignerProperties.GetIsInDesignMode(this))
				return;
#endif
			InitializeComponent();
		}
	}
