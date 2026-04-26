using System;
using System.Collections.Generic;
using System.Text;

namespace Nox
{
    public struct Position
    {
        public double x;
        public double y;
        public double z;

        public Position(System.Numerics.Vector3 other)
        {
            x = other.X;
            y = other.Y;
            z = other.Z;
        }

        public static explicit operator System.Numerics.Vector3(Position other)
        {
            return new System.Numerics.Vector3(
                (float)other.x,
                (float)other.y,
                (float)other.z
            );
        }
    }
}
