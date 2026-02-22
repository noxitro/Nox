using System;
using System.Collections.Generic;
using System.Text;

namespace Core.Attributes
{
	/// <summary>
	/// Core.RuntimeObject継承クラスに付与する属性
	/// runtimeとの型マッピング用
	/// </summary>
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
