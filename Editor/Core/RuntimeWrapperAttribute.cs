using System;
using System.Collections.Generic;
using System.Text;

namespace Core.Attributes
{
	[System.AttributeUsage(AttributeTargets.Class)]
	public class RuntimeWrapperAttribute : System.Attribute
	{
		public string FQN { get; init; }

		public RuntimeWrapperAttribute(string fqn)
		{
			FQN = fqn;
		}
	}
}
