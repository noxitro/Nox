
namespace RuntimeTypeDB
{
	file static class Local
	{
	}

	public enum AccessLevel : byte
	{
		Private,
		Protected,
		Public
	}

	public enum RuntimeTypeKind : byte
	{
		Invalid,
		Void,
		Bool,
		Char,
		Char8,
		Char16,
		Char32,
		WChar16,

		Int8,
		Uint8,
		Int16,
		Uint16,
		Int32,
		Uint32,
		Int64,
		Uint64,

		Float,
		Double,

		Class,
		Array,
		Pointer,
		Reference,
	}

	public enum TypeAttributeFlag : uint
	{

	}

	public enum RecordAttributeFlag : ushort
	{
		None,
		Class=1<<0,
		Struct=1<<1,
		Union=1<<2,
	}

	public enum FunctionAttributeFlag : ushort
	{
		None,
		Noexcept = 1 << 0,
		Const = 1 << 1,
		Constexpr = 1 << 2,
		Inline = 1 << 3,
		LValueReference = 1 << 4,
		RValueReference = 1 << 5,
		Volatile = 1 << 6,
		Virtual = 1 << 7,
		Abstract = 1 << 8,
		Consteval = 1 << 9,

		DefaultConstructor = 1 << 10,
		CopyConstructor = 1 << 11,
		MoveConstructor = 1 << 12,
		Destructor = 1 << 13,
		Static = 1 << 14,
		Explicit = 1 << 15,
	}

	public enum VariableAttributeFlag : ushort
	{
		None,
		Constexpr = 1 << 0,
		Static = 1 << 1,
		Constinit = 1 << 2,
		ThreadLocal_Static = 1 << 3,
		ThreadLocal_Dynamic = 1 << 4,
		Mutable = 1 << 5,
	}

	public enum AttributeKind : byte
	{
		Invalid,
		Annotate,
		EngineAnnotate,
	}

	[MessagePack.MessagePackObject(true)]
	public struct AttributeDecl
	{
		public AttributeKind Kind { get; set; }
		public string AttrName { get; set; } = string.Empty;
		public string Value { get; set; } = string.Empty;

		public AttributeDecl() { }
	}

	[MessagePack.MessagePackObject(true)]
	public class DeclBase { }

	[MessagePack.MessagePackObject(true)]
	public class NamedDecl : DeclBase
	{
		public string Name { get; set; } = string.Empty;
		public string FullName { get; set; } = string.Empty;
		public AccessLevel AccessLevel { get; set; }
		public AttributeDecl[] AttributeList { get; set; } = [];
	}

	[MessagePack.MessagePackObject(true)]
	public class TypeInfo
	{
		public RuntimeTypeKind Kind { get; set; }
		public AttributeDecl AttrKind { get; set; }
		public string Name { get; set; } = string.Empty;
		public string FullName { get; set; } = string.Empty;
		public string Namespace { get; set; } = string.Empty;
		public int Size { get; set; }
		public int Alignment { get; set; }

		public TypeInfo[] ArgumentTypeList { get; set; } = [];
		public TypeInfo ReturnType { get; set; } = TypeInfo.Invalid;

		public static TypeInfo Invalid { get; } = new TypeInfo();
	}

	[MessagePack.MessagePackObject(true)]
	public partial class FunctionDecl : NamedDecl
	{
		[MessagePack.MessagePackObject(true)]
		public struct ArgumentInfo
		{
			public string Name { readonly get; set; } = string.Empty;
			public bool IsDefault { readonly get; set; } = false;
			public TypeInfo TypeInfo { readonly get; set; } = TypeInfo.Invalid;
			public AttributeDecl[] AttributeList { readonly get; set; } = [];

			public ArgumentInfo() { }
		}

		public TypeInfo TypeInfo { get; set; } = TypeInfo.Invalid;
		public FunctionAttributeFlag FunctionAttributeFlags { get; set; } = FunctionAttributeFlag.None;
		public ArgumentInfo[] ArgumentList { private get; set; } = [];
		public int NumDefaultArgument { get; set; } = 0;
	}

	[MessagePack.MessagePackObject(true)]
	public class EnumDecl : NamedDecl
	{
		[MessagePack.MessagePackObject(true)]
		public struct EnumeratorInfo
		{
			public string Name { readonly get; set; } = string.Empty;
			public bool IsUnsigned { readonly get; set; } = false;
			public long Int64 { readonly get; set; } = 0;
			public AttributeDecl[] AttributeList { readonly get; set; } = [];

			public readonly ulong Uint64 => unchecked((ulong)Int64);
			public EnumeratorInfo() { }
		}

		public required TypeInfo TypeInfo { get; set; }
		public required TypeInfo UnderlyingTypeInfo { get; set; }
		public EnumeratorInfo[] EnumeratorInfoList { get; set; } = [];
	}

	[MessagePack.MessagePackObject(true)]
	public class NamespaceDecl : NamedDecl
	{
		public NamespaceDecl[] NamespaceList { get; set; } = [];
		public RecordDecl[] RecordList { get; set; } = [];
		public VariableDecl[] VariableList { get; set; } = [];
		public FunctionDecl[] FunctionList { get; set; } = [];
		public EnumDecl[] EnumList { get; set; } = [];
	}

	[MessagePack.MessagePackObject(true)]
	public class RecordDecl : NamedDecl
	{
		public required TypeInfo TypeInfo { get; set; }
		public RecordDecl[] RecordList { get; set; } = [];
		public VariableDecl[] VariableList { get; set; } = [];
		public FunctionDecl[] FunctionList { get; set; } = [];
		public EnumDecl[] EnumList { get; set; } = [];
	}

	[MessagePack.MessagePackObject(true)]
	public class VariableDecl
	{
		public TypeInfo Type { get; set; } = TypeInfo.Invalid;
		public VariableAttributeFlag VariableAttributeFlags { get; set; } = VariableAttributeFlag.None;
		public long OffsetBits { get; set; } = 0;
		public int BitFieldWidth { get; set; } = 0;
	}

	[MessagePack.MessagePackObject(true)]
	public class TypeDB
	{
		public NamespaceDecl[] NamespaceList { get; set; } = [];
	}

	public static class Util
	{
		private static string GetPath(ReadOnlySpan<char> platform, ReadOnlySpan<char> configuration)
		{
			ReadOnlySpan<char> fileName = "RuntimeTypeDB.bin";
			ReadOnlySpan<char> directory = System.IO.Path.GetTempPath();

			string path = $"{directory}/{fileName}";
			return System.IO.Path.GetFullPath(path);
		}

		public static void Serialize(TypeDB data, string platform, string configuration)
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
		public static TypeDB? Deserialize(string platform, string configuration)
		{
			string path = GetPath(platform, configuration);
			using (System.IO.FileStream fs = new System.IO.FileStream(path, System.IO.FileMode.Open))
			{
				return MessagePack.MessagePackSerializer.Deserialize<TypeDB>(fs);
			}
		}
	}
}
