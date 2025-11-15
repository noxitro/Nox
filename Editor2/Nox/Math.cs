
namespace Nox
{
	file interface Point2D<T> where T : struct, System.Numerics.INumber<T>
	{

	}

	public struct Float2 : Point2D<float>
	{
		public float x;
		public float y;
	}
}
