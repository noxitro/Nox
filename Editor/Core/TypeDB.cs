// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;
using System.Collections.Generic;
using System.Text;

namespace Core;

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

		public static List<System.Type> GetSubClassList<T>() where T : class
		{
			var baseType = typeof(T);
			var result = new List<System.Type>();
			foreach (var type in AllTypeList)
			{
				if (type.IsAbstract || type.IsInterface)
				{
					continue;
				}

				if (type.IsSubclassOf(baseType))
				{
					result.Add(type);
				}
			}

			return result;
		}

		private static readonly System.Type[] _AllTypeList = CollectTypeList();
		public static ReadOnlySpan<System.Type> AllTypeList => _AllTypeList;
	}
