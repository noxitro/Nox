using System;

namespace Core.UI.Views
{
	/// <summary>
	/// runtimeのシーンビューを表示する
	/// WindowsFormsHost.Child として使用し、win32apiで子ウィンドウをホストする
	/// </summary>
	public class SceneViewPanel : System.Windows.Forms.Panel
	{
		#region 非公開フィールド
		private IntPtr _AttachHandle = IntPtr.Zero;
		#endregion

		#region 公開プロパティ
		public IntPtr AttachHandle => _AttachHandle;
		public bool IsAttached => _AttachHandle != IntPtr.Zero;
		#endregion

		#region 公開メソッド
		public void Attach(IntPtr windowHandle)
		{
			if (windowHandle == IntPtr.Zero) return;
			Detach();

			_AttachHandle = windowHandle;

			Nox.Win32API.SetParent(_AttachHandle, this.Handle);  // ← WinForms.Panel なので Handle が使える

			Nox.Win32API.WindowStyle style = (Nox.Win32API.WindowStyle)Nox.Win32API.GetWindowLong(_AttachHandle, Nox.Win32API.GWL_STYLE);

			style &= ~(Nox.Win32API.WindowStyle.CAPTION | Nox.Win32API.WindowStyle.THICKFRAME | Nox.Win32API.WindowStyle.MINIMIZE | Nox.Win32API.WindowStyle.MAXIMIZE | Nox.Win32API.WindowStyle.SYSMENU);
			style |= Nox.Win32API.WindowStyle.CHILD;
			Nox.Win32API.SetWindowLong(_AttachHandle, Nox.Win32API.GWL_STYLE, (int)style);
			Nox.Win32API.ShowWindow(_AttachHandle, (int)Nox.Win32API.ShowWindowCommand.SHOW);

			ResizeChild();
		}

		public void Detach()
		{
			if (_AttachHandle == IntPtr.Zero) return;
			Nox.Win32API.ShowWindow(_AttachHandle, (int)Nox.Win32API.ShowWindowCommand.HIDE);
			Nox.Win32API.SetParent(_AttachHandle, IntPtr.Zero);
			_AttachHandle = IntPtr.Zero;
		}
		#endregion

		#region 非公開メソッド
		protected override void OnResize(EventArgs e)
		{
			base.OnResize(e);
			ResizeChild();
		}

		protected override void Dispose(bool disposing)
		{
			if (disposing) Detach();
			base.Dispose(disposing);
		}

		private void ResizeChild()
		{
			if (_AttachHandle == IntPtr.Zero) return;
			Nox.Win32API.MoveWindow(_AttachHandle, 0, 0, Width, Height, true);
		}
		#endregion
	}
}
