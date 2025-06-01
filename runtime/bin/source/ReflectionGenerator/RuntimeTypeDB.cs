using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

//	解析後バイナリデータとして保存する型情報
//	ツール側はこのデータを読み込む

namespace ReflectionGenerator
{
	public static class RuntimeTypeDB
	{
		#region 型定義
		public enum RuntimeTypeKind : byte
		{
			Invalid,
			Int8,
			Int16,
			Int32,
			Int64,
			UInt8,
			UInt16,
			UInt32,
			UInt64,
			Float,
			Double,
			Bool,
			Char,
			Char8,
			Char16,
			Char32,
			Void,
			Class,
			Array,
		}

		[System.Flags]
		public enum RuntimeTypeQualifier : byte
		{
			None,
			Const = 1 << 0,
			Volatile = 1 << 1,
		}

		[MessagePack.MessagePackObject(true)]
		public struct RuntimeTypeData
		{
			public string Name { get; set; }
			public string FullName { get; set; }
			public uint Size { get; set; }
			public uint Alignment { get; set; }
			public ushort ArrayRank { get; set; }
			public ushort ArrayExtent { get; set; }
			public RuntimeTypeKind Kind { get; set; }
		}

		[MessagePack.MessagePackObject(true)]
		public struct RuntimeAttributeData
		{
			public string FullName { get; set; }
		}

		[MessagePack.MessagePackObject(true)]
		public struct RuntimeClassData
		{
			public string FullName { get; set; }
			public string Namespace { get; set; }
			public RuntimeTypeData TypeData { get; set; }
			public RuntimeClassData[] ClassList { get; set; }
			public RuntimeVariableData[] VariableList { get; set; }
			public RuntimeVariableData[] FunctionList { get; set; }
			public RuntimeEnumData[] EnumList { get; set; }
		}

		[MessagePack.MessagePackObject(true)]
		public struct RuntimeVariableData
		{
			public string Name { get; set; }
			public RuntimeTypeData TypeData { get; set; }
		}

		[MessagePack.MessagePackObject(true)]
		public struct RuntimeFunctionArgumentData
		{
			public string Name { get; set; }
			public RuntimeTypeData TypeData { get; set; }
			public RuntimeAttributeData[] AttributeList { get; set; }

			public bool HasDefaultValue { get; set; }
		}

		[MessagePack.MessagePackObject(true)]
		public struct RuntimeFunctionData
		{
			public string Name { get; set; }
			public RuntimeTypeData TypeData { get; set; }
			public RuntimeFunctionArgumentData[] ArgumentList { get; set; }

			public RuntimeAttributeData Attribute { get; set; }
		}

		[MessagePack.MessagePackObject(true)]
		public struct RuntimeEnumeratorData
		{
			public bool IsUnsigned { get; set; }
			public long Value { get; set; }
			public ulong UnsignedValue { get; set; }
			public string Name { get; set; }
		}

		[MessagePack.MessagePackObject(true)]
		public struct RuntimeEnumData
		{
			public string Name { get; set; }
			public string FullName { get; set; }
			public string Namespace { get; set; }

			public RuntimeEnumeratorData[] EnumeratorList { get; set; }
			public RuntimeTypeData UnderlyingType { get; set; }
		}

		[System.Serializable]
		public struct RuntimeNamespaceDeclData
		{
			/// <summary>
			/// 名前空間
			/// グローバル定義の場合は、空文字列
			/// </summary>
			public string Namespace { get; set; }

			public RuntimeNamespaceDeclData[] NamespaceDeclList { get; set; }
			public RuntimeClassData[] ClassList { get; set; }
			public RuntimeVariableData[] VariableList { get; set; }
			public RuntimeFunctionData[] FunctionList { get; set; }
			public RuntimeEnumData[] EnumList { get; set; }
		}

		public struct TypeDB
		{
		}
		#endregion

		#region 非公開
		private static string GetPath(string platform, string configuration)
		{
			string fileName = "RuntimeTypeDB.bin";

			string directory = $"D:\\github\\Nox\\runtime\\build\\RuntimeTypeDB\\{platform}\\{configuration}\\";

			return $"{directory}{fileName}";
		}

		#endregion

		#region 公開

		internal static void Serialize(RuntimeNamespaceDeclData data, string platform, string configuration)
		{
			string path = GetPath(platform, configuration);
			using (System.IO.FileStream fs = new System.IO.FileStream(path, System.IO.FileMode.Create))
			{
				MessagePack.MessagePackSerializer.Serialize(fs, data);
			}
		}

		/// <summary>
		/// デシリアライズ
		/// </summary>
		/// <param name="platform">プラットフォーム</param>
		/// <param name="configuration">構成</param>
		/// <returns></returns>
		public static RuntimeNamespaceDeclData Desirialize(string platform, string configuration)
		{
			string path = GetPath(platform, configuration);
			using (System.IO.FileStream fs = new System.IO.FileStream(path, System.IO.FileMode.Open))
			{
				return MessagePack.MessagePackSerializer.Deserialize<RuntimeNamespaceDeclData>(fs);
			}
		}
		#endregion
	}
}