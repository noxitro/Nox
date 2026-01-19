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
	internal class RuntimeRemoteCodeGenerator
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
			Primitive,
			CustomPrimitive,// nox::vec3など
			FixedArray,
			Vector,
			RuntimeWrapper,
		}

		private readonly struct PropertyInfo
		{
			public readonly string TypeFQN;
			public bool HasSetter { get; init; }
			public bool HasGetter { get; init; }
			public required string Name { get; init; }
			public required PropertyTypeKind Kind { get; init; }

			public PropertyInfo(string fqn)
			{
				TypeFQN = fqn;
			}
		}

		private readonly struct RemoteTypeInfo
		{
			public readonly Core.RuntimeRemote.Attr.RuntimeRemoteCodeAttribute Attr;
			public readonly System.Type Type;
			public readonly bool IsQuery;

			public readonly PropertyInfo[] PropertyList = [];

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

				ReadOnlySpan<System.Reflection.PropertyInfo> srcPropList = type.GetProperties();
				int propLength = srcPropList.Length;
				PropertyInfo[] propList = new PropertyInfo[propLength];
				for (int i = 0; i < propLength; ++i)
				{
					System.Reflection.PropertyInfo src = srcPropList[i];
					System.Type propType = src.PropertyType;

					string propFQN = CreateRuntimeTypeFQN(propType);
					bool hasSetter = src.SetMethod != null;
					bool hasGetter = src.GetMethod != null;

					//	camel caseへ
					//	末尾に"_"
					string propName = src.Name;
					string camelCaseName = char.ToLowerInvariant(propName[0]) + propName.Substring(1) + "_";

					propList[i] = new PropertyInfo(propFQN)
					{
						Name = camelCaseName,
						HasGetter = hasGetter, HasSetter = hasSetter, 
					};
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

							codeWriter.WriteLine("private:");
							var propList = param.PropertyList;
							for (int propIndex = 0, propLength = propList.Length; propIndex < propLength; ++propIndex)
							{
								ref readonly var prop = ref propList[propIndex];
								codeWriter.WriteLine($"{prop.TypeFQN} {prop.Name};");
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

		private static string GetPrimitiveTypeName(TypeCode typeCode)
		{
			switch(typeCode)
			{
				case TypeCode.Boolean:	return "bool";
				case TypeCode.SByte:	return "nox::int8";
				case TypeCode.Byte:		return "nox::uint8";
				case TypeCode.Int16: return "nox::int16";
				case TypeCode.UInt16: return "nox::uint16";
				case TypeCode.Int32: return "nox::int32";
				case TypeCode.UInt32: return "nox::uint32";
				case TypeCode.Int64: return "nox::int64";
				case TypeCode.UInt64: return "nox::uint64";
				case TypeCode.Single: return "float";
				case TypeCode.Double: return "double";
				case TypeCode.Char: return "nox::char16";
			}

			Nox.Util.Assert(false, "not found primitive type:{0}", typeCode.ToString());
			return "";
		}

		public static (string, PropertyTypeKind) CreateRuntimeTypeFQN(System.Type type)
		{
			if (type.IsPrimitive)
			{
				return (GetPrimitiveTypeName(Type.GetTypeCode(type)), PropertyTypeKind.Primitive);
			}

			{
				var propAttr = type.GetCustomAttribute<Core.RuntimeRemote.RuntimeRemoteCodeNativeFQNAttribute>();
				if (propAttr != null)
				{
					return (propAttr.FQN, PropertyTypeKind.FixedArray);
				}
			}

			//	rutnime wrapper
			{
				Core.Attributes.RuntimeWrapperAttribute? attr = type.GetCustomAttribute<Core.Attributes.RuntimeWrapperAttribute>();
				if (attr != null)
				{
					return attr.FQN;
				}
			}

			{
				if (TryGetListElementType(type, out Type? elementType) == true && elementType != null)
				{
					return $"nox::Vector<{CreateRuntimeTypeFQN(elementType)}>";
				}
			}

			//	不明な型
			Nox.Util.Assert(false, "不明な型です: " + type.FullName);
			return "";
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
		#endregion
	}
}
