
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
	}
}
