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

namespace Core.UI.Views
{
	/// <summary>
	/// RutnimeSceneView.xaml の相互作用ロジック
	/// </summary>
	public partial class RutnimeSceneView : System.Windows.Controls.UserControl, NoxUI.IDependencyObject<RutnimeSceneView>
	{
		#region 非公開フィールド
		private readonly Core.UI.Views.SceneViewPanel _SceneViewPanel = new();

		#endregion

		#region 公開プロパティ
		public static readonly System.Windows.DependencyProperty SceneViewProperty =
			NoxUI.IDependencyObject<RutnimeSceneView>.Register<Core.RuntimeWrapper.SceneView?>(nameof(SceneView), null, ChangeSceneViewProperty);

		public Core.RuntimeWrapper.SceneView? SceneView
		{
			get => (Core.RuntimeWrapper.SceneView)GetValue(SceneViewProperty);
			set => SetValue(SceneViewProperty, value);
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
		}

		#region 非公開メソッド
		private static void ChangeSceneViewProperty(RutnimeSceneView owner, in DependencyPropertyChangedEventArgs e)
		{
			if (e.OldValue is Core.RuntimeWrapper.SceneView oldSceneView)
			{
				oldSceneView.WindowHandleChanged -= owner.OnSceneViewWindowHandleChanged;
			}

			if (e.NewValue is Core.RuntimeWrapper.SceneView sceneView)
			{
				sceneView.WindowHandleChanged += owner.OnSceneViewWindowHandleChanged;
				owner.AttachSceneView(sceneView);
			}
			else
			{
				owner._SceneViewPanel.Detach();
			}
		}

		private void OnSceneViewWindowHandleChanged(object? sender, EventArgs e)
		{
			if (Dispatcher.CheckAccess() == false)
			{
				Dispatcher.BeginInvoke((Action)(() => OnSceneViewWindowHandleChanged(sender, e)));
				return;
			}

			if (sender is Core.RuntimeWrapper.SceneView sceneView)
			{
				AttachSceneView(sceneView);
			}
		}

		private void AttachSceneView(Core.RuntimeWrapper.SceneView sceneView)
		{
			if (sceneView.WindowHandle != IntPtr.Zero)
			{
				_SceneViewPanel.Attach(sceneView.WindowHandle);
			}
			else
			{
				_SceneViewPanel.Detach();
			}
		}
		#endregion
	}
}
