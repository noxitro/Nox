using Microsoft.VisualStudio.Shell.Interop;
using Nox;
using System;
using System.Collections.Generic;
using System.Reflection;
using System.Text;

namespace Core
{
	public static class Util
	{
		public static string ToRuntimeFQN(string fqn)
		{
			return fqn.Replace(".", "::");
		}
	}
}

namespace Core.RuntimeRemote
{
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
			RuntimeType,
			EditorType
		}

		/// <summary>
		/// runtime側でのプロパティ定義情報
		/// </summary>
		private readonly struct RuntimePropertyInfo
		{
			/// <summary>
			/// 型名
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
			/// runtime側でのメンバ変数名
			/// camel_case_
			/// </summary>
			public required string NameCamelCase { get; init; }

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
			public readonly Core.RuntimeRemote.Attr.RuntimeRemoteCodeAttribute Attr;
			public readonly System.Type Type;
			public readonly bool IsQuery;

			public readonly RuntimePropertyInfo[] PropertyList = [];

			public RemoteTypeInfo(System.Type type, Core.RuntimeRemote.Attr.RuntimeRemoteCodeAttribute attr, bool isQuery)
			{
				Type = type;
				Attr = attr;
				IsQuery = isQuery;
			}
		}
		#endregion

		#region 公開メソッド
		public void GenerateCode()
		{
			//	RuntimeWrapper型にDTIを設定する
			System.Type queryType = typeof(Core.RuntimeRemote.Query);
			System.Type responseType = typeof(Core.RuntimeRemote.Response);

			System.Type runtimeRemoteCodeAttributeType = typeof(Core.RuntimeRemote.Attr.RuntimeRemoteCodeAttribute);

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
				if (type.IsAssignableFrom(queryType))
				{
					isQuery = true;
				}
				else if (type.IsAssignableFrom(responseType))
				{
					isQuery = false;
				}
				else
				{
					continue;
				}

				Core.RuntimeRemote.Attr.RuntimeRemoteCodeAttribute? attr = type.GetCustomAttribute<Core.RuntimeRemote.Attr.RuntimeRemoteCodeAttribute>();
				if (attr == null)
				{
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

				data.TypeInfoList.Add(new (type, attr, isQuery));
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
				codeWriter.WriteLineHeader("RuntimeRemoteCodeGenerator");
				codeWriter.WriteNewLine();

				codeWriter.WriteLinePragmaOnce();
				codeWriter.WriteLinePPIf("NOX_DEVELOP");

				if (data.IsCoreEntry)
				{
					codeWriter.WriteLineInclude("../editor_ipc_query.h");
					codeWriter.WriteLineInclude("../editor_ipc_response.h");
				}

				codeWriter.WriteNamespace("nox::dev::editor_ipc");
				using (codeWriter.Indent("{", "}"))
				{
					foreach(var param in data.TypeInfoList)
					{
						ReadOnlySpan<char> typeName = param.Type.Name;

						ReadOnlySpan<char> baseTypeFullName = param.IsQuery ? "nox::dev::editor_ipc::Query" : "nox::dev::editor_ipc::Response";
						if (param.IsQuery)
						{
							codeWriter.WriteLine($"class {typeName} : public {baseTypeFullName}");
						}

						using (codeWriter.Indent("{", "};"))
						{
							codeWriter.WriteLine($"NOX_DECLARE_OBJECT(nox::dev::editor_ipc{typeName}, {baseTypeFullName})");

							var propList = param.PropertyList;

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
										codeWriter.WriteLine("public:");
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

							codeWriter.WriteLine("public:");
							codeWriter.WriteLine($"{typeName}();");
							codeWriter.WriteLine("void OnSerialize(nox::dev::editor_ipc::SocketStreamWriter& writer)override;");
							codeWriter.WriteLine("void OnDeserialize(nox::dev::editor_ipc::SocketStreamReader& reader)override;");

							if (param.IsQuery)
							{
								if (param.Attr.EnabledExecute)
								{
									codeWriter.WriteLine("nox::PlacementObject<nox::dev::editor_ipc::Respose> Execute(std::span<nox::uint8> storage)const override;");
								}
								else
								{
									codeWriter.WriteLine("inline constexpr nox::PlacementObject<nox::dev::editor_ipc::Respose> Execute(std::span<nox::uint8>)const override { return nullptr; }");
								}
							}

							//	プロパティsetter, getter書き込み
							codeWriter.WriteNewLine();
							for (int propIndex = 0, propLength = propList.Length; propIndex < propLength; ++propIndex)
							{
								ref readonly var prop = ref propList[propIndex];
								codeWriter.WriteLine($"inline {prop.GetterTypeFqn} Get{prop.NamePascalCase}()");
								using (codeWriter.Indent("{", "}"))
								{
									codeWriter.WriteLine($"return {prop.NameCamelCase};");
								}

								codeWriter.WriteLine($"inline void Set{prop.NamePascalCase}({prop.SetterTypeFqn} value)");
								using (codeWriter.Indent("{", "}"))
								{
									codeWriter.WriteLine($"{prop.NameCamelCase} = value;");
								}
							}

							//	プロパティメンバ書き込み
							codeWriter.WriteNewLine();
							codeWriter.WriteLine("private:");
							for (int propIndex = 0, propLength = propList.Length; propIndex < propLength; ++propIndex)
							{
								ref readonly var prop = ref propList[propIndex];
								codeWriter.WriteLine($"{prop.TypeFQN} {prop.NameCamelCase};");
							}
						}
					}
				}

				codeWriter.WriteLinePPEndIf("NOX_DEVELOP");
			}

			//	cpp
			using (Nox.CodeWriter codeWriter = new CodeWriter($"{basePath}.g.cpp"))
			{
				codeWriter.WriteLineSource("RuntimeRemoteCodeGenerator");
				codeWriter.WriteNewLine();

				codeWriter.WriteIncludeStdafx();

				codeWriter.WriteLineInclude($"{fileName}.g.h");
				if (data.IsCoreEntry)
				{
					codeWriter.WriteLineInclude("../socket_stream_writer.h");
					codeWriter.WriteLineInclude("../socket_stream_reader.h");
				}

				codeWriter.WriteNewLine();

				foreach (var param in data.TypeInfoList)
				{
					codeWriter.WriteLine($"void nox::dev::editor_ipc::{param.Type}::OnSerialize(nox::dev::editor_ipc::SocketStreamWriter& writer)");
					using(codeWriter.Indent("(", ")"))
					{

					}

					codeWriter.WriteNewLine();

					codeWriter.WriteLine($"void nox::dev::editor_ipc::{param.Type}::OnDeserialize(nox::dev::editor_ipc::SocketStreamReader& reader)");
					using (codeWriter.Indent("(", ")"))
					{
						
					}
				}
			}

			//	user定義cppの部分を無いなら作成する
			string userSourcePath = $"{basePath}.cpp";
			if (System.IO.File.Exists(userSourcePath) == false)
			{
				using (Nox.CodeWriter codeWriter = new CodeWriter(userSourcePath))
				{
					codeWriter.WriteLineCopyRight();
					codeWriter.WriteNewLine();
					codeWriter.WriteIncludeStdafx();
					codeWriter.WriteLineInclude($"{fileName}.g.h");

					var dataList = data.TypeInfoList;
					foreach (var param in data.TypeInfoList)
					{
						if (param.Attr.EnabledExecute == false)
						{
							continue;
						}

						codeWriter.WriteNewLine();

						if (param.IsQuery)
						{
							codeWriter.WriteLine("nox::dev::editor_ipc::Response* nox::dev::editor_ipc::{param.Type}::Execute()const");
							using (codeWriter.Indent("{", "}"))
							{
								codeWriter.WriteLine("return nullptr;");
							}
						}
						else
						{
							codeWriter.WriteLine("void nox::dev::editor_ipc::{param.Type}::Execute()const");
							using (codeWriter.Indent("{", "}"))
							{
								codeWriter.WriteNewLine();
							}
						}
					}
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
			System.Type propertyType = sourceProperty.PropertyType;
			System.TypeCode typeCode = Type.GetTypeCode(propertyType);
			PropertyTypeKind propertyTypeKind = PropertyTypeKind.RuntimeType;

			string typeFqn;
			string setterTypeFqn;
			string getterTypeFqn;

			string propName = sourceProperty.Name;
			string pascalCaseName = char.ToUpperInvariant(propName[0]) + propName.Substring(1);
			string camelCaseName = char.ToLowerInvariant(propName[0]) + propName.Substring(1) + "_";

			if (propertyType.IsPrimitive)
			{
				typeFqn = Core.RuntimeTypeUtil.GetPrimitiveTypeName(typeCode).ToString();
				setterTypeFqn = getterTypeFqn = typeFqn;
			}
			else if (typeCode == TypeCode.String)
			{
				typeFqn = "nox::U8String";
				getterTypeFqn = setterTypeFqn = "std::u8string_view";
			}
			//	fixed string
			else
			if (propertyType.GetCustomAttribute<Core.RuntimeRemote.Attr.FixedStringAttribute>() is var propAttr && propAttr != null)
			{
				typeFqn = $"nox::FixedString<{propAttr.Length}>";
				getterTypeFqn = setterTypeFqn = "std::u8string_view";
			}
			//	runtime wrapper
			else
			if (propertyType.GetCustomAttribute<Core.Attributes.RuntimeWrapperAttribute>() is var wrapperAttr && wrapperAttr != null)
			{
				typeFqn = wrapperAttr.FQN;
				setterTypeFqn = typeFqn;
				getterTypeFqn = typeFqn;
			}
			//	other
			else
			{
				//	runtime primitive
				var runtimePrimitiveTypeFqn = GetRuntimePrimitiveType(propertyType);
				if (runtimePrimitiveTypeFqn != string.Empty)
				{
					typeFqn = runtimePrimitiveTypeFqn;
					setterTypeFqn = $"const {runtimePrimitiveTypeFqn}&";
					getterTypeFqn = $"const {runtimePrimitiveTypeFqn}&";
				}
				//	ツール側の定義型
				else
				{
					propertyTypeKind = PropertyTypeKind.EditorType;
					if (propertyType.IsEnum)
					{
						typeFqn = propertyType.Name;
						setterTypeFqn = getterTypeFqn = typeFqn;
					}
					else if (propertyType.IsValueType)
					{
						typeFqn = propertyType.Name;
						setterTypeFqn = getterTypeFqn = $"const {typeFqn}&";
					}
					else
					{
						typeFqn = propertyType.Name;
						setterTypeFqn = getterTypeFqn = typeFqn;
						Nox.Util.Assert(false, "invalid property type:{0}", propertyType.FullName);
					}
				}
			}

			RuntimePropertyInfo propInfo = new RuntimePropertyInfo(typeFqn)
			{
				SetterTypeFqn = setterTypeFqn,
				GetterTypeFqn = getterTypeFqn,
				NameCamelCase = camelCaseName,
				NamePascalCase = pascalCaseName,
				Kind = propertyTypeKind,
				RawPropertyInfo = sourceProperty
			};

			return propInfo;
		}

		private static string GetRuntimePrimitiveType(System.Type type)
		{
			if (type == typeof(Nox.Math.Vec3))
			{
				return "nox::Vec3";
			}

			return string.Empty;
		}
		#endregion
	}
}
