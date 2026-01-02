using System;
using System.Collections.Generic;
using System.Text;

namespace Core.RuntimeAttributes
{
	public abstract class RuntimeAttribute : System.Attribute
	{
		
	}

	public class ResourcePathAttribute : RuntimeAttribute
	{
		public required string Path { get; init; }

		public ResourcePathAttribute() { }
	}
}
