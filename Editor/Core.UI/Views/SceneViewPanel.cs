using System;

namespace Core.UI.Views;

	/// <summary>
	/// runtimeのシーンビューを表示する
	/// WindowsFormsHost.Child として使用し、win32apiで子ウィンドウをホストする
	/// </summary>
	public class SceneViewPanel : System.Windows.Forms.Panel
	{
		#region 非公開フィールド
		private IntPtr _AttachHandle = IntPtr.Zero;
		private IntPtr _OriginalParent = IntPtr.Zero;
		private int? _OriginalStyle = null;
		#endregion

		#region 公開プロパティ
		public IntPtr AttachHandle => _AttachHandle;
		public bool IsAttached => _AttachHandle != IntPtr.Zero;
		public string LastAttachError { get; private set; } = string.Empty;
		#endregion

		#region 公開メソッド
		public void Attach(IntPtr windowHandle)
		{
			if (windowHandle == IntPtr.Zero)
			{
				LastAttachError = "Runtime window handle is zero.";
				Nox.LogTrace.WarningLine<Core.LogId.Runtime>("Runtime window handle is zero.");
				Detach();
				return;
			}

			if (Nox.Win32API.IsWindow(windowHandle) == false)
			{
				LastAttachError = $"Runtime window handle is invalid: {windowHandle}";
				Nox.LogTrace.WarningLine<Core.LogId.Runtime>("Runtime window handle is invalid: {0}", windowHandle);
				Detach();
				return;
			}

			if (_AttachHandle == windowHandle)
			{
				ResizeChild();
				return;
			}

			Detach();

			_AttachHandle = windowHandle;
			_OriginalParent = Nox.Win32API.GetParent(_AttachHandle);
			_OriginalStyle = Nox.Win32API.GetWindowLong(_AttachHandle, Nox.Win32API.GWL_STYLE);

			Nox.Win32API.WindowStyle style = (Nox.Win32API.WindowStyle)_OriginalStyle.Value;

			style &= ~(Nox.Win32API.WindowStyle.CAPTION | Nox.Win32API.WindowStyle.THICKFRAME | Nox.Win32API.WindowStyle.MINIMIZE | Nox.Win32API.WindowStyle.MAXIMIZE | Nox.Win32API.WindowStyle.SYSMENU | Nox.Win32API.WindowStyle.BORDER | Nox.Win32API.WindowStyle.POPUP);
			style |= Nox.Win32API.WindowStyle.CHILD | Nox.Win32API.WindowStyle.VISIBLE;
			Nox.Win32API.SetWindowLong(_AttachHandle, Nox.Win32API.GWL_STYLE, (int)style);

			System.Runtime.InteropServices.Marshal.SetLastPInvokeError(0);
			IntPtr previousParent = Nox.Win32API.SetParent(_AttachHandle, Handle);
			int setParentError = System.Runtime.InteropServices.Marshal.GetLastPInvokeError();
			if (previousParent == IntPtr.Zero && setParentError != 0)
			{
				LastAttachError = $"SetParent failed. Error={setParentError}";
				Nox.LogTrace.ErrorLine<Core.LogId.Runtime>("Runtime window SetParent failed. hWnd={0}, error={1}", _AttachHandle, setParentError);
				Nox.Win32API.SetWindowLong(_AttachHandle, Nox.Win32API.GWL_STYLE, _OriginalStyle.Value);
				Nox.Win32API.SetWindowPos(_AttachHandle, IntPtr.Zero, 0, 0, 0, 0, Nox.Win32API.SWP_NOMOVE | Nox.Win32API.SWP_NOSIZE | Nox.Win32API.SWP_NOZORDER | Nox.Win32API.SWP_FRAMECHANGED);
				_AttachHandle = IntPtr.Zero;
				_OriginalParent = IntPtr.Zero;
				_OriginalStyle = null;
				return;
			}

			Nox.Win32API.ShowWindow(_AttachHandle, (int)Nox.Win32API.ShowWindowCommand.SHOW);
			Nox.Win32API.SetWindowPos(_AttachHandle, IntPtr.Zero, 0, 0, Width, Height, Nox.Win32API.SWP_NOZORDER | Nox.Win32API.SWP_FRAMECHANGED);
			LastAttachError = string.Empty;

			ResizeChild();
		}

		public void Detach()
		{
			if (_AttachHandle == IntPtr.Zero) return;
			Nox.Win32API.ShowWindow(_AttachHandle, (int)Nox.Win32API.ShowWindowCommand.HIDE);
			if (_OriginalStyle.HasValue)
			{
				Nox.Win32API.SetWindowLong(_AttachHandle, Nox.Win32API.GWL_STYLE, _OriginalStyle.Value);
			}
			Nox.Win32API.SetParent(_AttachHandle, _OriginalParent);
			Nox.Win32API.SetWindowPos(_AttachHandle, IntPtr.Zero, 0, 0, 0, 0, Nox.Win32API.SWP_NOMOVE | Nox.Win32API.SWP_NOSIZE | Nox.Win32API.SWP_NOZORDER | Nox.Win32API.SWP_FRAMECHANGED);
			_AttachHandle = IntPtr.Zero;
			_OriginalParent = IntPtr.Zero;
			_OriginalStyle = null;
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

		protected override void OnHandleDestroyed(EventArgs e)
		{
			Detach();
			base.OnHandleDestroyed(e);
		}

		private void ResizeChild()
		{
			if (_AttachHandle == IntPtr.Zero) return;
			Nox.Win32API.MoveWindow(_AttachHandle, 0, 0, Width, Height, true);
		}
		#endregion
	}
