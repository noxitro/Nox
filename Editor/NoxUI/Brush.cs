using System;
using System.Collections.Generic;
using System.Text;

namespace NoxUI;

	public static class Brush
	{
		public static readonly System.Windows.Media.SolidColorBrush WindowBackground = Create(30, 30, 30);


		private static System.Windows.Media.SolidColorBrush Create(byte r, byte g, byte b)
		{
			System.Windows.Media.SolidColorBrush brush = new System.Windows.Media.SolidColorBrush(System.Windows.Media.Color.FromRgb(r, g, b));	
			brush.Freeze();
			return brush;
		}
	}
