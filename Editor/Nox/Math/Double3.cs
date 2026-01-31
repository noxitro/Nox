using System;
using System.Collections.Generic;
using System.Text;

namespace Nox.Math
{
	public struct Double3
	{
		public double x;
		public double y;
		public double z;

		public readonly Double2 xy => new Double2 { x = x, y = y };

		public readonly override string ToString()
		{
			return $"({x}, {y}, {z})";
		}
	}
}
