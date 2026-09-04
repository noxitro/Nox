using Microsoft.VisualStudio.Shell.Interop;
using Nox;
using Nox.Extensions;
using System;
using System.Collections.Generic;
using System.Reflection;
using System.Text;

namespace Core.RuntimeRemote;

public class RuntimeRemoteCodeGenerator
{
	#region 内部クラス定義
	private readonly struct Data
	{
		public readonly bool IsCoreEntry;
		public readonly List<RemoteTypeInfo> TypeInfoList = new ();

		public Data(bool isCoreEntry)
		{
			IsCoreEntry = isCoreEntry;
		}
	}

	private enum PropertyTypeKind : byte
	{
		Invalid,
		PrimitiveType,
		RuntimeObject,
		//RuntimeManagedObject,
		String,
		EditorType,
	}

	/// <summary>
	/// runtime側でのプロパティ定義情報
	/// </summary>
	private readonly struct RuntimePropertyInfo
	{
		/// <summary>
		/// 型名
		/// runtime fqn
		/// </summary>
		public readonly string TypeFQN;

		/// <summary>
		/// setter引数の型名
		/// </summary>
		public string SetterTypeFqn { get; init; } = string.Empty;

		/// <summary>
		/// getter戻り値の型名
		/// </summary>
		public string GetterTypeFqn { get; init; } = string.Empty;

		/// <summary>
		/// メンバ変数として定義する際の型名
		/// </summary>
		public required string MemberDeclTypeName { get; init; }

		/// <summary>
		/// runtime側でのメンバ変数名
		/// snake_case_
		/// </summary>
		public required string NameSnakeCase { get; init; }

		/// <summary>
		/// getterでの記述名
		/// </summary>
		public string? GetterStr { get; init; } = null;

		/// <summary>
		/// setterでの記述名
		/// </summary>
		public string? SetterStr { get; init; } = null;

		/// <summary>
		/// runtime側での関数名用
		/// </summary>
		public required string NamePascalCase { get; init; }

		public required System.Reflection.PropertyInfo RawPropertyInfo { get; init; }

		public required PropertyTypeKind Kind { get; init; }

		public RuntimePropertyInfo(string fqn)
		{
			TypeFQN = fqn;
		}
	}

	private readonly struct RemoteTypeInfo
	{
		public readonly Core.RuntimeRemote.Attributes.RuntimeRemoteCodeAttribute Attr;
		public readonly System.Type Type;
		public readonly bool IsQuery;

		public readonly RuntimePropertyInfo[] PropertyList = [];

		public RemoteTypeInfo(System.Type type, Core.RuntimeRemote.Attributes.RuntimeRemoteCodeAttribute attr, bool isQuery, RuntimePropertyInfo[] propertyList)
		{
			Type = type;
			Attr = attr;
			IsQuery = isQuery;
			PropertyList = propertyList;
		}
	}
	#endregion

	#region 公開メソッド
	public void GenerateCode()
	{
		//	RuntimeWrapper型にDTIを設定する
		System.Type queryType = typeof(Core.RuntimeRemote.Query);
		System.Type responseType = typeof(Core.RuntimeRemote.Response);

		System.Type runtimeRemoteCodeAttributeType = typeof(Core.RuntimeRemote.Attributes.RuntimeRemoteCodeAttribute);

		//	key: 出力先ファイルパス
		Dictionary<string, Data> dict = new();

		string solutionDir = Core.StudioManager.Instance.StudioInfo.RuntimeSolutionDir;

		foreach (System.Type type in Core.TypeDB.AllTypeList)
		{
			if (type.IsAbstract)
			{
				continue;
			}

			bool isQuery = false;
			if (queryType.IsAssignableFrom(type))
			{
				isQuery = true;
			}
			else if (responseType.IsAssignableFrom(type))
			{
				isQuery = false;
			}
			else
			{
				continue;
			}

			Core.RuntimeRemote.Attributes.RuntimeRemoteCodeAttribute? attr = type.GetCustomAttribute<Core.RuntimeRemote.Attributes.RuntimeRemoteCodeAttribute>();
			if (attr == null)
			{
				Nox.LogTrace.ErrorLine<Core.RuntimeRemote.LogId.RuntimeRemote>("Type '{0}' is missing RuntimeRemoteCodeAttribute.", type.FullName ?? "invalid");
				continue;
			}

			//	Serialize対象のプロパティ一覧を作成
			ReadOnlySpan<System.Reflection.PropertyInfo> srcPropList = GetSerializeProperties(type);
			RuntimePropertyInfo[] propList = new RuntimePropertyInfo[srcPropList.Length];
			for (int i = 0, length = srcPropList.Length; i < length; ++i)
			{
				propList[i] = CreatePropertyInfo(srcPropList[i]);
			}

			string genPath = $"{solutionDir}/{attr.Path}";
			if (dict.TryGetValue(genPath, out var data) == false)
			{
				bool coreEntry = attr.Path.IndexOf("core/") == 0;
				dict.Add(genPath, data = new(coreEntry));
			}

			data.TypeInfoList.Add(new RemoteTypeInfo (type, attr, isQuery, propList));
		}

		foreach (var v in dict)
		{
			GenerateImpl(v.Key, v.Value);
		}
	}
	#endregion

	#region 非公開メソッド
	/// <summary>
	/// 
	/// </summary>
	/// <param name="basePath">出力先ファイルパス（拡張子を除く）</param>
	/// <param name="data"></param>
	private void GenerateImpl(string basePath, in Data data)
	{
		string fileName = System.IO.Path.GetFileName(basePath);

		//	h
		using (Nox.CodeWriter codeWriter = new CodeWriter($"{basePath}.g.h"))
		{
			Nox.LogTrace.InfoLine<Core.RuntimeRemote.LogId.RuntimeRemote>("Generating remote code to '{0}.g.h' and '{0}.g.cpp'", basePath);

			codeWriter.WriteLineHeader("RuntimeRemoteCodeGenerator");
			codeWriter.WriteNewLine();

			codeWriter.WriteLinePPIf("NOX_DEVELOP");

			//	メンバの前方宣言
			{
				HashSet<System.Type> runtimeWrawpperTypeHashSet = new HashSet<Type>();

				foreach (var param in data.TypeInfoList)
				{
					foreach (var prop in param.PropertyList)
					{
						switch (prop.Kind)
						{
							case PropertyTypeKind.RuntimeObject:
								break;
							//	runtime wrapper以外は前方宣言不要
							default:
								continue;
						}

						System.Type runtimeWrapperType = prop.RawPropertyInfo.PropertyType;
						if (runtimeWrawpperTypeHashSet.Contains(runtimeWrapperType) == true)
						{
							continue;
						}
						if (runtimeWrawpperTypeHashSet.Count == 0)
						{
							codeWriter.WriteLine("// forward declaration for runtime wrapper types");
						}
						runtimeWrawpperTypeHashSet.Add(runtimeWrapperType);

						string namespaceStr = Core.Util.GetNamespaceFromRuntimeFQN(prop.TypeFQN);
						codeWriter.WriteLine($"namespace {namespaceStr} {{ class {Core.Util.GetNameFromRuntimeFQN(prop.TypeFQN)}; }}");
					}
				}

				if (runtimeWrawpperTypeHashSet.Count > 0)
				{
					codeWriter.WriteLine("// end forward declaration");
					codeWriter.WriteNewLine();
				}
			}

			if (data.IsCoreEntry)
			{
				codeWriter.WriteLineInclude("../editor_remote_query.h");
				codeWriter.WriteLineInclude("../editor_remote_response.h");
			}

			codeWriter.WriteNewLine();
			codeWriter.WriteNamespace("nox::dev::editor_remote");
			using (codeWriter.Indent("{", "}"))
			{
				foreach(var param in data.TypeInfoList)
				{
					ReadOnlySpan<char> typeName = param.Type.Name;

					ReadOnlySpan<char> baseTypeFullName = param.IsQuery ? "nox::dev::editor_remote::Query" : "nox::dev::editor_remote::Response";
					
					if (param.Attr.Comment != string.Empty)
					{
						codeWriter.WriteLine($"/// @brief {param.Attr.Comment}");
					}
					if (param.Attr.EnabledSend)
					{
                        codeWriter.WriteLine($"class {typeName} final : public {baseTypeFullName}");
                    }
					else
					{
                        codeWriter.WriteLine($"class {typeName} final : public {baseTypeFullName}, nox::dev::editor_remote::IRecvOnlyQueryTag");
                    }

					using (codeWriter.Indent("{", "};"))
					{
						codeWriter.WriteLine($"NOX_DECLARE_OBJECT(nox::dev::editor_remote::{typeName}, {baseTypeFullName});");

						var propList = param.PropertyList;
						int propertyLength = propList.Length;

						//	editor側の型定義
						{
							bool first = true;

							foreach (var prop in propList)
							{
								if (prop.Kind != PropertyTypeKind.EditorType)
								{
									continue;
								}

								if (first)
								{
									codeWriter.WriteLineOutdent("public:", 1);
									first = false;
								}
								else
								{
									codeWriter.WriteNewLine();
								}

								System.Type propType = prop.RawPropertyInfo.PropertyType;
								if (propType.IsEnum)
								{
									ReadOnlySpan<char> underlyingTypeFqn = Core.RuntimeTypeUtil.GetPrimitiveTypeName(Type.GetTypeCode(propType.GetEnumUnderlyingType()));
									codeWriter.WriteLine($"enum class {propType.Name} : {underlyingTypeFqn}");
									using (codeWriter.Indent("{", "};"))
									{
										ReadOnlySpan<string> nameList = propType.GetEnumNames();
										Array enumValues = propType.GetEnumValues();

										for (int enumeratorIndex = 0, length = nameList.Length; enumeratorIndex < length; ++enumeratorIndex)
										{
											string enumName = nameList[enumeratorIndex];
											var enumValue = Convert.ChangeType(enumValues.GetValue(enumeratorIndex), Enum.GetUnderlyingType(propType));
											if (enumeratorIndex + 1 < length)
											{
												codeWriter.WriteLine($"{enumName} = {enumValue},");
											}
											else
											{
												codeWriter.WriteLine($"{enumName} = {enumValue}");
											}
										}
									}
							}
									else if (propType.IsValueType)
									{
										codeWriter.WriteLine($"struct {propType.Name}");
										using (codeWriter.Indent("{", "};"))
										{
											var structPropList = GetSerializeProperties(propType);
											for (int structPropIndex = 0, structPropLength = structPropList.Length; structPropIndex < structPropLength; ++structPropIndex)
											{
												var structProp = structPropList[structPropIndex];
												System.Type structPropType = structProp.PropertyType;
												System.TypeCode structPropTypeCode = Type.GetTypeCode(structPropType);
												string structPropTypeFqn;
												if (structPropType.IsPrimitive)
												{
													structPropTypeFqn = Core.RuntimeTypeUtil.GetPrimitiveTypeName(structPropTypeCode).ToString();
												}
												else if (structPropTypeCode == TypeCode.String)
												{
													structPropTypeFqn = "nox::U8String";
												}
												else
												{
													Nox.Util.Assert(false, "unsupported editor type property:{0}", structPropType.FullName);
													continue;
												}
												string structPropName = char.ToLowerInvariant(structProp.Name[0]) + structProp.Name.Substring(1) + "_;";
												codeWriter.WriteLine($"{structPropTypeFqn} {structPropName}");
											}
										}
									}
									else
									{
										Nox.Util.Assert(false, "unsupported editor type property:{0}", propType.FullName);
									}
								} 
							}

							codeWriter.WriteLineOutdent("public:", 1);

							if (propertyLength == 0)
							{
								//	メンバが無い場合はnoexcept指定してデフォルトコンストラクタを生成しておく
								codeWriter.WriteLine($"{typeName}()noexcept{{}}");
							}
							else
							{
								codeWriter.WriteLine($"{typeName}(){{}}");
							}

							//	メンバが無い場合は仮にSerialize/Deserialize関数をinlineで空実装しておく
							if (propertyLength == 0)
							{
								codeWriter.WriteLine("inline constexpr void OnSerialize(nox::dev::editor_remote::SocketStreamWriter&)override {}");
								codeWriter.WriteLine("inline constexpr void OnDeserialize(nox::dev::editor_remote::SocketStreamReader&)override {}");
							}
							else
							{
								codeWriter.WriteLine("void OnSerialize(nox::dev::editor_remote::SocketStreamWriter& writer)override;");
								codeWriter.WriteLine("void OnDeserialize(nox::dev::editor_remote::SocketStreamReader& reader)override;");
							}

							if (param.IsQuery)
							{
								if (param.Attr.EnabledExecute)
								{
									codeWriter.WriteLine("nox::PlacementObject<nox::dev::editor_remote::Response> Execute(nox::World&, std::span<nox::uint8> storage)const override;");
								}
								else
								{
									codeWriter.WriteLine("inline constexpr nox::PlacementObject<nox::dev::editor_remote::Response> Execute(nox::World&, std::span<nox::uint8>)const override { return nullptr; }");
								}
							}

							//	プロパティsetter, getter書き込み
							int propLength = propList.Length;
							if (propLength > 0)
							{
								codeWriter.WriteNewLine();
								for (int propIndex = 0; propIndex < propLength; ++propIndex)
								{
									ref readonly var prop = ref propList[propIndex];

									codeWriter.WriteLine($"inline {prop.GetterTypeFqn} Get{prop.NamePascalCase}()const noexcept");
									using (codeWriter.Indent("{", "}"))
									{
										if (prop.GetterStr != null)
										{
											codeWriter.WriteLine($"return {prop.GetterStr};");
										}
										else
										{
											codeWriter.WriteLine($"return {prop.NameSnakeCase};");
										}
									}

									codeWriter.WriteNewLine();

									codeWriter.WriteLine($"inline void Set{prop.NamePascalCase}({prop.SetterTypeFqn} value)");
									using (codeWriter.Indent("{", "}"))
									{
										if (prop.SetterStr != null)
										{
											codeWriter.WriteLine($"{prop.SetterStr} = value;");
										}
										else
										{
											codeWriter.WriteLine($"{prop.NameSnakeCase} = value;");
										}
									}

									codeWriter.WriteNewLine();
								}

								//	プロパティメンバ書き込み
								codeWriter.WriteLineOutdent("private:", 1);
								for (int propIndex = 0; propIndex < propLength; ++propIndex)
								{
									ref readonly var prop = ref propList[propIndex];
									codeWriter.WriteLine($"{prop.MemberDeclTypeName} {prop.NameSnakeCase} {{}};");
								}
							}
						}

						codeWriter.WriteNewLine();
					}
				}

				codeWriter.WriteLinePPEndIf("NOX_DEVELOP");
			}

			//	cpp
			using (Nox.CodeWriter codeWriter = new CodeWriter($"{basePath}.g.cpp"))
			{
				codeWriter.WriteLineSource("RuntimeRemoteCodeGenerator");
				codeWriter.WriteNewLine();

				codeWriter.WriteIncludePch();

				//	.g.h が丸ごと NOX_DEVELOP で消えるので、実装側も同じ条件で消す
				codeWriter.WriteLinePPIf("NOX_DEVELOP");
				codeWriter.WriteLineInclude($"{fileName}.g.h");

				//	codegen_preamble.hがあれば、includeしておく
				{
					string preamblePath = System.IO.Path.Combine(System.IO.Path.GetDirectoryName(basePath) ?? string.Empty, "codegen_preamble.h");
					if (System.IO.File.Exists(preamblePath))
					{
						codeWriter.WriteLineInclude("codegen_preamble.h");
						codeWriter.WriteNewLine();
					}
				}

				codeWriter.WriteNewLine();

				foreach (var param in data.TypeInfoList)
				{
					string runtimeTypeFQN = Core.Util.ToRuntimeFQN(param.Type.Name ?? string.Empty);
					int propertyLength = param.PropertyList.Length;

					if (propertyLength > 0)
					{
						codeWriter.WriteLine($"void nox::dev::editor_remote::{runtimeTypeFQN}::OnSerialize(nox::dev::editor_remote::SocketStreamWriter& writer)");
						using (codeWriter.Indent("{", "}"))
						{
							if (param.Attr.EnabledSend)
							{
								for (int propIndex = 0; propIndex < propertyLength; ++propIndex)
								{
									ref readonly RuntimePropertyInfo propInfo = ref param.PropertyList[propIndex];

									codeWriter.WriteLine($"writer.Write({propInfo.NameSnakeCase});");
								}
							}
							else
							{
								codeWriter.WriteLine("NOX_ASSERT(false, u8\"受信専用Queryです\");");
                            }
						}

						codeWriter.WriteNewLine();

						codeWriter.WriteLine($"void nox::dev::editor_remote::{runtimeTypeFQN}::OnDeserialize(nox::dev::editor_remote::SocketStreamReader& reader)");
						using (codeWriter.Indent("{", "}"))
						{
							if (param.Attr.EnabledRecv)
							{
								for (int propIndex = 0; propIndex < propertyLength; ++propIndex)
								{
									ref readonly RuntimePropertyInfo propInfo = ref param.PropertyList[propIndex];

									switch (propInfo.Kind)
									{
										case PropertyTypeKind.String:
											codeWriter.WriteLine($"reader.Read({propInfo.NameSnakeCase});");
											break;
										default:
											codeWriter.WriteLine($"reader.Read({propInfo.NameSnakeCase});");
											break;
									}
								}
							}
							else
							{
								codeWriter.WriteLine("NOX_ASSERT(false, u8\"送信専用Queryです\");");
                            }
						}
					}
				}

				codeWriter.WriteLinePPEndIf("NOX_DEVELOP");
			}

			//	user定義cppの部分を無いなら作成する
			string userSourcePath = $"{basePath}.cpp";
			if (System.IO.File.Exists(userSourcePath) == false)
			{
				using (Nox.CodeWriter codeWriter = new CodeWriter(userSourcePath))
				{
					Nox.LogTrace.InfoLine<Core.RuntimeRemote.LogId.RuntimeRemote>("Generating remote user code to '{0}.cpp'", basePath);

					codeWriter.WriteLineCopyRight();
					codeWriter.WriteNewLine();
					codeWriter.WriteIncludePch();
					//	.g.h と同じ条件で消す
					codeWriter.WriteLinePPIf("NOX_DEVELOP");
					codeWriter.WriteLineInclude($"{fileName}.g.h");
					
					var dataList = data.TypeInfoList;
					foreach (var param in data.TypeInfoList)
					{
						if (param.Attr.EnabledExecute == false)
						{
							continue;
						}

						string runtimeTypeFQN = Core.Util.ToRuntimeFQN(param.Type.Name??string.Empty);

						codeWriter.WriteNewLine();

						if (param.IsQuery)
						{
							codeWriter.WriteLine($"nox::PlacementObject<nox::dev::editor_remote::Response> nox::dev::editor_remote::{runtimeTypeFQN}::Execute(nox::World&, std::span<nox::uint8> storage)const");
							using (codeWriter.Indent("{", "}"))
							{
								codeWriter.WriteLine("return nullptr;");
							}
						}
						else
						{
							codeWriter.WriteLine($"void nox::dev::editor_remote::{param.Type}::Execute()const");
							using (codeWriter.Indent("{", "}"))
							{
								codeWriter.WriteNewLine();
							}
						}
					}

					codeWriter.WriteLinePPEndIf("NOX_DEVELOP");
				}
			}
		}

		private static bool TryGetListElementType(Type type, out Type? elementType)
		{
			elementType = null;
			if (type == null) return false;

			// 配列
			if (type.IsArray)
			{
				elementType = type.GetElementType();
				return elementType != null;
			}

			// ジェネリック型そのものが List<T>/IList<T>/IEnumerable<T>/IReadOnlyList<T> 等
			if (type.IsGenericType)
			{
				var def = type.GetGenericTypeDefinition();
				if (def == typeof(List<>) || def == typeof(IList<>) || def == typeof(IEnumerable<>) || def == typeof(IReadOnlyList<>))
				{
					elementType = type.GetGenericArguments()[0];
					return true;
				}
			}

			// 実装しているインターフェイスを調べる
			foreach (var iface in type.GetInterfaces())
			{
				if (iface.IsGenericType)
				{
					var id = iface.GetGenericTypeDefinition();
					if (id == typeof(IEnumerable<>) || id == typeof(IList<>) || id == typeof(IReadOnlyList<>))
					{
						elementType = iface.GetGenericArguments()[0];
						return true;
					}
				}
				else if (iface == typeof(System.Collections.IList))
				{
					// 非ジェネリック IList をサポートする（要素型不明 -> object）
					elementType = typeof(object);
					return true;
				}
			}

			return false;
		}

		/// <summary>
		/// Serialize対象のプロパティ一覧を取得する
		/// </summary>
		/// <param name="type"></param>
		/// <returns></returns>
		private static ReadOnlySpan<System.Reflection.PropertyInfo> GetSerializeProperties(System.Type type)
		{
			ReadOnlySpan<System.Reflection.PropertyInfo> propList = type.GetProperties();
			Span<System.Reflection.PropertyInfo> serializePropList = new System.Reflection.PropertyInfo[propList.Length];
			int enabledCount = 0;
			for (int i = 0, length = propList.Length; i < length; ++i)
			{
				var prop = propList[i];
				
				if (prop.CanRead == false || prop.CanWrite == false)
				{
					continue;
				}
				serializePropList[i] = prop;
				++enabledCount;
			}

			return serializePropList.Slice(0, enabledCount);
		}

		private static RuntimePropertyInfo CreatePropertyInfo(System.Reflection.PropertyInfo sourceProperty)
		{
			System.Type managedObjectType = typeof(Core.RuntimeObject);

			System.Type propertyType = sourceProperty.PropertyType;
			System.TypeCode typeCode = Type.GetTypeCode(propertyType);
			PropertyTypeKind propertyTypeKind;

			string typeFqn;			//	真の型名
			string setterTypeFqn;	//	getterの型
			string getterTypeFqn;	//	setterの型
			string memberDeclTypeName;//	メンバ変数の型名
			string? getterStr = null;       //	getterの実装が特殊な場合の記述文字列
			string? setterStr = null;       //	setterの実装が特殊な場合の記述文字列

			string propName = sourceProperty.Name;
			string pascalCaseName = char.ToUpperInvariant(propName[0]) + propName.Substring(1);
			//string snakeCaseName = char.ToLowerInvariant(propName[0]) + propName.Substring(1) + "_";
			string snakeCaseName;
			{
				StringBuilder sb = new StringBuilder();
				for (int i = 0, length = propName.Length; i < length; ++i)
				{
					char c = propName[i];
					if (char.IsUpper(c) && i > 0)
					{
						sb.Append('_');
					}
					sb.Append(char.ToLowerInvariant(c));
				}
				sb.Append('_');
				snakeCaseName = sb.ToString();
			}

			if (propertyType.IsPrimitive)
			{
				typeFqn = Core.RuntimeTypeUtil.GetPrimitiveTypeName(typeCode).ToString();
				memberDeclTypeName = typeFqn;
				setterTypeFqn = getterTypeFqn = typeFqn;
				propertyTypeKind = PropertyTypeKind.PrimitiveType;
			}
			//	fixed string
			else if (sourceProperty.GetCustomAttribute<Core.RuntimeRemote.Attributes.FixedStringAttribute>() is var propAttr && propAttr != null)
			{
				typeFqn = $"nox::U8FixedString<{propAttr.Length}>";
				memberDeclTypeName = typeFqn;
				getterTypeFqn = setterTypeFqn = "std::u8string_view";
				propertyTypeKind = PropertyTypeKind.String;
			}
			//	fixed array
			else if (sourceProperty.GetCustomAttribute<Core.RuntimeRemote.Attributes.FixedArrayAttribute>() is var fixedArrayAttr && fixedArrayAttr != null)
			{
				//	配列型のはずなので、ElementTypeを得る
				var elementType = propertyType.GetElementType();
				Nox.Util.Assert(elementType != null, $"配列型ではありません:{propertyType.FullName}");

				var runtimePrimitiveTypeFqn = GetRuntimePrimitiveType(elementType);
				if (runtimePrimitiveTypeFqn != string.Empty)
				{
					typeFqn = $"std::array<{runtimePrimitiveTypeFqn}, {fixedArrayAttr.Length}>";
				}
				else if (elementType.IsPrimitive)
				{
					typeFqn = $"std::array<{Core.RuntimeTypeUtil.GetPrimitiveTypeName(Type.GetTypeCode(elementType)).ToString()}, {fixedArrayAttr.Length}>";
				}
				else
				{
					typeFqn = $"std::array<{elementType.Name}, {fixedArrayAttr.Length}>";
				}

				memberDeclTypeName = typeFqn;
				getterTypeFqn = setterTypeFqn = $"const {typeFqn}&";
				propertyTypeKind = PropertyTypeKind.PrimitiveType;
			}
			//	string_view
			else if (sourceProperty.GetCustomAttribute<Core.RuntimeRemote.Attributes.StringViewAttribute>() is var svAttr && svAttr != null)
			{
				typeFqn = "std::u8string_view";
				memberDeclTypeName = typeFqn;
				getterTypeFqn = setterTypeFqn = typeFqn;
				propertyTypeKind = PropertyTypeKind.String;
			}
			else if (typeCode == TypeCode.String)
			{
				typeFqn = "nox::U8String";
				memberDeclTypeName = typeFqn;
				getterTypeFqn = setterTypeFqn = "std::u8string_view";
				propertyTypeKind = PropertyTypeKind.String;
			}
			//	runtime wrapper
			else if (propertyType.GetCustomAttribute<Core.Attributes.RuntimeWrapperAttribute>() is var wrapperAttr && wrapperAttr != null)
			{
				typeFqn = wrapperAttr.RuntimeFQN;
				setterTypeFqn = $"{typeFqn}*";
				getterTypeFqn = $"{typeFqn}*";

				propertyTypeKind = PropertyTypeKind.RuntimeObject;
				memberDeclTypeName = "nox::IntrusivePtr<nox::ManagedObject>";

				getterStr = $"reinterpret_cast<{typeFqn}*>({snakeCaseName}.Get())";
				setterStr = $"{snakeCaseName} = reinterpret_cast<{typeFqn}*>(value)";
			}
			//	other
			else
			{
				//	runtime primitive
				//	vec3など
				var runtimePrimitiveTypeFqn = GetRuntimePrimitiveType(propertyType);
				if (runtimePrimitiveTypeFqn != string.Empty)
				{
					typeFqn = runtimePrimitiveTypeFqn;
					memberDeclTypeName = typeFqn;
					setterTypeFqn = $"const {runtimePrimitiveTypeFqn}&";
					getterTypeFqn = $"const {runtimePrimitiveTypeFqn}&";

					propertyTypeKind = PropertyTypeKind.PrimitiveType;
				}
				//	ツール側の定義型
				else
				{
					propertyTypeKind = PropertyTypeKind.EditorType;
					typeFqn = propertyType.Name;

					if (propertyType.IsEnum)
					{
						setterTypeFqn = getterTypeFqn = typeFqn;
					}
					else if (propertyType.IsValueType)
					{
						setterTypeFqn = getterTypeFqn = $"const {typeFqn}&";
					}
					else
					{
						setterTypeFqn = getterTypeFqn = typeFqn;
						Nox.Util.Assert(false, "invalid property type:{0}", propertyType.FullName);
					}
					memberDeclTypeName = propertyType.Name;
				}
			}

			RuntimePropertyInfo propInfo = new RuntimePropertyInfo(typeFqn)
			{
				SetterTypeFqn = setterTypeFqn,
				GetterTypeFqn = getterTypeFqn,
				MemberDeclTypeName = memberDeclTypeName,
				NameSnakeCase = snakeCaseName,
				NamePascalCase = pascalCaseName,
				Kind = propertyTypeKind,
				RawPropertyInfo = sourceProperty,
				GetterStr = getterStr,
				SetterStr = setterStr,
			};

			return propInfo;
		}

		private static string GetRuntimePrimitiveType(System.Type type)
		{
			if (type == typeof(System.Numerics.Vector3))
			{
				return "nox::Vec3";
			}

			return string.Empty;
		}
		#endregion
	}
