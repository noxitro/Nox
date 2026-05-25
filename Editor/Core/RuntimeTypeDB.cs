using Nox;
using Nox.Extensions;
using System;
using System.Collections.Generic;
using System.Reflection;
using Core.RuntimeAttributes;
using System.Reflection.Metadata.Ecma335;
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

		public static Core.RuntimeTypeKind Convert(ReflectionGenerator.RuntimeTypeDB.RuntimeTypeKind value)
		{
			return (Core.RuntimeTypeKind)(byte)value;
		}
		/// <summary>
		/// annotate文字列を型名(FQN)と引数リストに分解する。
		/// 例: "nox::attr::Resource(u8"ext", 1)" → ("nox::attr::Resource", ["ext", 1])
		/// </summary>
		public static ReadOnlySpan<char> ParseAnnotate(ReadOnlySpan<char> annotate, out object[] outArgs)
		{
			int parenStart = annotate.IndexOf('(');
			if (parenStart < 0 || annotate[^1] != ')')
			{
				outArgs = [];
				return annotate;
			}

			ReadOnlySpan<char> fqn = annotate[..parenStart];
			// C++のグローバルスコープ修飾子 (先頭 "::") を除去
			if (fqn.StartsWith("::"))
			{
				fqn = fqn[2..];
			}

			ReadOnlySpan<char> body = annotate[(parenStart + 1)..^1];

			if (body.IsEmpty || body.IsWhiteSpace())
			{
				outArgs = []	;
				return fqn;
			}

			List<object> args = [];
			int pos = 0;

			while (pos < body.Length)
			{
				SkipWhiteSpace(body, ref pos);
				if (pos >= body.Length) break;

				if (TryParseStringLiteral(body, ref pos, out string? strValue))
				{
					args.Add(strValue);
				}
				else
				{
					int start = pos;
					while (pos < body.Length && body[pos] != ',') pos++;
					ReadOnlySpan<char> token = body[start..pos].Trim();
					args.Add(ParseLiteral(token));
				}

				SkipWhiteSpace(body, ref pos);
				if (pos < body.Length && body[pos] == ',') pos++;
			}

			outArgs = args.ToArray();
			return fqn;
		}

		private static void SkipWhiteSpace(ReadOnlySpan<char> span, ref int pos)
		{
			while (pos < span.Length && char.IsWhiteSpace(span[pos])) pos++;
		}

		/// <summary>
		/// C++文字列リテラル（u8"...", u"...", U"...", L"...", "..."）を解析する。
		/// </summary>
		private static bool TryParseStringLiteral(ReadOnlySpan<char> span, ref int pos, out string value)
		{
			int saved = pos;

			// C++文字列プレフィックスをスキップ: u8, u, U, L
			if (pos < span.Length)
			{
				if (span[pos] == 'u')
				{
					pos++;
					if (pos < span.Length && span[pos] == '8') pos++;
				}
				else if (span[pos] is 'U' or 'L')
				{
					pos++;
				}
			}

			if (pos >= span.Length || span[pos] != '"')
			{
				pos = saved;
				value = string.Empty;
				return false;
			}

			pos++; // 開き " をスキップ
			var sb = new StringBuilder();
			while (pos < span.Length && span[pos] != '"')
			{
				if (span[pos] == '\\' && pos + 1 < span.Length)
				{
					sb.Append(span[pos + 1] switch
					{
						'n' => '\n',
						't' => '\t',
						'r' => '\r',
						'\\' => '\\',
						'"' => '"',
						'0' => '\0',
						char c => c,
					});
					pos += 2;
				}
				else
				{
					sb.Append(span[pos]);
					pos++;
				}
			}

			if (pos < span.Length) pos++; // 閉じ " をスキップ
			value = sb.ToString();
			return true;
		}

		/// <summary>
		/// 数値・boolリテラルを解析する。C++の型サフィックス (f, u, l 等) にも対応。
		/// </summary>
		private static object ParseLiteral(ReadOnlySpan<char> token)
		{
			if (token.Equals("true", StringComparison.Ordinal)) return true;
			if (token.Equals("false", StringComparison.Ordinal)) return false;

			// C++数値サフィックスを除去 (f, F, u, U, l, L, ll, LL, ul, ull 等)
			ReadOnlySpan<char> numeric = StripNumericSuffix(token);

			if (int.TryParse(numeric, System.Globalization.NumberStyles.Integer, System.Globalization.CultureInfo.InvariantCulture, out int intVal))
				return intVal;
			if (long.TryParse(numeric, System.Globalization.NumberStyles.Integer, System.Globalization.CultureInfo.InvariantCulture, out long longVal))
				return longVal;
			if (double.TryParse(numeric, System.Globalization.NumberStyles.Float, System.Globalization.CultureInfo.InvariantCulture, out double doubleVal))
				return doubleVal;

			// フォールバック: 文字列として返す
			return token.ToString();
		}

		private static ReadOnlySpan<char> StripNumericSuffix(ReadOnlySpan<char> token)
		{
			int end = token.Length;
			while (end > 0 && token[end - 1] is 'f' or 'F' or 'u' or 'U' or 'l' or 'L')
			{
				end--;
			}
			return end > 0 ? token[..end] : token;
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

		private static readonly System.Attribute _UnknownRuntimeAttribute = new UnknownRuntimeAttribute();

		private readonly IReadOnlyDictionary<string, System.Type> _RuntimeAttributeTypeDictWithRuntimeFQN ;
		#endregion

		#region 公開メソッド
		public RuntimeTypeDB()
		{
			Dictionary<string, System.Type> dict = new();
			_RuntimeAttributeTypeDictWithRuntimeFQN = dict;

			//	runtime attributeを収集
			IReadOnlyList<System.Type> runtimeAttributeTypeList = Core.TypeDB.GetSubClassList<Core.RuntimeAttributes.RuntimeAttribute>();

			foreach (var runtimeAttributeType in runtimeAttributeTypeList)
			{
				Core.RuntimeAttributes.RuntimeAttributeAttachAttribute? attachAttr = runtimeAttributeType.GetCustomAttribute<Core.RuntimeAttributes.RuntimeAttributeAttachAttribute>();
				Nox.Util.Assert(attachAttr != null, "RuntimeAttributeを継承したクラスにはRuntimeWrapperAttributeを付与する必要があります。Type={0}", runtimeAttributeType.GetFullName());

				if (dict.TryAdd(attachAttr.FQN, runtimeAttributeType) == false)
				{
					Nox.LogTrace.ErrorLine<Core.LogId.Runtime>("同じFQNのRuntimeAttributeが既に登録されています。FQN={0}, Type1={1}, Type2={2}", attachAttr.FQN, runtimeAttributeType.FullName, _RuntimeAttributeTypeDictWithRuntimeFQN[attachAttr.FQN].FullName);
				}
			}
		}

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

		public IReadOnlyList<Core.RuntimeRecordDecl> GetRecordDeclList()
		{
			return _RecordDeclList;
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

				if (source.FullName == "nox::SceneResource")
				{
					Nox.Util.BreakPoint();
				}

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
				declList[i].PropertyList = CreatePropertyDeclList(declList[i]);
				
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
				attributeList[i] = CreateAttribute(sourceList[i]);
			}
			return attributeList;
		}

		private RuntimePropertyDecl[] CreatePropertyDeclList(RuntimeRecordDecl recordDecl)
		{
			List<RuntimePropertyDecl> propertyList = [];
			Dictionary<string, int> propertyIndexByName = new(StringComparer.Ordinal);

			foreach (RuntimeVariableDecl variableDecl in recordDecl.VariableList)
			{
				if (Core.RuntimeRemote.Util.IsRemoteVariable(variableDecl) == false ||
					HasAttribute<HideAttribute>(variableDecl.AttributeList))
				{
					continue;
				}

				AddOrReplacePropertyDecl(
					propertyList,
					propertyIndexByName,
					new RuntimePropertyDecl
					{
						Name = variableDecl.Name,
						FullName = recordDecl.FullName + "::" + variableDecl.Name,
						AttributeList = variableDecl.AttributeList,
						TypeInfo = NormalizePropertyTypeInfo(variableDecl.TypeInfo),
						VariableDecl = variableDecl,
						GetterFunctionDecl = null,
						SetterFunctionDecl = null,
					},
					preferReplacement: false);
			}

			foreach (RuntimeFunctionDecl functionDecl in recordDecl.FunctionList)
			{
				if (TryGetPropertyMetadata(functionDecl, out string propertyName, out RuntimeTypeInfo propertyType, out bool isGetter) == false)
				{
					continue;
				}

				RuntimePropertyDecl propertyDecl = new()
				{
					Name = propertyName,
					FullName = recordDecl.FullName + "::" + propertyName,
					AttributeList = functionDecl.AttributeList,
					TypeInfo = propertyType,
					VariableDecl = null,
					GetterFunctionDecl = isGetter ? functionDecl : null,
					SetterFunctionDecl = isGetter ? null : functionDecl,
				};

				if (propertyIndexByName.TryGetValue(propertyName, out int existingIndex))
				{
					RuntimePropertyDecl existing = propertyList[existingIndex];
					RuntimeTypeInfo mergedType = existing.TypeInfo;
					if (existing.GetterFunctionDecl == null && existing.SetterFunctionDecl == null)
					{
						mergedType = propertyType;
					}
					else
					{
						Nox.Util.Assert(
							existing.TypeInfo.FullName == propertyType.FullName,
							$"Property getter/setter type mismatch. Property={propertyName}, Existing={existing.TypeInfo.FullName}, New={propertyType.FullName}");
					}

					propertyList[existingIndex] = new RuntimePropertyDecl
					{
						Name = propertyName,
						FullName = existing.FullName,
						AttributeList = MergeAttributeLists(existing.AttributeList, functionDecl.AttributeList),
						TypeInfo = mergedType,
						VariableDecl = existing.GetterFunctionDecl == null && existing.SetterFunctionDecl == null ? null : existing.VariableDecl,
						GetterFunctionDecl = isGetter ? functionDecl : existing.GetterFunctionDecl,
						SetterFunctionDecl = isGetter ? existing.SetterFunctionDecl : functionDecl,
					};
					continue;
				}

				AddOrReplacePropertyDecl(propertyList, propertyIndexByName, propertyDecl, preferReplacement: true);
			}

			return propertyList.ToArray();
		}

		private static void AddOrReplacePropertyDecl(
			List<RuntimePropertyDecl> propertyList,
			Dictionary<string, int> propertyIndexByName,
			RuntimePropertyDecl propertyDecl,
			bool preferReplacement)
		{
			if (propertyIndexByName.TryGetValue(propertyDecl.Name, out int existingIndex))
			{
				if (preferReplacement)
				{
					propertyList[existingIndex] = propertyDecl;
				}
				return;
			}

			propertyIndexByName.Add(propertyDecl.Name, propertyList.Count);
			propertyList.Add(propertyDecl);
		}

		private static bool TryGetPropertyMetadata(RuntimeFunctionDecl functionDecl, out string propertyName, out RuntimeTypeInfo propertyType, out bool isGetter)
		{
			propertyName = string.Empty;
			propertyType = RuntimeTypeInfo.Invalid;
			isGetter = false;

			if ((functionDecl.FunctionAttributeFlags & RuntimeFunctionAttributeFlag.Static) != 0)
			{
				return false;
			}

			string? explicitPropertyName = null;
			if (functionDecl.AttributeList.OfType<PropertyGetterAttribute>().FirstOrDefault() is PropertyGetterAttribute getterAttribute)
			{
				explicitPropertyName = getterAttribute.PropertyName;
				isGetter = true;
			}
			else if (functionDecl.AttributeList.OfType<PropertySetterAttribute>().FirstOrDefault() is PropertySetterAttribute setterAttribute)
			{
				explicitPropertyName = setterAttribute.PropertyName;
				isGetter = false;
			}
			else if (functionDecl.AttributeList.OfType<PropertyAttribute>().FirstOrDefault() is PropertyAttribute propertyAttribute)
			{
				explicitPropertyName = propertyAttribute.PropertyName;
				int argumentLength = functionDecl.GetArgumentList().Length;
				if (argumentLength == 0)
				{
					isGetter = true;
				}
				else if (argumentLength == 1)
				{
					isGetter = false;
				}
				else
				{
					return false;
				}
			}
			else
			{
				return false;
			}

			propertyName = string.IsNullOrWhiteSpace(explicitPropertyName)
				? TrimPropertyFunctionPrefix(functionDecl.Name)
				: explicitPropertyName;
			if (string.IsNullOrWhiteSpace(propertyName))
			{
				return false;
			}

			if (isGetter)
			{
				if (functionDecl.TypeInfo.ReturnTypeInfo == RuntimeTypeInfo.Invalid)
				{
					return false;
				}

				propertyType = NormalizePropertyTypeInfo(functionDecl.TypeInfo.ReturnTypeInfo);
				return propertyType != RuntimeTypeInfo.Invalid;
			}

			ReadOnlySpan<RuntimeFunctionDecl.ArgumentInfo> argumentList = functionDecl.GetArgumentList();
			if (argumentList.Length != 1)
			{
				return false;
			}

			propertyType = NormalizePropertyTypeInfo(argumentList[0].TypeInfo);
			return propertyType != RuntimeTypeInfo.Invalid;
		}

		private static RuntimeTypeInfo NormalizePropertyTypeInfo(RuntimeTypeInfo typeInfo)
		{
			RuntimeTypeInfo current = typeInfo;
			while (current.TypeKind is RuntimeTypeKind.LValueReference or RuntimeTypeKind.RValueReference or RuntimeTypeKind.Pointer)
			{
				if (current.PointeeTypeInfo == RuntimeTypeInfo.Invalid)
				{
					break;
				}

				current = current.PointeeTypeInfo;
			}

			if (current != RuntimeTypeInfo.Invalid &&
				string.IsNullOrWhiteSpace(current.FullName) == false &&
				current.TypeKind != RuntimeTypeKind.Invalid)
			{
				return current;
			}

			if (TryNormalizeKnownTypeName(typeInfo.FullName, out string normalizedTypeName) == false)
			{
				return RuntimeTypeInfo.Invalid;
			}

			return new RuntimeTypeInfo
			{
				TypeKind = RuntimeTypeKind.Class,
				Size = typeInfo.Size,
				Alignment = typeInfo.Alignment,
				Name = normalizedTypeName[(normalizedTypeName.LastIndexOf("::", StringComparison.Ordinal) + 2)..],
				FullName = normalizedTypeName,
				Namespace = normalizedTypeName.Contains("::", StringComparison.Ordinal)
					? normalizedTypeName[..normalizedTypeName.LastIndexOf("::", StringComparison.Ordinal)]
					: string.Empty,
			};
		}

		private static bool TryNormalizeKnownTypeName(string typeName, out string normalizedTypeName)
		{
			normalizedTypeName = typeName.Trim();
			if (normalizedTypeName.StartsWith("const ", StringComparison.Ordinal))
			{
				normalizedTypeName = normalizedTypeName["const ".Length..];
			}

			normalizedTypeName = normalizedTypeName.TrimEnd();
			while (normalizedTypeName.EndsWith("&", StringComparison.Ordinal) ||
				normalizedTypeName.EndsWith("*", StringComparison.Ordinal))
			{
				normalizedTypeName = normalizedTypeName[..^1].TrimEnd();
			}

			return normalizedTypeName is
				"nox::Position" or
				"nox::Vec3" or
				"nox::detail::Vector3D<float>" or
				"nox::Vec3d" or
				"nox::detail::Vector3D<double>" or
				"nox::Quat" or
				"nox::detail::Quaternion<float>";
		}

		private static string TrimPropertyFunctionPrefix(string name)
		{
			if (name.StartsWith("Get", StringComparison.Ordinal) && name.Length > 3)
			{
				return name[3..];
			}

			if (name.StartsWith("Set", StringComparison.Ordinal) && name.Length > 3)
			{
				return name[3..];
			}

			if (name.StartsWith("Is", StringComparison.Ordinal) && name.Length > 2)
			{
				return name[2..];
			}

			return name;
		}

		private static System.Attribute[] MergeAttributeLists(System.Attribute[] existingAttributes, System.Attribute[] newAttributes)
		{
			List<System.Attribute> merged = new(existingAttributes.Length + newAttributes.Length);
			HashSet<Type> seenTypes = new();
			foreach (System.Attribute attribute in existingAttributes)
			{
				if (seenTypes.Add(attribute.GetType()))
				{
					merged.Add(attribute);
				}
			}

			foreach (System.Attribute attribute in newAttributes)
			{
				if (seenTypes.Add(attribute.GetType()))
				{
					merged.Add(attribute);
				}
			}

			return merged.ToArray();
		}

		private static bool HasAttribute<T>(IEnumerable<System.Attribute> attributes) where T : System.Attribute
		{
			return attributes.OfType<T>().Any();
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

			RuntimeTypeInfo returnTypeInfo;
			if (source.ReturnType != null &&
				source.ReturnType.Kind != ReflectionGenerator.RuntimeTypeDB.RuntimeTypeKind.Invalid)
			{
				returnTypeInfo = GetCreateRuntimeTypeInfo(source.ReturnType);
			}
			else
			{
				returnTypeInfo = RuntimeTypeInfo.Invalid;
			}

			RuntimeTypeInfo[] argumentTypeList;
			if (source.ArgumentTypeList == null || source.ArgumentTypeList.Length == 0)
			{
				argumentTypeList = [];
			}
			else
			{
				argumentTypeList = new RuntimeTypeInfo[source.ArgumentTypeList.Length];
				for (int i = 0; i < argumentTypeList.Length; ++i)
				{
					argumentTypeList[i] = GetCreateRuntimeTypeInfo(source.ArgumentTypeList[i]);
				}
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
				ReturnTypeInfo = returnTypeInfo,
				ArgumentTypeList = argumentTypeList,
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
			if (decl is RuntimeRecordDecl recordDecl && _RecordDeclDictWithFQN.TryAdd(recordDecl.FullName, recordDecl))
			{
				_RecordDeclList.Add(recordDecl);
			}
		}


		public System.Attribute CreateAttribute(in ReflectionGenerator.RuntimeTypeDB.AttributeDecl attributeDecl)
		{
			switch (attributeDecl.Kind)
			{
				case ReflectionGenerator.RuntimeTypeDB.AttributeKind.Standard:
					return _UnknownRuntimeAttribute;

				case ReflectionGenerator.RuntimeTypeDB.AttributeKind.EngineAnnotate:
					ReadOnlySpan<char> annotate = attributeDecl.Value;

					//	アノテーション属性の文字列を解析して、型名(FQN)と引数リストに分解する
					ReadOnlySpan<char> fqn = Local.ParseAnnotate(annotate, out object[] args);

					switch (fqn)
					{
						case "nox::attr::DataMember":
							return new System.Runtime.Serialization.DataMemberAttribute();
						case "nox::attr::IgnoreDataMember":
							return new System.Runtime.Serialization.IgnoreDataMemberAttribute();
						case "nox::attr::dev::DisplayName":
							{
								if (args.Length != 1 || args[0] is not string)
								{
									Nox.LogTrace.WarningLine<Core.LogId.Runtime>($"DisplayName属性の引数が不正です。FQN={fqn}, Args={string.Join(", ", args)}");
									break;
								}
								return new System.ComponentModel.DisplayNameAttribute((string)args[0]);
							}

						case "nox::attr::dev::Description":
							{
								if (args.Length != 1 || args[0] is not string)
								{
									Nox.LogTrace.WarningLine<Core.LogId.Runtime>($"Description属性の引数が不正です。FQN={fqn}, Args={string.Join(", ", args)}");
									break;
								}
								return new System.ComponentModel.DescriptionAttribute((string)args[0]);
							}

						case "nox::attr::dev::Category":
							{
								if (args.Length != 1 || args[0] is not string)
								{
									Nox.LogTrace.WarningLine<Core.LogId.Runtime>($"Category属性の引数が不正です。FQN={fqn}, Args={string.Join(", ", args)}");
									break;
								}
								return new System.ComponentModel.CategoryAttribute((string)args[0]);
							}

						case "nox::attr::dev::ReadOnly":
							return new System.ComponentModel.ReadOnlyAttribute(true);

						default:
							{
								//	RuntimeAttribute継承属性を探す
								//	例：nox::attr::Resource(u8"ext", 1)、nox::attr::dev::PropertySetter()など

								if (_RuntimeAttributeTypeDictWithRuntimeFQN.TryGetValue(fqn.ToString(), out System.Type? wrapperAttrType) == false)
								{
									Nox.LogTrace.WarningLine<Core.LogId.Runtime>($"RuntimeAttributeの生成に失敗しました。RuntimeAttributeのFQN={fqn}に対応するRuntimeWrapperAttributeが見つかりませんでした。");
									break;
								}

								ReadOnlySpan<System.Reflection.ConstructorInfo> constructors = wrapperAttrType.GetConstructors();
								foreach (var constructor in constructors)
								{
									ReadOnlySpan<ParameterInfo> parameters = constructor.GetParameters();

									if (parameters.Length != args.Length)
									{
										continue;
									}

									for (int argIndex = 0; argIndex < parameters.Length; argIndex++)
									{
										var paramType = parameters[argIndex].ParameterType;
										if (args[argIndex] is not null && args[argIndex].GetType() != paramType)
										{
											args[argIndex] = System.Convert.ChangeType(args[argIndex], paramType, System.Globalization.CultureInfo.InvariantCulture);
										}
									}

									var attr = constructor.Invoke(args);
									Nox.Util.Assert(attr != null, "runtimeWrapper属性の生成に失敗しました:{0}", wrapperAttrType.GetFullName());

									var attrImpl = attr as Core.RuntimeAttributes.RuntimeAttribute;
									Nox.Util.Assert(attrImpl != null, $"RuntimeWrapperAttributeを継承したクラスはCore.RuntimeAttributes.RuntimeAttributeも継承する必要があります。Type={wrapperAttrType.GetFullName()}");
									return attrImpl;
								}

								Nox.Util.Assert(false, $"RuntimeWrapperAttributeのコンストラクタが見つかりませんでした。Type={wrapperAttrType.GetFullName()}, FQN={fqn}");
								throw new NotImplementedException();
							}
					}
					break;
			}

			return _UnknownRuntimeAttribute;
		}
		#endregion
	}
}
