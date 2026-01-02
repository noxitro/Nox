
namespace ReflectionGenerator.RuntimeTypeDB
{
	//	NOTE:	Enum定義関連は、ReflectionGenerator/Parser/CppParser2.csと合わせる

	file static class Local
	{
	}

	public enum AccessLevel : byte
	{
		Private,
		Protected,
		Public
	}

	//	元の定義:CppParser2.TypeKind
	public enum RuntimeTypeKind : byte
	{
		Invalid,
		Void,
		Bool,
		Char,
		SignedChar,
		UnsignedChar,
		Char8,
		Char16,
		Char32,
		WChar16,

		Int8,
		UInt8,
		Int16,
		Uint16,
		Int32,
		UInt32,
		Int64,
		UInt64,

		Long,
		UnsignedLong,

		Float,
		Double,
		Enum,

		Class,
		Struct,
		Union,
		BoundedArray,
		UnboundedArray,
		Pointer,
		LValueReference,
		RValueReference,

		// void()
		Function,
		// void(Class::*)(), int Class::*
		MemberPointer,

		TypeAlias,
		Nullptr,
		Auto,
	}

	public enum TypeAttributeFlag : uint
	{
		None,
		LValueReference = 1 << 0,
		RValueReference = 1 << 1,
		Const = 1 << 2,
		Volatile = 1 << 3,
		Anonymous = 1 << 4,
		Elaborated = 1 << 5,
	}

	public enum RecordAttributeFlag : ushort
	{
		None,
		Class = 1 << 0,
		Struct = 1 << 1,
		Union = 1 << 2,
		Anonymous = 1 << 3,
	}

	public enum FunctionAttributeFlag : uint
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
		Delete = 1 << 16,
		OutOfLine = 1 << 17,
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

	//	元の定義:CppParser2.AttrKind
	public enum AttributeKind : byte
	{
		Invalid,
		Annotate,
		EngineAnnotate,
		/// <summary>
		/// ReflectionGenerator用EngineAnnotate属性
		/// </summary>
		ReflectionTarget,
		IgnoreReflectionTarget,
		NoDiscard,
		Standard,
	}

	[MessagePack.MessagePackObject(true)]
	public struct AttributeDecl
	{
		public AttributeKind Kind { get; set; }
		public string AttrName { get; set; } = string.Empty;
		public string Value { get; set; } = string.Empty;

		public AttributeDecl() { }
	}

	#region 型情報
	[MessagePack.MessagePackObject(true)]
	public class TypeInfo
	{
		public RuntimeTypeKind Kind { get; set; }
		public string Name { get; set; } = string.Empty;
		public string FullName { get; set; } = string.Empty;
		public string Namespace { get; set; } = string.Empty;
		public long Size { get; set; }
		public long Alignment { get; set; }

		public TypeInfo[] ArgumentTypeList { get; set; } = [];
		public TypeInfo ReturnType { get; set; } = TypeInfo.Invalid;

		public TypeInfo UnderlyingTypeInfo { get; set; } = TypeInfo.Invalid;
		public TypeInfo PointeeTypeInfo { get; set; } = TypeInfo.Invalid;

		public DeclBase Decl { get; init; } = RuntimeInvalidDecl.Invalid;


		public static TypeInfo Invalid { get; } = new TypeInfo();

	}
	#endregion

	#region 宣言情報
	[MessagePack.MessagePackObject(true)]
	public class DeclBase { }

	file class RuntimeInvalidDecl : DeclBase
	{
		public static readonly RuntimeInvalidDecl Invalid = new ();
	}

	[MessagePack.MessagePackObject(true)]
	public class NamedDecl : DeclBase
	{
		public string Name { get; set; } = string.Empty;
		public string FullName { get; set; } = string.Empty;
		public AccessLevel AccessLevel { get; set; }
		public AttributeDecl[] AttributeList { get; set; } = [];
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
	public class TypeDecl : NamedDecl
	{
		public string Namespace { get; set; } = string.Empty;
	}

	[MessagePack.MessagePackObject(true)]
	public partial class FunctionDecl : TypeDecl
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
		public ArgumentInfo[] ArgumentList { get; set; } = [];
		public int NumDefaultArgument { get; set; } = 0;
	}

	[MessagePack.MessagePackObject(true)]
	public class EnumDecl : TypeDecl
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
		public EnumeratorInfo[] EnumeratorInfoList { get; set; } = [];
	}

	[MessagePack.MessagePackObject(true)]
	public class RecordDecl : TypeDecl
	{
		public required TypeInfo TypeInfo { get; set; }
		public RecordDecl[] RecordList { get; set; } = [];
		public VariableDecl[] VariableList { get; set; } = [];
		public FunctionDecl[] FunctionList { get; set; } = [];
		public EnumDecl[] EnumList { get; set; } = [];
	}

	[MessagePack.MessagePackObject(true)]
	public class VariableDecl : TypeDecl
	{
		public TypeInfo Type { get; set; } = TypeInfo.Invalid;
		public VariableAttributeFlag VariableAttributeFlags { get; set; } = VariableAttributeFlag.None;
		public long OffsetBits { get; set; } = 0;
		public int BitFieldWidth { get; set; } = 0;
	}
	#endregion

	[MessagePack.MessagePackObject(true)]
	public class TypeDB
	{
		public NamespaceDecl[] NamespaceList { get; set; } = [];
	}

	public static class Util
	{
		private const string FileName = "RuntimeTypeDB.bin";
		private const int MaxPathLength = 1024;

		private static string GetPath(ReadOnlySpan<char> platform, ReadOnlySpan<char> configuration)
		{
			ReadOnlySpan<char> directory = System.IO.Path.GetTempPath();

			string path = $"{directory}/{FileName}";
			return System.IO.Path.GetFullPath(path);
		}

		private static ReadOnlySpan<char> GetPath2(Span<char> dest, ReadOnlySpan<char> platform, ReadOnlySpan<char> configuration)
		{
			// GetTempPath は string を返すのでここだけはヒープ確保
			string directoryString = System.IO.Path.GetTempPath();
			ReadOnlySpan<char> directory = directoryString.AsSpan();
			ReadOnlySpan<char> fileSpan = FileName.AsSpan();

			int requiredLen = directory.Length + fileSpan.Length;
			if (requiredLen > dest.Length)
				throw new ArgumentException("dest が短すぎます。", nameof(dest));

			directory.CopyTo(dest);
			fileSpan.CopyTo(dest[directory.Length..]);

			return dest[..requiredLen];
		}

		public static void Serialize(TypeDB data, ReadOnlySpan<char> platform, ReadOnlySpan<char> configuration)
		{
			Span<char> pathBuffer = stackalloc char[MaxPathLength];

			ReadOnlySpan<char> path = GetPath2(pathBuffer, platform, configuration);

			using (System.IO.FileStream fs = new System.IO.FileStream(path.ToString(), System.IO.FileMode.Create))
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
		public static TypeDB? Deserialize(ReadOnlySpan<char> platform, ReadOnlySpan<char> configuration)
		{
			Span<char> pathBuffer = stackalloc char[MaxPathLength];

			ReadOnlySpan<char> path = GetPath2(pathBuffer, platform, configuration);
			using (System.IO.FileStream fs = new System.IO.FileStream(path.ToString(), System.IO.FileMode.Open))
			{
				return MessagePack.MessagePackSerializer.Deserialize<TypeDB>(fs);
			}
		}
	}
}
