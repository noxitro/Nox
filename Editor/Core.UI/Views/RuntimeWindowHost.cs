using System;

namespace Core.UI.Views
{
	//public static class Helper
	//{
	//	static System.Windows.DependencyProperty MakeDependencyProperty<PropertyType, OwnerType>(string path)
	//	{
	//		return System.Windows.DependencyProperty.Register(path, typeof(PropertyType), typeof(OwnerType));
	//	}
	//}

	//public class RuntimeWindowHost : System.Windows.Interop.HwndHost
	//{
	//	public static readonly System.Windows.DependencyProperty ExecutablePathProperty =
	//		System.Windows.DependencyProperty.Register(nameof(ExecutablePath), typeof(string), typeof(RuntimeWindowHost), new System.Windows.PropertyMetadata(string.Empty));
	//	public string ExecutablePath
	//	{
	//		get => (string)GetValue(ExecutablePathProperty);
	//		set => SetValue(ExecutablePathProperty, value);
	//	}

	//	public static readonly System.Windows.DependencyProperty ArgumentsProperty =
	//		System.Windows.DependencyProperty.Register(nameof(Arguments), typeof(string), typeof(RuntimeWindowHost), new System.Windows.PropertyMetadata(string.Empty));
	//	public string Arguments
	//	{
	//		get => (string)GetValue(ArgumentsProperty);
	//		set => SetValue(ArgumentsProperty, value);
	//	}

	//	public static readonly System.Windows.DependencyProperty ProcessProperty =
	//		System.Windows.DependencyProperty.Register(nameof(Process), typeof(System.Diagnostics.Process), typeof(RuntimeWindowHost), new System.Windows.PropertyMetadata(null));
	//	public System.Diagnostics.Process? Process
	//	{
	//		get => (System.Diagnostics.Process?)GetValue(ProcessProperty);
	//		set => SetValue(ProcessProperty, value);
	//	}

	//	public static readonly System.Windows.DependencyProperty MainWindowHandleProperty =
	//		System.Windows.DependencyProperty.Register(nameof(MainWindowHandle), typeof(System.IntPtr), typeof(RuntimeWindowHost), new System.Windows.PropertyMetadata(System.IntPtr.Zero));
	//	public System.IntPtr MainWindowHandle
	//	{
	//		get => (System.IntPtr)GetValue(MainWindowHandleProperty);
	//		set => SetValue(MainWindowHandleProperty, value);
	//	}

	//	private System.Diagnostics.Process? _process;
	//	private System.IntPtr _runtimeHwnd = System.IntPtr.Zero;
	//	private bool _ownsProcess = false;
	//	private bool _placeholderOnly = false;
	//	private bool _isDesignMode => System.ComponentModel.DesignerProperties.GetIsInDesignMode(this);

	//	protected override System.Runtime.InteropServices.HandleRef BuildWindowCore(System.Runtime.InteropServices.HandleRef hwndParent)
	//	{
	//		if (_isDesignMode)
	//		{
	//			_runtimeHwnd = CreatePlaceholder(hwndParent.Handle, "Runtime (design preview)");
	//			_placeholderOnly = true;
	//			return new System.Runtime.InteropServices.HandleRef(this, _runtimeHwnd);
	//		}

	//		_process = Process;
	//		_ownsProcess = false;
	//		_placeholderOnly = false;

	//		// プロセスが既に終了していればクリア
	//		if (_process is { HasExited: true })
	//		{
	//			_process.Dispose();
	//			_process = null;
	//		}

	//		// プロセスが無い場合は起動する。ただしパス未設定/不存在ならプレースホルダーで継続。
	//		if (_process == null)
	//		{
	//			if (string.IsNullOrWhiteSpace(ExecutablePath) || !System.IO.File.Exists(ExecutablePath))
	//			{
	//				_runtimeHwnd = CreatePlaceholder(hwndParent.Handle, "runtime.exe が見つかりません");
	//				_placeholderOnly = true;
	//				return new System.Runtime.InteropServices.HandleRef(this, _runtimeHwnd);
	//			}

	//			try
	//			{
	//				_process = System.Diagnostics.Process.Start(new System.Diagnostics.ProcessStartInfo
	//				{
	//					FileName = ExecutablePath,
	//					Arguments = Arguments,
	//					UseShellExecute = true,
	//				}) ?? throw new System.ComponentModel.Win32Exception("Failed to start runtime process.");
	//				_ownsProcess = true;
	//			}
	//			catch (System.ComponentModel.Win32Exception ex)
	//			{
	//				_runtimeHwnd = CreatePlaceholder(hwndParent.Handle, $"runtime.exe 起動失敗: {ex.Message}");
	//				_placeholderOnly = true;
	//				return new System.Runtime.InteropServices.HandleRef(this, _runtimeHwnd);
	//			}
	//		}

	//		_runtimeHwnd = MainWindowHandle != System.IntPtr.Zero
	//			? MainWindowHandle
	//			: WaitForMainWindow(_process, System.TimeSpan.FromSeconds(10));

	//		if (_runtimeHwnd == System.IntPtr.Zero)
	//		{
	//			_runtimeHwnd = CreatePlaceholder(hwndParent.Handle, "Runtime window not found");
	//			_placeholderOnly = true;
	//			return new System.Runtime.InteropServices.HandleRef(this, _runtimeHwnd);
	//		}

	//		SetParent(_runtimeHwnd, hwndParent.Handle);

	//		int style = GetWindowLong(_runtimeHwnd, GWL_STYLE);
	//		style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);
	//		SetWindowLong(_runtimeHwnd, GWL_STYLE, style);

	//		ShowWindow(_runtimeHwnd, SW_SHOW);
	//		ResizeChild(ActualWidth, ActualHeight);

	//		return new System.Runtime.InteropServices.HandleRef(this, hwndParent.Handle);
	//	}

	//	protected override void DestroyWindowCore(System.Runtime.InteropServices.HandleRef hwnd)
	//	{
	//		if (_runtimeHwnd != System.IntPtr.Zero)
	//		{
	//			if (_placeholderOnly || _isDesignMode)
	//			{
	//				DestroyWindow(_runtimeHwnd);
	//			}
	//			else
	//			{
	//				SetParent(_runtimeHwnd, System.IntPtr.Zero);
	//			}
	//			_runtimeHwnd = System.IntPtr.Zero;
	//		}

	//		if (!_isDesignMode && !_placeholderOnly && _ownsProcess && _process is { HasExited: false })
	//		{
	//			try { _process.Kill(entireProcessTree: true); } catch { /* ignore */ }
	//		}
	//		_process?.Dispose();
	//		_process = null;
	//		_placeholderOnly = false;
	//		_ownsProcess = false;
	//	}

	//	protected override void OnWindowPositionChanged(System.Windows.Rect rcBoundingBox)
	//	{
	//		base.OnWindowPositionChanged(rcBoundingBox);
	//		if (_runtimeHwnd != System.IntPtr.Zero)
	//		{
	//			MoveWindow(_runtimeHwnd, 0, 0, (int)rcBoundingBox.Width, (int)rcBoundingBox.Height, true);
	//		}
	//	}

	//	private void ResizeChild(double width, double height)
	//	{
	//		if (_runtimeHwnd != System.IntPtr.Zero)
	//		{
	//			MoveWindow(_runtimeHwnd, 0, 0, (int)width, (int)height, true);
	//		}
	//	}

	//	private static System.IntPtr WaitForMainWindow(System.Diagnostics.Process process, System.TimeSpan timeout)
	//	{
	//		var sw = System.Diagnostics.Stopwatch.StartNew();
	//		while (sw.Elapsed < timeout)
	//		{
	//			if (process.HasExited)
	//				break;
	//			process.Refresh();
	//			if (process.MainWindowHandle != System.IntPtr.Zero)
	//				return process.MainWindowHandle;
	//			System.Threading.Thread.Sleep(50);
	//		}
	//		return System.IntPtr.Zero;
	//	}

	//	private static System.IntPtr CreatePlaceholder(System.IntPtr parent, string text)
	//	{
	//		return CreateWindowEx(
	//			0,
	//			"STATIC",
	//			text,
	//			WS_CHILD | WS_VISIBLE,
	//			0, 0,
	//			200, 50,
	//			parent,
	//			System.IntPtr.Zero,
	//			System.IntPtr.Zero,
	//			System.IntPtr.Zero);
	//	}

	//	#region Win32
	//	private const int GWL_STYLE = -16;
	//	private const int WS_CAPTION = 0x00C00000;
	//	private const int WS_THICKFRAME = 0x00040000;
	//	private const int WS_MINIMIZE = 0x20000000;
	//	private const int WS_MAXIMIZE = 0x01000000;
	//	private const int WS_SYSMENU = 0x00080000;
	//	private const int WS_CHILD = unchecked((int)0x40000000);
	//	private const int WS_VISIBLE = 0x10000000;
	//	private const int SW_SHOW = 5;

	//	[System.Runtime.InteropServices.DllImport("user32.dll", SetLastError = true)]
	//	private static extern System.IntPtr SetParent(System.IntPtr hWndChild, System.IntPtr hWndNewParent);

	//	[System.Runtime.InteropServices.DllImport("user32.dll", SetLastError = true)]
	//	private static extern int GetWindowLong(System.IntPtr hWnd, int nIndex);

	//	[System.Runtime.InteropServices.DllImport("user32.dll", SetLastError = true)]
	//	private static extern int SetWindowLong(System.IntPtr hWnd, int nIndex, int dwNewLong);

	//	[System.Runtime.InteropServices.DllImport("user32.dll", SetLastError = true)]
	//	private static extern bool MoveWindow(System.IntPtr hWnd, int X, int Y, int nWidth, int nHeight, bool bRepaint);

	//	[System.Runtime.InteropServices.DllImport("user32.dll", SetLastError = true)]
	//	private static extern bool ShowWindow(System.IntPtr hWnd, int nCmdShow);

	//	[System.Runtime.InteropServices.DllImport("user32.dll", SetLastError = true, CharSet = System.Runtime.InteropServices.CharSet.Unicode)]
	//	private static extern System.IntPtr CreateWindowEx(
	//		int dwExStyle,
	//		string lpClassName,
	//		string lpWindowName,
	//		int dwStyle,
	//		int x,
	//		int y,
	//		int nWidth,
	//		int nHeight,
	//		System.IntPtr hWndParent,
	//		System.IntPtr hMenu,
	//		System.IntPtr hInstance,
	//		System.IntPtr lpParam);

	//	[System.Runtime.InteropServices.DllImport("user32.dll", SetLastError = true)]
	//	[return: System.Runtime.InteropServices.MarshalAs(System.Runtime.InteropServices.UnmanagedType.Bool)]
	//	private static extern bool DestroyWindow(System.IntPtr hWnd);
	//	#endregion
	//}
}