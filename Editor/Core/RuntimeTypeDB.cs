using Core.RuntimeWrapper;
using Nox;
using System;
using System.Collections.Generic;
using System.Text;

namespace Core
{
	public static class RuntimeTypeExtensions
	{
		public static bool IsIntegral(this Core.RuntimeTypeKind self)
		{
			return self switch
			{
				Core.RuntimeTypeKind.Bool or
				Core.RuntimeTypeKind.Char or
				Core.RuntimeTypeKind.SignedChar or
				Core.RuntimeTypeKind.UnsignedChar or
				Core.RuntimeTypeKind.Char8 or
				Core.RuntimeTypeKind.Char16 or
				Core.RuntimeTypeKind.Char32 or
				Core.RuntimeTypeKind.WChar16 or
				Core.RuntimeTypeKind.Int8 or
				Core.RuntimeTypeKind.UInt8 or
				Core.RuntimeTypeKind.Int16 or
				Core.RuntimeTypeKind.Uint16 or
				Core.RuntimeTypeKind.Int32 or
				Core.RuntimeTypeKind.UInt32 or
				Core.RuntimeTypeKind.Int64 or
				Core.RuntimeTypeKind.UInt64 or
				Core.RuntimeTypeKind.Long or
				Core.RuntimeTypeKind.UnsignedLong
				=> true,
				_ => false,
			};
		}

		public static bool IsFloatingPoint(this Core.RuntimeTypeKind self)
		{
			return self switch
			{
				Core.RuntimeTypeKind.Float or
				Core.RuntimeTypeKind.Double
				=> true,
				_ => false,
			};
		}
	}

	file sealed class UnknownRuntimeAttribute : System.Attribute
	{
	}

	file static class Local
	{
		private static readonly System.Attribute _UnknownRuntimeAttribute = new UnknownRuntimeAttribute();

		public static Core.RuntimeTypeKind Convert(ReflectionGenerator.RuntimeTypeDB.RuntimeTypeKind value)
		{
			return (Core.RuntimeTypeKind)(byte)value;
		}

		public static System.Attribute CreateAttribute(in ReflectionGenerator.RuntimeTypeDB.AttributeDecl attributeDecl)
		{
			switch(attributeDecl.Kind)
			{
				case ReflectionGenerator.RuntimeTypeDB.AttributeKind.Annotate:
					ReadOnlySpan<char> annotate = attributeDecl.Value;
					
					switch (annotate)
					{
						case "nox::attr::DataMember()":
							return new System.Runtime.Serialization.DataMemberAttribute();
						case "nox::attr::IgnoreDataMember()":
							return new System.Runtime.Serialization.IgnoreDataMemberAttribute();
					}
					break;
			}

			return _UnknownRuntimeAttribute;
		}
	}

	public class RuntimeTypeDB
	{
		#region 非公開フィールド
		/// <summary>
		/// root namespace decl list
		/// </summary>
		private Core.RuntimeNamespaceDecl[] _NamespaceDeclList = [];

	//	private readonly Dictionary<string, DeclBase> _DeclDictWithFQN = new();
		private readonly Dictionary<string, RuntimeTypeInfo> _TypeInfoDictWithFQN = new();

		private readonly List<RuntimeRecordDecl> _RecordDeclList = new();
		private readonly Dictionary<string, RuntimeRecordDecl> _RecordDeclDictWithFQN = new();
		#endregion

		#region 公開メソッド
		public void Build(PlatformType platform, ConfigurationType configuration)
		{
			ReflectionGenerator.RuntimeTypeDB.TypeDB? typeDB;
			using (new Nox.ScopeProfiler<Core.LogId.Runtime>("RuntimeTypeDB Deserialize"))
			{
				typeDB = ReflectionGenerator.RuntimeTypeDB.Util.Deserialize(platform.GetName(), configuration.GetName());
			}

			if (typeDB == null)
			{
				Nox.LogTrace.ErrorLine<Core.LogId.Runtime>("RuntimeTypeDBの生成に失敗");
			}
			Nox.Util.Assert(typeDB != null, "RuntimeTypeDBの生成に失敗");

			using (new Nox.ScopeProfiler<Core.LogId.Runtime>("RuntimeTypeDB CreateNamespaceDeclList"))
			{
				_NamespaceDeclList = CreateNamespaceDeclList(typeDB.NamespaceList);
			}
		}

		public static Core.RuntimeRecordDecl? GetRuntimeRecordDecl<T>() where T : Core.RuntimeObject, IRuntimeObject<T>
		{
			return IRuntimeObject<T>.RuntimeRecordDecl;
		}

		public Core.RuntimeTypeInfo? FindType(ReadOnlySpan<char> fqn)
		{
			if (_TypeInfoDictWithFQN.TryGetValue(fqn.ToString(), out var typeInfo) == true)
			{
				return typeInfo;
			}
			return null;
		}

		public Core.RuntimeRecordDecl? FindRecordDecl(ReadOnlySpan<char> fqn)
		{
			if (_RecordDeclDictWithFQN.TryGetValue(fqn.ToString(), out var typeInfo) == true)
			{
				return typeInfo;
			}
			return null;
		}

		//public Core.DeclBase? FindDecl(ReadOnlySpan<char> fqn)
		//{
		//	if (_DeclDictWithFQN.TryGetValue(fqn.ToString(), out var decl))
		//	{
		//		return decl;
		//	}
		//	return null;
		//}

		//public T? FindDecl<T>(ReadOnlySpan<char> fqn) where T : Core.DeclBase
		//{
		//	return FindDecl(fqn) as T;
		//}
		#endregion

		#region 非公開メソッド

		private RuntimeNamespaceDecl[] CreateNamespaceDeclList(ReadOnlySpan<ReflectionGenerator.RuntimeTypeDB.NamespaceDecl> sourceList)
		{
			RuntimeNamespaceDecl[] declList = new RuntimeNamespaceDecl[sourceList.Length];
			for (int i = 0; i < sourceList.Length; i++)
			{
				var source = sourceList[i];
				declList[i] = new RuntimeNamespaceDecl()
				{
					FullName = source.FullName,
					Name = source.Name,
					AttributeList = CreateAttributeList(source.AttributeList),
					RecordList = CreateRecordDeclList(source.RecordList),
					EnumList = CreateEnumDeclList(source.EnumList),
					FieldList = CreateVariableDeclList(source.VariableList),
					MethodList = CreateFunctionDeclList(source.FunctionList),
					NamespaceList = CreateNamespaceDeclList(source.NamespaceList)
				};

				AddDeclWithFQN(declList[i]);
			}
			return declList;
		}

		private RuntimeRecordDecl[] CreateRecordDeclList(ReadOnlySpan<ReflectionGenerator.RuntimeTypeDB.RecordDecl> sourceList)
		{
			RuntimeRecordDecl[] declList = new RuntimeRecordDecl[sourceList.Length];
			for (int i = 0; i < sourceList.Length; i++)
			{
				var source = sourceList[i];

				var typeInfo = GetCreateRuntimeTypeInfo(source.TypeInfo, source);
				declList[i] = new RuntimeRecordDecl()
				{
					Name = source.Name,
					FullName = source.FullName,
					Namespace = source.Namespace,
					AttributeList = CreateAttributeList(source.AttributeList),
					RecordList = CreateRecordDeclList(source.RecordList),
					EnumList = CreateEnumDeclList(source.EnumList),
					VariableList = CreateVariableDeclList(source.VariableList),
					FunctionList = CreateFunctionDeclList(source.FunctionList),
					//IsNoxObject = source.IsNoxObject,
					//IsReflectionClass = source.IsReflectionClass,
					TypeInfo = typeInfo,
				};
				typeInfo.Decl = declList[i];


				AddDeclWithFQN(declList[i]);
			}
			return declList;
		}

		public RuntimeEnumDecl[] CreateEnumDeclList(ReadOnlySpan<ReflectionGenerator.RuntimeTypeDB.EnumDecl> sourceList)
		{
			RuntimeEnumDecl[] declList = new RuntimeEnumDecl[sourceList.Length];
			for (int i = 0; i < sourceList.Length; i++)
			{
				var source = sourceList[i];
				ReadOnlySpan<ReflectionGenerator.RuntimeTypeDB.EnumDecl.EnumeratorInfo> sourceEnumeratorList = source.EnumeratorInfoList;
				RuntimeEnumDecl.EnumeratorInfo[] enumeratorInfoList = new RuntimeEnumDecl.EnumeratorInfo[sourceEnumeratorList.Length];
				for(int enumeratorInfoIndex = 0; enumeratorInfoIndex < sourceEnumeratorList.Length; enumeratorInfoIndex++)
				{
					var sourceEnumeratorInfo = sourceEnumeratorList[enumeratorInfoIndex];
					enumeratorInfoList[enumeratorInfoIndex] = new RuntimeEnumDecl.EnumeratorInfo()
					{
						Name = sourceEnumeratorInfo.Name,
						IsUnsigned = sourceEnumeratorInfo.IsUnsigned,
						Int64 = sourceEnumeratorInfo.Int64,
						AttributeList = CreateAttributeList(sourceEnumeratorInfo.AttributeList)
					};
				}

				declList[i] = new RuntimeEnumDecl()
				{
					FullName = source.FullName,
					Name = source.Name,
					Namespace = source.Namespace,
					FixedUnderlyingType = source.FixedUnderlyingType,
					AttributeList = CreateAttributeList(source.AttributeList),
					EnumeratorInfoList = enumeratorInfoList, 
					TypeInfo = GetCreateRuntimeTypeInfo(source.TypeInfo, source),
				};

				AddDeclWithFQN(declList[i]);
			}
			return declList;
		}

		public RuntimeVariableDecl[] CreateVariableDeclList(ReadOnlySpan<ReflectionGenerator.RuntimeTypeDB.VariableDecl> sourceList)
		{
			RuntimeVariableDecl[] declList = new RuntimeVariableDecl[sourceList.Length];
			for (int i = 0; i < sourceList.Length; i++)
			{
				var source = sourceList[i];
				declList[i] = new RuntimeVariableDecl()
				{
					FullName = source.FullName,
					Name = source.Name,
					Namespace = source.Namespace,
					AttributeList = CreateAttributeList(source.AttributeList),
					TypeInfo = GetCreateRuntimeTypeInfo(source.Type),
					VariableAttributeFlags = (RuntimeVariableAttributeFlag)(byte)source.VariableAttributeFlags,
					OffsetBits = source.OffsetBits,
					BitFieldWidth = source.BitFieldWidth
				};

				AddDeclWithFQN(declList[i]);
			}
			return declList;
		}

		public RuntimeFunctionDecl[] CreateFunctionDeclList(ReadOnlySpan<ReflectionGenerator.RuntimeTypeDB.FunctionDecl> sourceList)
		{
			RuntimeFunctionDecl[] declList = new RuntimeFunctionDecl[sourceList.Length];
			for (int i = 0; i < sourceList.Length; i++)
			{
				var source = sourceList[i];
				var sourceArgList = source.ArgumentList;
				int sourceArgLength = sourceArgList.Length;
				var argList = new RuntimeFunctionDecl.ArgumentInfo[sourceArgLength];
				for (int argIndex = 0; argIndex < sourceArgLength; ++argIndex)
				{
					ref readonly var sourceArg = ref sourceArgList[argIndex];
					argList[argIndex] = new RuntimeFunctionDecl.ArgumentInfo
					{
						Name = sourceArg.Name,
						IsDefault = sourceArg.IsDefault,
						TypeInfo = GetCreateRuntimeTypeInfo(sourceArg.TypeInfo),
						AttributeList = CreateAttributeList(sourceArg.AttributeList),
					};
				}

				declList[i] = new RuntimeFunctionDecl()
				{
					Name = source.Name,
					FullName = source.FullName,
					Namespace = source.Namespace,
					AttributeList = CreateAttributeList(source.AttributeList),
					TypeInfo = GetCreateRuntimeTypeInfo(source.TypeInfo),
					FunctionAttributeFlags = (RuntimeFunctionAttributeFlag)(byte)source.FunctionAttributeFlags,
					ArgumentList = argList,
					NumDefaultArgument = source.NumDefaultArgument
				};

				AddDeclWithFQN(declList[i]);
			}
			return declList;
		}

		private System.Attribute[] CreateAttributeList(ReadOnlySpan<ReflectionGenerator.RuntimeTypeDB.AttributeDecl> sourceList)
		{
			System.Attribute[] attributeList = new System.Attribute[sourceList.Length];
			for (int i = 0; i < sourceList.Length; i++)
			{
				attributeList[i] = Local.CreateAttribute(sourceList[i]);
			}
			return attributeList;
		}


		private RuntimeTypeInfo GetCreateRuntimeTypeInfo(ReflectionGenerator.RuntimeTypeDB.TypeInfo source, ReflectionGenerator.RuntimeTypeDB.DeclBase? declaration = null)
		{
			{
				if (_TypeInfoDictWithFQN.TryGetValue(source.FullName, out RuntimeTypeInfo? outValue) == true)
				{
					return outValue;
				}
			}

			RuntimeTypeInfo underlyingTypeInfo;
			if (source.Kind == ReflectionGenerator.RuntimeTypeDB.RuntimeTypeKind.Enum)
			{
				if (source.UnderlyingTypeInfo.Kind != ReflectionGenerator.RuntimeTypeDB.RuntimeTypeKind.Invalid)
				{
					underlyingTypeInfo = GetCreateRuntimeTypeInfo(source.UnderlyingTypeInfo);
				}
				else
				{
					underlyingTypeInfo = RuntimeTypeInfo.Invalid;
				}
			}
			else
			{
				underlyingTypeInfo = RuntimeTypeInfo.Invalid;
			}

			RuntimeTypeInfo pointeeTypeInfo;
			if (source.Kind == ReflectionGenerator.RuntimeTypeDB.RuntimeTypeKind.Pointer ||
				source.Kind == ReflectionGenerator.RuntimeTypeDB.RuntimeTypeKind.LValueReference ||
				source.Kind == ReflectionGenerator.RuntimeTypeDB.RuntimeTypeKind.RValueReference)
			{
				pointeeTypeInfo = GetCreateRuntimeTypeInfo(source.PointeeTypeInfo);
			}
			else
			{
				pointeeTypeInfo = RuntimeTypeInfo.Invalid;
			}

			RuntimeTypeInfo runtimeTypeInfo = new()
			{
				TypeKind = Local.Convert(source.Kind),
				Size = source.Size,
				Alignment = source.Alignment,
				Name = source.Name,
				FullName = source.FullName,
				Namespace = source.Namespace,
				UnderlyingTypeInfo = underlyingTypeInfo,
				PointeeTypeInfo = pointeeTypeInfo,
			};

//			Nox.LogTrace.InfoLine<Core.LogId.Runtime>("type:{0}", runtimeTypeInfo.FullName);

			Nox.Util.Assert(_TypeInfoDictWithFQN.ContainsKey(runtimeTypeInfo.FullName) == false, "同じFQNのRuntimeTypeInfoが既に登録されています。FQN={0}", runtimeTypeInfo.FullName);
			_TypeInfoDictWithFQN[runtimeTypeInfo.FullName] = runtimeTypeInfo;

			return runtimeTypeInfo;
		}

		private void AddDeclWithFQN(NamedDecl decl)
		{
		//	Nox.Util.Assert(_DeclDictWithFQN.ContainsKey(decl.FullName) == false, "同じFQNのDeclが既に登録されています。FQN={0}", decl.FullName);
		//	_DeclDictWithFQN[decl.FullName] = decl;
		}
		#endregion
	}
}
