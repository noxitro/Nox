// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System.Collections;
using System.Windows;

namespace Core.UI.Views;

	public partial class PropertyInspectorView : System.Windows.Controls.UserControl
	{
		public static readonly DependencyProperty ItemsSourceProperty = DependencyProperty.Register(
			nameof(ItemsSource),
			typeof(IEnumerable),
			typeof(PropertyInspectorView),
			new PropertyMetadata(null));

		public static readonly DependencyProperty LabelWidthProperty = DependencyProperty.Register(
			nameof(LabelWidth),
			typeof(GridLength),
			typeof(PropertyInspectorView),
			new PropertyMetadata(new GridLength(120)));

		public static readonly DependencyProperty PropertyInspectorAutomationIdProperty = DependencyProperty.Register(
			nameof(PropertyInspectorAutomationId),
			typeof(string),
			typeof(PropertyInspectorView),
			new PropertyMetadata("NoxStudio.PropertyInspector.PropertyList"));

		public PropertyInspectorView()
		{
			InitializeComponent();
		}

		public IEnumerable? ItemsSource
		{
			get => (IEnumerable?)GetValue(ItemsSourceProperty);
			set => SetValue(ItemsSourceProperty, value);
		}

		public GridLength LabelWidth
		{
			get => (GridLength)GetValue(LabelWidthProperty);
			set => SetValue(LabelWidthProperty, value);
		}

		public string PropertyInspectorAutomationId
		{
			get => (string)GetValue(PropertyInspectorAutomationIdProperty);
			set => SetValue(PropertyInspectorAutomationIdProperty, value);
		}
	}
