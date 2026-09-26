// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;
using System.ComponentModel;
using System.Windows;

namespace NoxUI;

	/// <summary>
	/// View に対応する ViewModel を規約で見つけて DataContext に設定する添付プロパティ。
	/// <code>noxui:ViewModelLocator.AutoWireViewModel="True"</code>
	///
	/// 規約は Prism の既定と同じ:
	///   名前空間の ".Views." を ".ViewModels." に置き換え、
	///   型名が "View" で終わるなら "Model" を、そうでなければ "ViewModel" を足す。
	///   例: Core.UI.Views.HierarchyView → Core.UI.ViewModels.HierarchyViewModel
	/// 解決は <see cref="ServiceLocator.GetOrCreate"/> に委ねるので、DI に登録して
	/// あればそれが、無ければコンストラクタ注入だけで新規生成される。
	/// </summary>
	public static class ViewModelLocator
	{
		public static readonly DependencyProperty AutoWireViewModelProperty = DependencyProperty.RegisterAttached(
			"AutoWireViewModel",
			typeof(bool),
			typeof(ViewModelLocator),
			new PropertyMetadata(false, OnAutoWireViewModelChanged));

		public static bool GetAutoWireViewModel(DependencyObject element)
		{
			return (bool)element.GetValue(AutoWireViewModelProperty);
		}

		public static void SetAutoWireViewModel(DependencyObject element, bool value)
		{
			element.SetValue(AutoWireViewModelProperty, value);
		}

		private static void OnAutoWireViewModelChanged(DependencyObject dependencyObject, DependencyPropertyChangedEventArgs e)
		{
			if (e.NewValue is not true || dependencyObject is not FrameworkElement view)
			{
				return;
			}

			// デザイナ上では DI が無いので何もしない (d:DataContext に任せる)
			if (DesignerProperties.GetIsInDesignMode(view))
			{
				return;
			}

			Type? viewModelType = ResolveViewModelType(view.GetType());
			if (viewModelType == null)
			{
				throw new InvalidOperationException($"{view.GetType().FullName} に対応する ViewModel が見つからない。規約 (Views→ViewModels、+ViewModel) を確認すること。");
			}

			view.DataContext = ServiceLocator.GetOrCreate(viewModelType);
		}

		private static Type? ResolveViewModelType(Type viewType)
		{
			string viewName = viewType.FullName ?? viewType.Name;
			string viewModelName = viewName.Replace(".Views.", ".ViewModels.", StringComparison.Ordinal);
			viewModelName += viewModelName.EndsWith("View", StringComparison.Ordinal) ? "Model" : "ViewModel";
			return viewType.Assembly.GetType(viewModelName);
		}
	}
