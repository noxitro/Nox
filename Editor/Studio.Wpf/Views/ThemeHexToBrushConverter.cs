// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;
using System.Globalization;
using System.Windows.Data;
using System.Windows.Media;

namespace Studio.Wpf.Views;

public sealed class ThemeHexToBrushConverter : IValueConverter
{
    public object? Convert(object value, Type targetType, object parameter, CultureInfo culture)
    {
        if (value is not string hexValue || TryParseHex(hexValue, out Color color) == false)
        {
            return null;
        }

        SolidColorBrush brush = new(color);
        brush.Freeze();
        return brush;
    }

    public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        => throw new NotSupportedException();

    private static bool TryParseHex(string hexValue, out Color color)
    {
        color = default;
        if (string.IsNullOrWhiteSpace(hexValue))
        {
            return false;
        }

        string normalized = hexValue.Trim();
        if (normalized.StartsWith('#'))
        {
            normalized = normalized[1..];
        }

        if (normalized.Length == 6)
        {
            color = Color.FromArgb(
                0xFF,
                System.Convert.ToByte(normalized[0..2], 16),
                System.Convert.ToByte(normalized[2..4], 16),
                System.Convert.ToByte(normalized[4..6], 16));
            return true;
        }

        if (normalized.Length == 8)
        {
            color = Color.FromArgb(
                System.Convert.ToByte(normalized[0..2], 16),
                System.Convert.ToByte(normalized[2..4], 16),
                System.Convert.ToByte(normalized[4..6], 16),
                System.Convert.ToByte(normalized[6..8], 16));
            return true;
        }

        return false;
    }
}
