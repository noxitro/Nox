
namespace Nox
{
	file interface Point2D<T> where T : struct, System.Numerics.INumber<T>
	{
		public T x { get; set; }
		public T y { get; set; }

	}

	public struct Float2 : Point2D<float>
	{
		public float x { readonly get; set; }
		public float y { readonly get; set; }
	}

	public struct Float2Legacy
	{
	}
}
