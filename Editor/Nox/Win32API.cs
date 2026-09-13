// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;
using System.Runtime.InteropServices;

namespace Nox;

	/// <summary>
	/// Win32 API の P/Invoke 定義
	/// .NET 10 では LibraryImport（ソース生成）を使用
	/// </summary>
	public static partial class Win32API
	{
		#region Window Relationship
		[LibraryImport("user32.dll", SetLastError = true)]
		public static partial IntPtr SetParent(IntPtr hWndChild, IntPtr hWndNewParent);

		[LibraryImport("user32.dll", SetLastError = true)]
		public static partial IntPtr GetParent(IntPtr hWnd);
		#endregion

		#region Window Style
		[LibraryImport("user32.dll", EntryPoint = "GetWindowLongW", SetLastError = true)]
		public static partial int GetWindowLong(IntPtr hWnd, int nIndex);

		[LibraryImport("user32.dll", EntryPoint = "SetWindowLongW", SetLastError = true)]
		public static partial int SetWindowLong(IntPtr hWnd, int nIndex, int dwNewLong);
		#endregion

		#region Window Position / Visibility
		[LibraryImport("user32.dll", SetLastError = true)]
		[return: MarshalAs(UnmanagedType.Bool)]
		public static partial bool MoveWindow(IntPtr hWnd, int x, int y, int nWidth, int nHeight, [MarshalAs(UnmanagedType.Bool)] bool bRepaint);

		[LibraryImport("user32.dll", SetLastError = true)]
		[return: MarshalAs(UnmanagedType.Bool)]
		public static partial bool ShowWindow(IntPtr hWnd, int nCmdShow);

		[LibraryImport("user32.dll", SetLastError = true)]
		[return: MarshalAs(UnmanagedType.Bool)]
		public static partial bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter, int x, int y, int cx, int cy, uint uFlags);
		#endregion

		#region Window Info
		[LibraryImport("user32.dll", SetLastError = true)]
		[return: MarshalAs(UnmanagedType.Bool)]
		public static partial bool IsWindow(IntPtr hWnd);

		[LibraryImport("user32.dll", SetLastError = true)]
		[return: MarshalAs(UnmanagedType.Bool)]
		public static partial bool IsWindowVisible(IntPtr hWnd);
		#endregion

		#region Focus
		[LibraryImport("user32.dll", SetLastError = true)]
		public static partial IntPtr SetFocus(IntPtr hWnd);
		#endregion

		#region Constants - GetWindowLong index
		public const int GWL_STYLE   = -16;
		public const int GWL_EXSTYLE = -20;
		#endregion

		#region Constants - Window Styles
		public enum WindowStyle : int
		{
			CHILD      = unchecked((int)0x40000000),
			VISIBLE    = 0x10000000,
			CAPTION    = 0x00C00000,
			THICKFRAME = 0x00040000,
			MINIMIZE   = 0x20000000,
			MAXIMIZE   = 0x01000000,
			SYSMENU    = 0x00080000,
			BORDER     = 0x00800000,
			POPUP      = unchecked((int)0x80000000),

		}
		public const int WS_CHILD      = unchecked((int)0x40000000);
		public const int WS_VISIBLE    = 0x10000000;
		public const int WS_CAPTION    = 0x00C00000;
		public const int WS_THICKFRAME = 0x00040000;
		public const int WS_MINIMIZE   = 0x20000000;
		public const int WS_MAXIMIZE   = 0x01000000;
		public const int WS_SYSMENU    = 0x00080000;
		public const int WS_BORDER     = 0x00800000;
		public const int WS_POPUP      = unchecked((int)0x80000000);
		#endregion

		#region Constants - ShowWindow commands
		public enum ShowWindowCommand : int
		{
			HIDE    = 0,
			SHOW    = 5,
			RESTORE = 9,
		}
		public const int SW_HIDE    = 0;
		public const int SW_SHOW    = 5;
		public const int SW_RESTORE = 9;
		#endregion

		#region Constants - SetWindowPos flags
		public enum SetWindowPosFlags : uint
		{
			NOSIZE     = 0x0001,
			NOMOVE     = 0x0002,
			NOZORDER   = 0x0004,
			FRAMECHANGED = 0x0020,
		}

		public const uint SWP_NOSIZE     = 0x0001;
		public const uint SWP_NOMOVE     = 0x0002;
		public const uint SWP_NOZORDER   = 0x0004;
		public const uint SWP_FRAMECHANGED = 0x0020;
		#endregion
	}
