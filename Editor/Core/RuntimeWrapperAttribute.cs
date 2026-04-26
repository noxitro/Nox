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
		/// <summary>
		/// RuntimeFqn
		/// </summary>
		public string RuntimeFQN { get; init; }

		public RuntimeWrapperAttribute(string fqn)
		{
			RuntimeFQN = fqn;
		}
	}
}
