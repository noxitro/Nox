using System;
using System.Collections.Generic;
using System.Text;

namespace Core
{
	public static class TypeDB
	{
		private static System.Type[] CollectTypeList()
		{
			var asmList = AppDomain.CurrentDomain.GetAssemblies();
			var typeList = new List<System.Type>();
			foreach (var asm in asmList)
			{
				try
				{
					var types = asm.GetTypes();
					typeList.AddRange(types);
				}
				catch (System.Reflection.ReflectionTypeLoadException ex)
				{
				}
			}

			return typeList.ToArray();
		}

		private static readonly System.Type[] _AllTypeList = CollectTypeList();
		public static ReadOnlySpan<System.Type> AllTypeList => _AllTypeList;
	}
}
