
namespace RuntimeTypeDB
{
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
		Pointer,
		Reference,
	}

	public enum RecordAttributeFlag
	{
		Class,
		Struct,
		Union,
	}

	public class TypeInfo
	{
		public string Name { get; init; }
		public string FullName { get; init; }
		public string Namespace { get; init; }
		public int Size { get; init; }
		public int Alignment { get; init; }
	}

	public class NamespaceInfo
	{

	}

	public class RecordInfo
	{
		public TypeInfo UnderlyingType { get; init; }
	}

	public class VariableInfo
	{

	}

	public class FunctionInfo
	{

	}

	public class EnumInfo
	{

	}

	public readonly struct TypeDB
	{

	}

	public static class Util
	{
		private static string GetPath(string platform, string configuration)
		{
			string fileName = "RuntimeTypeDB.bin";

			string directory = $"D:\\github\\Nox\\runtime\\build\\RuntimeTypeDB\\{platform}\\{configuration}\\";

			return $"{directory}{fileName}";
		}

		internal static void Serialize(TypeDB data, string platform, string configuration)
		{
			string path = GetPath(platform, configuration);
			using (System.IO.FileStream fs = new System.IO.FileStream(path, System.IO.FileMode.Create))
			{
		//		MessagePack.MessagePackSerializer.Serialize(fs, data);
			}
		}

		/// <summary>
		/// デシリアライズ
		/// </summary>
		/// <param name="platform">プラットフォーム</param>
		/// <param name="configuration">構成</param>
		/// <returns></returns>
		public static TypeDB Deserialize(string platform, string configuration)
		{
			string path = GetPath(platform, configuration);
			using (System.IO.FileStream fs = new System.IO.FileStream(path, System.IO.FileMode.Open))
			{
				return default;
			//	return MessagePack.MessagePackSerializer.Deserialize<RuntimeNamespaceDeclData>(fs);
			}
		}
	}
}
