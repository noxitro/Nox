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
	/// RutnimeSceneView.xaml の相互作用ロジック
	/// </summary>
	public partial class RutnimeSceneView : System.Windows.Controls.UserControl, NoxUI.IDependencyObject<RutnimeSceneView>
	{
		#region 非公開フィールド
		private readonly Core.UI.Views.SceneViewPanel _SceneViewPanel = new();
		//private Core.RuntimeWrapper.SceneView? _SubscribedSceneView;

		#endregion

		#region 公開プロパティ
		//public static readonly System.Windows.DependencyProperty SceneViewProperty =
		//	NoxUI.IDependencyObject<RutnimeSceneView>.Register<Core.RuntimeWrapper.SceneView?>(nameof(SceneView), null, ChangeSceneViewProperty);

		//public Core.RuntimeWrapper.SceneView? SceneView
		//{
		//	get => (Core.RuntimeWrapper.SceneView)GetValue(SceneViewProperty);
		//	set => SetValue(SceneViewProperty, value);
		//}

		public static readonly System.Windows.DependencyProperty WindowHandleProperty =
			NoxUI.IDependencyObject<RutnimeSceneView>.Register<IntPtr>(nameof(WindowHandle), IntPtr.Zero, ChangeWindowHandleProperty);

		public IntPtr WindowHandle
		{
			get => (IntPtr)GetValue(WindowHandleProperty);
			set => SetValue(WindowHandleProperty, value);
		}

		public static readonly System.Windows.DependencyProperty IsAttachedProperty =
			NoxUI.IDependencyObject<RutnimeSceneView>.Register<bool>(nameof(IsAttached), new System.Windows.PropertyMetadata(false));

		public bool IsAttached
		{
			get => (bool)GetValue(IsAttachedProperty);
			private set => SetValue(IsAttachedProperty, value);
		}

		public static readonly System.Windows.DependencyProperty AttachErrorProperty =
			NoxUI.IDependencyObject<RutnimeSceneView>.Register<string>(nameof(AttachError), new System.Windows.PropertyMetadata(string.Empty));

		public string AttachError
		{
			get => (string)GetValue(AttachErrorProperty);
			private set => SetValue(AttachErrorProperty, value);
		}
		#endregion

		public RutnimeSceneView()
		{
			InitializeComponent();
#if DEBUG
			if (System.ComponentModel.DesignerProperties.GetIsInDesignMode(this))
				return;
#endif
//			_SceneViewPanel = new SceneViewPanel();
			this.WinFormsHost.Child = _SceneViewPanel;
			Loaded += OnLoaded;
			Unloaded += OnUnloaded;
		}

		#region 非公開メソッド
		private static void ChangeSceneViewProperty(RutnimeSceneView owner, in DependencyPropertyChangedEventArgs e)
		{
		//	owner.UpdateSceneViewSubscription((Core.RuntimeWrapper.SceneView?)e.NewValue);
			owner.AttachCurrentWindow();
		}

		private static void ChangeWindowHandleProperty(RutnimeSceneView owner, in DependencyPropertyChangedEventArgs e)
		{
			owner.AttachCurrentWindow();
		}

		private void OnSceneViewWindowHandleChanged(object? sender, EventArgs e)
		{
			if (NoxUI.DispatcherHelper.IsShuttingDown(Dispatcher) || IsLoaded == false)
			{
				return;
			}

			if (Dispatcher.CheckAccess() == false)
			{
				NoxUI.DispatcherHelper.TryBeginInvoke(Dispatcher, () => OnSceneViewWindowHandleChanged(sender, e));
				return;
			}

		//	if (sender is Core.RuntimeWrapper.SceneView sceneView)
			{
				AttachCurrentWindow();
			}
		}

		private void OnLoaded(object sender, RoutedEventArgs e)
		{
		//	UpdateSceneViewSubscription(SceneView);
			AttachCurrentWindow();
		}

		private void OnUnloaded(object sender, RoutedEventArgs e)
		{
		//	UpdateSceneViewSubscription(null);
			_SceneViewPanel.Detach();
			IsAttached = false;
			AttachError = string.Empty;
		}

		//private void UpdateSceneViewSubscription(Core.RuntimeWrapper.SceneView? sceneView)
		//{
		//	if (ReferenceEquals(_SubscribedSceneView, sceneView))
		//	{
		//		return;
		//	}

		//	if (_SubscribedSceneView != null)
		//	{
		//		_SubscribedSceneView.WindowHandleChanged -= OnSceneViewWindowHandleChanged;
		//	}

		//	_SubscribedSceneView = sceneView;
		//	if (_SubscribedSceneView != null && IsLoaded)
		//	{
		//		_SubscribedSceneView.WindowHandleChanged += OnSceneViewWindowHandleChanged;
		//	}
		//}

		private void AttachCurrentWindow()
		{
			if (IsLoaded == false || NoxUI.DispatcherHelper.IsShuttingDown(Dispatcher))
			{
				_SceneViewPanel.Detach();
				IsAttached = false;
				AttachError = string.Empty;
				return;
			}

			IntPtr windowHandle = WindowHandle;
		//	if (windowHandle == IntPtr.Zero && SceneView != null)
			{
		//		windowHandle = SceneView.WindowHandle;
			}

			if (windowHandle != IntPtr.Zero)
			{
				try
				{
					_SceneViewPanel.Attach(windowHandle);
					IsAttached = _SceneViewPanel.IsAttached;
					AttachError = _SceneViewPanel.LastAttachError;
				}
				catch (Exception ex)
				{
					Nox.LogTrace.ErrorLine<Core.LogId.Runtime>("Runtime SceneView attach failed: {0}", ex);
					IsAttached = false;
					AttachError = ex.Message;
				}
			}
			else
			{
				_SceneViewPanel.Detach();
				IsAttached = false;
				AttachError = string.Empty;
			}
		}
		#endregion
	}
