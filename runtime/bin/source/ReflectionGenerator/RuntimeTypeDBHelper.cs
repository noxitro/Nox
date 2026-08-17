using MessagePack.Resolvers;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ReflectionGenerator;

	internal static class RuntimeTypeDBHelper
	{
		public static void Serialize(
			Nox.CustomTask.Data customTaskData,
			IReadOnlyList<Parser2.NamespaceDecl> namespaceDeclList
			)
		{
			RuntimeTypeDB.TypeDB typeDB = new RuntimeTypeDB.TypeDB();
			typeDB.NamespaceList = CreateNamespaceDeclList(namespaceDeclList);
			RuntimeTypeDB.Util.Serialize(typeDB, customTaskData.Platform, customTaskData.Configuration);
		}

		private static RuntimeTypeDB.AttributeDecl[] CreateAttributeList(ReadOnlySpan<Parser2.AttributeDecl> sourceSpan)
		{
			int sourceLength = sourceSpan.Length;
			if (sourceLength <= 0)
			{
				return [];
			}

			RuntimeTypeDB.AttributeDecl[] declList = new RuntimeTypeDB.AttributeDecl[sourceLength];
			for (int i = 0; i < sourceLength; ++i)
			{
				ref readonly Parser2.AttributeDecl src = ref sourceSpan[i];
				declList[i] = new RuntimeTypeDB.AttributeDecl
				{
					AttrName = src.AttrName,
					Kind = (RuntimeTypeDB.AttributeKind)src.AttrKind,
					Value = src.Value,
				};
			}
			return declList;
		}

		private static RuntimeTypeDB.NamespaceDecl[] CreateNamespaceDeclList(IReadOnlyList<Parser2.NamespaceDecl> sourceList)
		{
			int sourceLength = sourceList.Count;
			if(sourceLength <= 0)
			{
				return [];
			}

			RuntimeTypeDB.NamespaceDecl[] declList = new RuntimeTypeDB.NamespaceDecl[sourceLength];
			for (int i = 0; i < sourceLength; i++)
			{
				Parser2.NamespaceDecl source = sourceList[i];
				declList[i] = new RuntimeTypeDB.NamespaceDecl
				{
					Name = source.Name,
					FullName = source.FullName,
					AccessLevel = (RuntimeTypeDB.AccessLevel)source.AccessLevel,
					AttributeList = CreateAttributeList(source.AttributeSpan),
					NamespaceList = CreateNamespaceDeclList(source.NamespaceList),
					RecordList = CreateRecordDeclList(source.RecordList),
					FunctionList = CreateFunctionDeclList(source.FunctionList),
					VariableList = CreateVariableDeclList(source.VariableList),
					EnumList = CreateEnumDeclList(source.EnumList),
				};
			}

			return declList;
		}

		private static RuntimeTypeDB.EnumDecl[] CreateEnumDeclList(IReadOnlyList<Parser2.EnumDecl> sourceList)
		{
			int count = sourceList.Count;
			if (count == 0)
			{
				return [];
			}

			RuntimeTypeDB.EnumDecl[] declList = new RuntimeTypeDB.EnumDecl[count];
			for (int i = 0; i < count; i++)
			{
				Parser2.EnumDecl source = sourceList[i];

				ReadOnlySpan<Parser2.EnumDecl.EnumeratorInfo> sourceEnumeratorInfoList = source.EnumeratorSpan;
				int sourceEnumeratorInfoLength = sourceEnumeratorInfoList.Length;
				RuntimeTypeDB.EnumDecl.EnumeratorInfo[] enumeratorInfoList = new RuntimeTypeDB.EnumDecl.EnumeratorInfo[sourceEnumeratorInfoLength];
				for (int enumeratorInfoIndex = 0; enumeratorInfoIndex < sourceEnumeratorInfoLength; ++enumeratorInfoIndex)
				{
					ref readonly Parser2.EnumDecl.EnumeratorInfo sourceEnumeratorInfo = ref sourceEnumeratorInfoList[enumeratorInfoIndex];

					enumeratorInfoList[enumeratorInfoIndex] = new RuntimeTypeDB.EnumDecl.EnumeratorInfo()
					{
						AttributeList = CreateAttributeList(sourceEnumeratorInfo.AttributeList),
						Int64 = sourceEnumeratorInfo.Int64,
						IsUnsigned = sourceEnumeratorInfo.IsUnsigned,
						Name = sourceEnumeratorInfo.Name,
					};
				}

				declList[i] = new RuntimeTypeDB.EnumDecl
				{
					TypeInfo = CreateTypeInfo(source.TypeInfo),
					EnumeratorInfoList = enumeratorInfoList,
					FixedUnderlyingType = source.FixedUnderlyingType,
					Name = source.Name,
					FullName = source.FullName,
					AccessLevel = (RuntimeTypeDB.AccessLevel)source.AccessLevel,
					Namespace = source.Namespace,
					AttributeList = CreateAttributeList(source.AttributeSpan),
				};
			}
			return declList;
		}

		private static RuntimeTypeDB.RecordDecl[] CreateRecordDeclList(IReadOnlyList<Parser2.RecordDecl> sourceList)
		{
			int count = sourceList.Count;
			if (count == 0)
			{
				return [];
			}

			var declList = new RuntimeTypeDB.RecordDecl[count];
			for (int i = 0; i < count; ++i)
			{
				Parser2.RecordDecl source = sourceList[i];

				var recordType = new RuntimeTypeDB.TypeInfo
				{
					Kind = RuntimeTypeDB.RuntimeTypeKind.Class,
					Name = source.Name,
					FullName = source.FullName,
					Namespace = source.Namespace,
				};

				declList[i] = new RuntimeTypeDB.RecordDecl
				{
					Name = source.Name,
					FullName = source.FullName,
					Namespace = source.Namespace,
					AccessLevel = (RuntimeTypeDB.AccessLevel)source.AccessLevel,
					AttributeList = CreateAttributeList(source.AttributeSpan),
					TypeInfo = recordType,
					RecordList = CreateRecordDeclList(source.RecordList),
					FunctionList = CreateFunctionDeclList(source.FunctionList),
					VariableList = CreateVariableDeclList(source.VariableList),
					EnumList = CreateEnumDeclList(source.EnumList),
				};
			}

			return declList;
		}

		private static RuntimeTypeDB.FunctionDecl[] CreateFunctionDeclList(IReadOnlyList<Parser2.FunctionDecl> sourceList)
		{
			int count = sourceList.Count;
			if (count == 0)
			{
				return [];
			}

			var declList = new RuntimeTypeDB.FunctionDecl[count];
			for (int i = 0; i < count; i++)
			{
				Parser2.FunctionDecl source = sourceList[i];
				ReadOnlySpan<Parser2.FunctionDecl.ArgumentInfo> sourceArgList = source.ArgumentSpan;
				int sourceArgLength = source.ArgumentSpan.Length;
				RuntimeTypeDB.FunctionDecl.ArgumentInfo[] argList = new RuntimeTypeDB.FunctionDecl.ArgumentInfo[sourceArgLength];
				for(int argIndex = 0; argIndex < sourceArgLength; ++argIndex)
				{
					ref readonly Parser2.FunctionDecl.ArgumentInfo sourceArg = ref sourceArgList[argIndex];
					argList[argIndex] = new RuntimeTypeDB.FunctionDecl.ArgumentInfo
					{
						Name = sourceArg.Name,
						IsDefault = sourceArg.IsDefault,
						TypeInfo = CreateTypeInfo(sourceArg.TypeInfo),
						AttributeList = CreateAttributeList(sourceArg.AttributeList),
					};
				}

				declList[i] = new RuntimeTypeDB.FunctionDecl
				{
					Name = source.Name,
					FullName = source.FullName,
					Namespace = source.Namespace,
					AccessLevel = (RuntimeTypeDB.AccessLevel)source.AccessLevel,
					AttributeList = CreateAttributeList(source.AttributeSpan),
					TypeInfo = CreateTypeInfo(source.TypeInfo),
					FunctionAttributeFlags = (RuntimeTypeDB.FunctionAttributeFlag)(ushort)source.FunctionAttributeFlags,
					ArgumentList = argList,
					NumDefaultArgument = source.NumDefaultArgument,
				};
			}
			return declList;
		}

		private static RuntimeTypeDB.VariableDecl[] CreateVariableDeclList(IReadOnlyList<Parser2.VariableDecl> sourceList)
		{
			int count = sourceList.Count;
			if (count == 0) return [];

			var declList = new RuntimeTypeDB.VariableDecl[count];
			for (int i = 0; i < count; i++)
			{
				Parser2.VariableDecl src = sourceList[i];
				declList[i] = new RuntimeTypeDB.VariableDecl
				{
					Name = src.Name,
					FullName = src.FullName,
					Namespace = src.Namespace,
					AccessLevel = (RuntimeTypeDB.AccessLevel)src.AccessLevel,
					AttributeList = CreateAttributeList(src.AttributeSpan),
					Type = CreateTypeInfo(src.Type),
					VariableAttributeFlags = (RuntimeTypeDB.VariableAttributeFlag)(ushort)src.VariableAttributeFlags,
					OffsetBits = src.OffsetBits,
					BitFieldWidth = src.BitFieldWidth,
				};
			}
			return declList;
		}

		private static RuntimeTypeDB.TypeInfo CreateTypeInfo(Parser2.TypeInfo src)
		{
			switch (src)
			{
				case Parser2.FunctionTypeInfo f:
					{
						int argc = f.ArgumentTypeList.Length;
						var args = argc == 0 ? Array.Empty<RuntimeTypeDB.TypeInfo>() : new RuntimeTypeDB.TypeInfo[argc];
						for (int i = 0; i < argc; i++)
						{
							args[i] = CreateTypeInfo(f.ArgumentTypeList[i]);
						}

						return new RuntimeTypeDB.TypeInfo
						{
							// 必要なら RuntimeTypeKind に Function を追加してここで設定
							Kind = RuntimeTypeDB.RuntimeTypeKind.Invalid,
							Name = f.Name,
							FullName = f.FullName,
							Namespace = f.Namespace,
							ArgumentTypeList = args,
							ReturnType = CreateTypeInfo(f.ReturnType),
							Size = f.Size,
							Alignment = f.Alignment,
						};
					}

				case Parser2.PointerTypeInfo impl:
					return new RuntimeTypeDB.TypeInfo
					{
						Size = impl.Size,
						Alignment = impl.Alignment,
						Kind = RuntimeTypeDB.RuntimeTypeKind.Pointer,
						Name = impl.Name,
						FullName = impl.FullName,
						Namespace = impl.Namespace,
					};

				case Parser2.EnumTypeInfo impl:
					return new RuntimeTypeDB.TypeInfo
					{
						Size = impl.Size,
						Alignment = impl.Alignment,
						Kind = RuntimeTypeDB.RuntimeTypeKind.Enum,
						Name = impl.Name,
						FullName = impl.FullName,
						Namespace = impl.Namespace,
						UnderlyingTypeInfo = CreateTypeInfo(impl.UnderlyingTypeInfo),
					};

				default:

					// それ以外は素直にマップ
					return new RuntimeTypeDB.TypeInfo
					{
						Kind = (RuntimeTypeDB.RuntimeTypeKind)src.TypeKind,
						Name = src.Name,
						FullName = src.FullName,
						Namespace = src.Namespace,
						Size = src.Size,
						Alignment = src.Alignment,

					};
			}
		}
	}
