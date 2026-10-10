// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Interop;
using System.Windows.Media;

namespace Studio.Wpf;

/// <summary>
/// OS が描くタイトルバーをテーマの色に合わせる。
/// Windows 11 (ビルド 22000 以降) ではキャプション・文字・枠線の色を指定できる。
/// Windows 10 (ビルド 19041 以降) では暗色モードの指定だけが効き、他の属性は失敗を返すので無視する。
/// </summary>
internal static class WindowCaptionTheme
{
    private const int DwmwaUseImmersiveDarkMode = 20;
    private const int DwmwaBorderColor = 34;
    private const int DwmwaCaptionColor = 35;
    private const int DwmwaTextColor = 36;

    [DllImport("dwmapi.dll", ExactSpelling = true)]
    private static extern int DwmSetWindowAttribute(IntPtr hwnd, int attribute, ref int value, int size);

    /// <summary>
    /// 現在のテーマ辞書の色をウィンドウのキャプションに適用する。
    /// ウィンドウのハンドルがまだ無ければ何もしない (SourceInitialized 以降に呼ぶ)。
    /// </summary>
    public static void Apply(Window window)
    {
        IntPtr hwnd = new WindowInteropHelper(window).Handle;
        if (hwnd == IntPtr.Zero)
        {
            return;
        }

        Color background = GetColor(window, "Nox.Brush.WindowBackground", Colors.Black);
        int darkMode = IsDark(background) ? 1 : 0;
        _ = DwmSetWindowAttribute(hwnd, DwmwaUseImmersiveDarkMode, ref darkMode, sizeof(int));

        SetColor(hwnd, DwmwaCaptionColor, background);
        SetColor(hwnd, DwmwaTextColor, GetColor(window, "Nox.Brush.TextPrimary", Colors.White));
        SetColor(hwnd, DwmwaBorderColor, GetColor(window, "Nox.Brush.GridLine", background));
    }

    private static Color GetColor(Window window, string brushKey, Color fallback)
        => window.TryFindResource(brushKey) is SolidColorBrush brush ? brush.Color : fallback;

    private static void SetColor(IntPtr hwnd, int attribute, Color color)
    {
        // COLORREF は 0x00BBGGRR
        int colorRef = color.R | (color.G << 8) | (color.B << 16);
        _ = DwmSetWindowAttribute(hwnd, attribute, ref colorRef, sizeof(int));
    }

    private static bool IsDark(Color color)
        => (color.R * 299 + color.G * 587 + color.B * 114) / 1000 < 128;
}
