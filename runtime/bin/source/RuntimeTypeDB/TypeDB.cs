// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

namespace ReflectionGenerator.RuntimeTypeDB;

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

		public required bool FixedUnderlyingType { get; set; } = false;
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

	/// <summary>
	/// TypeDB バイナリの読み書き
	/// </summary>
	/// <remarks>
	/// 以前は %TEMP%\RuntimeTypeDB.bin という 1 台に 1 つしかない固定パスだった。
	/// 同じリポジトリの複数 worktree、あるいは同じツリーの Debug/Release を同時にビルドすると
	/// 互いのファイルを上書きしてしまうため、リフレクション前処理データ
	/// (Nox.CustomTask.Util.GetBinFilePath) と同じ方針に揃える。
	///
	///   * 実体はソースツリーごとの生成出力ディレクトリ (ReflectionGenerator.exe の -out) の下
	///   * プラットフォーム/構成をファイル名に含める
	///     runtime/reflection_generated/gen/RuntimeTypeDB.&lt;Platform&gt;.&lt;Configuration&gt;.bin
	///
	/// 読み手 (Editor) はビルドしたツリーの場所を知らないので、書き手が %TEMP% に
	/// 「実体の在り処だけを書いた」ポインタファイルを毎回置き直す。読み手は
	/// 明示指定 → 環境変数 → ポインタファイル の順で解決する。
	/// </remarks>
	public static class Util
	{
		private const string FILE_BASE_NAME = "RuntimeTypeDB";
		private const string FILE_EXTENSION = ".bin";

		/// <summary>
		/// %TEMP% に置く、実体のフルパスだけを書いたテキストファイルの拡張子
		/// </summary>
		private const string POINTER_FILE_EXTENSION = ".path.txt";

		/// <summary>
		/// 実体の置き場所を明示的に上書きする環境変数。ポインタファイルより優先される
		/// </summary>
		public const string OUTPUT_DIRECTORY_ENVIRONMENT_VARIABLE = "NOX_RUNTIME_TYPEDB_DIR";

		private static string GetFileName(ReadOnlySpan<char> platform, ReadOnlySpan<char> configuration)
		{
			return string.Concat(FILE_BASE_NAME, ".", platform.ToString(), ".", configuration.ToString(), FILE_EXTENSION);
		}

		/// <summary>
		/// TypeDB バイナリの実体のパスを組み立てる
		/// </summary>
		/// <param name="outputGenerateDir">生成出力ディレクトリ (ReflectionGenerator.exe の -out)</param>
		public static string GetFilePath(string outputGenerateDir, ReadOnlySpan<char> platform, ReadOnlySpan<char> configuration)
		{
			if (string.IsNullOrEmpty(outputGenerateDir) == true)
			{
				throw new ArgumentException("outputGenerateDir が空です。", nameof(outputGenerateDir));
			}

			return System.IO.Path.GetFullPath(System.IO.Path.Combine(outputGenerateDir, GetFileName(platform, configuration)));
		}

		/// <summary>
		/// 実体の在り処を指すポインタファイルのパス
		/// </summary>
		public static string GetPointerFilePath(ReadOnlySpan<char> platform, ReadOnlySpan<char> configuration)
		{
			string fileName = string.Concat(FILE_BASE_NAME, ".", platform.ToString(), ".", configuration.ToString(), POINTER_FILE_EXTENSION);
			return System.IO.Path.GetFullPath(System.IO.Path.Combine(System.IO.Path.GetTempPath(), fileName));
		}

		/// <summary>
		/// 読み手向けのパス解決。明示指定 → 環境変数 → ポインタファイル の順
		/// </summary>
		/// <returns>解決できなければ null</returns>
		public static string? ResolveFilePath(string? outputGenerateDir, ReadOnlySpan<char> platform, ReadOnlySpan<char> configuration)
		{
			if (string.IsNullOrEmpty(outputGenerateDir) == false)
			{
				return GetFilePath(outputGenerateDir!, platform, configuration);
			}

			string? environmentDirectory = Environment.GetEnvironmentVariable(OUTPUT_DIRECTORY_ENVIRONMENT_VARIABLE);
			if (string.IsNullOrEmpty(environmentDirectory) == false)
			{
				return GetFilePath(environmentDirectory!, platform, configuration);
			}

			string pointerFilePath = GetPointerFilePath(platform, configuration);
			if (System.IO.File.Exists(pointerFilePath) == false)
			{
				return null;
			}

			string filePath = System.IO.File.ReadAllText(pointerFilePath).Trim();
			return string.IsNullOrEmpty(filePath) ? null : filePath;
		}

		/// <summary>
		/// シリアライズ
		/// </summary>
		/// <param name="outputGenerateDir">生成出力ディレクトリ (ReflectionGenerator.exe の -out)</param>
		public static void Serialize(TypeDB data, string outputGenerateDir, ReadOnlySpan<char> platform, ReadOnlySpan<char> configuration)
		{
			string path = GetFilePath(outputGenerateDir, platform, configuration);

			System.IO.Directory.CreateDirectory(System.IO.Path.GetDirectoryName(path)!);

			using (System.IO.FileStream fs = new System.IO.FileStream(path, System.IO.FileMode.Create))
			{
				MessagePack.MessagePackSerializer.Serialize(fs, data);
			}

			WritePointerFile(path, platform, configuration);
		}

		/// <summary>
		/// デシリアライズ (読み手がツリーの場所を知らない場合)
		/// </summary>
		/// <param name="platform">プラットフォーム</param>
		/// <param name="configuration">構成</param>
		public static TypeDB? Deserialize(ReadOnlySpan<char> platform, ReadOnlySpan<char> configuration)
		{
			return Deserialize(null, platform, configuration);
		}

		/// <summary>
		/// デシリアライズ
		/// </summary>
		/// <param name="outputGenerateDir">生成出力ディレクトリ。null/空なら環境変数とポインタファイルから解決する</param>
		public static TypeDB? Deserialize(string? outputGenerateDir, ReadOnlySpan<char> platform, ReadOnlySpan<char> configuration)
		{
			string? path = ResolveFilePath(outputGenerateDir, platform, configuration);
			if (path == null)
			{
				System.Console.WriteLine(
					$"TypeDB file not found: 場所を解決できませんでした。ReflectionGeneratorでビルドするか、環境変数 {OUTPUT_DIRECTORY_ENVIRONMENT_VARIABLE} に生成出力ディレクトリを設定してください。");
				return null;
			}

			if (System.IO.File.Exists(path) == false)
			{
				System.Console.WriteLine($"TypeDB file not found: {path}");
				return null;
			}

			using (System.IO.FileStream fs = new System.IO.FileStream(path, System.IO.FileMode.Open))
			{
				return MessagePack.MessagePackSerializer.Deserialize<TypeDB>(fs);
			}
		}

		/// <summary>
		/// %TEMP% のポインタファイルを置き直す
		/// </summary>
		/// <remarks>
		/// 中身は実体のフルパス 1 行だけ。データそのものではないので、
		/// 複数ツリーが取り合っても壊れるものは無い (最後にビルドしたツリーが指される)。
		/// 特定のツリーへ固定したいときは環境変数で上書きする。
		/// </remarks>
		private static void WritePointerFile(string filePath, ReadOnlySpan<char> platform, ReadOnlySpan<char> configuration)
		{
			try
			{
				System.IO.File.WriteAllText(GetPointerFilePath(platform, configuration), filePath);
			}
			catch (System.IO.IOException)
			{
				//	他のビルドが同時に書いていても実体は無事なので、ここは落とさない
			}
			catch (UnauthorizedAccessException)
			{
			}
		}
	}
