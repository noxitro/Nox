using ReflectionGenerator.Parser;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;

namespace ReflectionGenerator.Parser2
{
	public static class ParseExtensions
	{
		public static bool IsAnyOn(this Parser2.TypeAttributeFlag self, Parser2.TypeAttributeFlag flags)
		{
			return (self & flags) != 0;
		}

		public static bool IsAnyOn(this Parser2.RecordAttributeFlag self, Parser2.RecordAttributeFlag flags)
		{
			return (self & flags) != 0;
		}

		public static bool IsAnyOn(this Parser2.VariableAttributeFlag self, Parser2.VariableAttributeFlag flags)
		{
			return (self & flags) != 0;
		}

		public static bool IsAnyOn(this Parser2.FunctionAttributeFlag self, Parser2.FunctionAttributeFlag flags)
		{
			return (self & flags) != 0;
		}

		public static bool IsOn(this Parser2.TypeAttributeFlag self, Parser2.TypeAttributeFlag flag)
		{
			return (self & flag) == flag;
		}

		public static bool IsOn(this Parser2.RecordAttributeFlag self, Parser2.RecordAttributeFlag flag)
		{
			return (self & flag) == flag;
		}

		public static bool IsOn(this Parser2.VariableAttributeFlag self, Parser2.VariableAttributeFlag flag)
		{
			return (self & flag) == flag;
		}

		public static bool IsOn(this Parser2.FunctionAttributeFlag self, Parser2.FunctionAttributeFlag flag)
		{
			return (self & flag) == flag;
		}
	}

	file static class Extension
	{
		public static string GetFQN(this ClangSharp.Interop.CXCursor cursor, bool skipAnonymouse=true)
		{
			string fqn = GetName(cursor);
			cursor = cursor.SemanticParent;

			while (cursor.IsNull == false)
			{
				switch (cursor.kind)
				{
					case ClangSharp.Interop.CXCursorKind.CXCursor_ClassDecl:
					case ClangSharp.Interop.CXCursorKind.CXCursor_UnionDecl:
					case ClangSharp.Interop.CXCursorKind.CXCursor_StructDecl:
						if (skipAnonymouse && cursor.IsAnonymous)
						{
						//	break;
						}
						fqn = $"{GetName(cursor)}::{fqn}";
						break;

					case ClangSharp.Interop.CXCursorKind.CXCursor_Namespace:
						if (skipAnonymouse && cursor.IsAnonymous)
						{
						//	break;
						}
						fqn = $"{cursor.Spelling.CString}::{fqn}";
						break;

					default:
						//						Util.Assert(false, $"Unsupported cursor kind '{cursor.kind}' found while getting FQN.");
						break;
				}

				cursor = cursor.SemanticParent;
			}


			if (fqn.Contains("nox::reflection::detail::CompileTimeTypeImpl<nox::Behavior"))
			{
				Util.BreakPoint();
			}

			return fqn;
		}

		public static string GetFQN(this in ClangSharp.Interop.CXType type)
		{
			switch (type.TypeClass)
			{
				case ClangSharp.Interop.CX_TypeClass.CX_TypeClass_Builtin:
					return type.Spelling.CString;

				case ClangSharp.Interop.CX_TypeClass.CX_TypeClass_Enum:
				case ClangSharp.Interop.CX_TypeClass.CX_TypeClass_Record:
					return type.Declaration.GetFQN();
				default:
					return type.CanonicalType.Spelling.CString;
			}
		}

		private static string GetName(this in ClangSharp.Interop.CXType type)
		{
			return GetName(type.Declaration);
		}

		private static bool HasPackTemplateArgument(this ClangSharp.Interop.CXCursor cursor)
		{
			int n = cursor.NumTemplateArguments;
			for (uint i = 0; i < n; ++i)
			{
				var ta = cursor.GetTemplateArgument(i);
				if (ta.kind == ClangSharp.Interop.CXTemplateArgumentKind.CXTemplateArgumentKind_Pack)
					return true;
			}
			return false;
		}

		private static string GetNameTemplateArgument(in ClangSharp.Interop.CX_TemplateArgument templateArgument)
		{
			switch (templateArgument.kind)
			{
				case ClangSharp.Interop.CXTemplateArgumentKind.CXTemplateArgumentKind_Type:
					return GetFQN(templateArgument.AsType.CanonicalType);

				case ClangSharp.Interop.CXTemplateArgumentKind.CXTemplateArgumentKind_Integral:
					return templateArgument.AsIntegral.ToString();

				case ClangSharp.Interop.CXTemplateArgumentKind.CXTemplateArgumentKind_Declaration:
					return templateArgument.AsType.CanonicalType.Spelling.CString;

				case ClangSharp.Interop.CXTemplateArgumentKind.CXTemplateArgumentKind_Pack:
					string name = "";
					for (int packElementIndex = 0, numPackElements = templateArgument.NumPackElements; packElementIndex < numPackElements; ++packElementIndex)
					{
						var packElemengt = templateArgument.GetPackElement((uint)packElementIndex);
						if (packElementIndex > 0)
						{
							name += ", ";
						}
						name += GetNameTemplateArgument(packElemengt);
					}
					return name;

				default:
					Util.Assert(false);
					break;
			}
			return string.Empty;
		}

		private static string GetNameWithTemplateSpecialization(ClangSharp.Interop.CXCursor cursor)
		{
			// 追加: 引数にパックを含む場合は正規化綴りにフォールバック
			if (cursor.HasPackTemplateArgument())
			{
			//	return cursor.Type.CanonicalType.Spelling.CString;
			}

			int numTemplateArgument = cursor.NumTemplateArguments;

			switch(cursor.kind)
			{
				case ClangSharp.Interop.CXCursorKind.CXCursor_ClassTemplatePartialSpecialization:
					break;
			}

			ClangSharp.Interop.CXCursor rootSpecializedCursorTemplate = cursor.SpecializedCursorTemplate;
			while(true)
			{
				var tmp = rootSpecializedCursorTemplate.SpecializedCursorTemplate;
				if (tmp.IsNull == true)
				{
					break;
				}
				rootSpecializedCursorTemplate = tmp;
			}

			Util.Assert(rootSpecializedCursorTemplate.TemplatedDecl.IsNull == false, "TemplatedDecl is null");
			string name = $"{cursor.Spelling.CString}<";
			for (uint i = 0; i < numTemplateArgument; ++i)
			{
				if (i > 0)
				{
					name += ", ";
				}

				ClangSharp.Interop.CX_TemplateArgument templateArgument = cursor.GetTemplateArgument(i);
				name += GetNameTemplateArgument(templateArgument);
			}

			name += ">";
			return name;
		}

		private static string GetName(this in ClangSharp.Interop.CXCursor cursor)
		{
			switch(cursor.kind)
			{
				case ClangSharp.Interop.CXCursorKind.CXCursor_FunctionDecl:
				case ClangSharp.Interop.CXCursorKind.CXCursor_FunctionTemplate:
				case ClangSharp.Interop.CXCursorKind.CXCursor_CXXMethod:
					return cursor.Spelling.CString;
			}

			switch (cursor.DeclKind)
			{
				case ClangSharp.Interop.CX_DeclKind.CX_DeclKind_ClassTemplateSpecialization:
					return GetNameWithTemplateSpecialization(cursor);

				default:
					return cursor.DisplayName.CString;
			}
		}

		public static bool HasLambdaType(this in ClangSharp.Interop.CXType type)
		{
			return HasLambdaType(type.GetFQN());
		}

		public static bool HasLambdaType(ReadOnlySpan<char> s)
		{
			return s.Contains("(lambda at ", StringComparison.OrdinalIgnoreCase);
		}

		public static DeclCategory GetDeclCategory(this ClangSharp.Interop.CXCursor cursor)
		{
			// 参照としてのカーソルか？
			if (cursor.IsReference)
			{
				return DeclCategory.Reference;
			}

			string usr = cursor.GetNormalizedUsr();

			// 定義（関数本体 / クラス本体 / 変数定義 等）
			if (cursor.IsDefinition)
			{
				return DeclCategory.Definition;
			}

			// 宣言（前方宣言 or 再宣言）
			if (cursor.IsDeclaration)
			{
				// USR が取れない場合は分類不能
				if (string.IsNullOrEmpty(usr))
				{
					return DeclCategory.Unknown;
				}

				return DeclCategory.Unknown;
			}

			// 上記以外は分類不能
			return DeclCategory.Unknown;
		}

		public static string GetNormalizedUsr(this ClangSharp.Interop.CXCursor cursor)
		{
			string usr = cursor.Usr.CString;
			if (string.IsNullOrEmpty(usr)) return string.Empty;

			int len = usr.Length;
			System.Text.StringBuilder? sb = null;
			int i = 0;

			while (i < len)
			{
				// 次の '>' を探す
				int gt = usr.IndexOf('>', i);
				if (gt < 0 || gt + 2 >= len)
				{
					break; // 以降にパターンは存在しない
				}

				if (usr[gt + 1] != '#')
				{
					i = gt + 1;
					continue;
				}

				// '#'+digits をスキップ
				int j = gt + 2;
				while (j < len && (uint)(usr[j] - '0') <= 9) j++;

				// 末尾が '$' なら >#<digits>$ を >#$ に正規化
				if (j < len && usr[j] == '$')
				{
					sb ??= new System.Text.StringBuilder(len);

					// 直前の未処理部分をコピー
					if (gt > i)
					{
						sb.Append(usr, i, gt - i);
					}

					// 正規化トークンを追加
					sb.Append(">#$");

					// 次の探索開始位置（'$' の次）
					i = j + 1;
					continue;
				}

				// パターン不一致、探索を継続
				i = gt + 1;
			}

			// 置換が一度も無ければ元文字列を返す（アロケーション回避）
			if (sb is null) return usr;

			// 末尾を追加
			if (i < len)
			{
				sb.Append(usr, i, len - i);
			}

			return sb.ToString();
		}
#if DEBUG
		private static uint _HashCount = 0;
		private static readonly Dictionary<uint, uint> _DebugHashSet = new();
		public static uint MakeDebugHashCode(this in ClangSharp.Interop.CXCursor cursor)
		{
			if (_DebugHashSet.TryGetValue(cursor.Hash, out uint hash) == true)
			{
				return hash;
			}

			hash = System.Threading.Interlocked.Increment(ref _HashCount);
			_DebugHashSet.Add(cursor.Hash, hash);
			return hash;
		}
#endif
	}

	file static class Local
	{
		public static ReflectionGenerateKind GetReflectionGenerateKind(ReadOnlySpan<AttributeDecl> attributeList)
		{
			for (int i = 0, length = attributeList.Length; i < length; ++i)
			{
				AttributeDecl attribute = attributeList[i];

				switch(attribute.AttrKind)
				{
					case AttrKind.ReflectionTarget:
						return ReflectionGenerateKind.Reflection;
					case AttrKind.IgnoreReflectionTarget:
						return ReflectionGenerateKind.IgnoreReflection;
				}
			}

			return ReflectionGenerateKind.None;
		}

		public static TypeKind ToTypeKind(in ClangSharp.Interop.CXTypeKind kind)
		{
			return kind switch
			{
				ClangSharp.Interop.CXTypeKind.CXType_Invalid => TypeKind.Invalid,
				ClangSharp.Interop.CXTypeKind.CXType_Void => TypeKind.Void,
				ClangSharp.Interop.CXTypeKind.CXType_Bool => TypeKind.Bool,

				ClangSharp.Interop.CXTypeKind.CXType_Int => TypeKind.Int32,
				ClangSharp.Interop.CXTypeKind.CXType_Long => TypeKind.Int64,
				_ => TypeKind.Invalid,
			};
		}

	}

	#region メタ情報
	public readonly record struct UniqueDeclKey
	{
		public required string Usr { get; init; }
		public required int LocationHash { get; init; }
	}

	public readonly record struct MetaInfo
	{
		public required string ProjectName { get; init; }

		public required string SourceFilePath { get; init; }
		public required uint SourceLine { get; init; }
		public required uint SourceColumn { get; init; }

		public required UniqueDeclKey UniqueDeclKey { get; init; }

		public MetaInfo()
		{
			_SourceLocation = $"{SourceFilePath}:{SourceLine.ToString()}:{SourceColumn.ToString()}";
		}

		private readonly string _SourceLocation = string.Empty;
		public readonly ReadOnlySpan<char> SourceLocation => $"{SourceFilePath}:{SourceLine.ToString()}:{SourceColumn.ToString()}";
	}

	#endregion

	#region 型情報
	public enum DeclCategory
	{
		Unknown,        // 初回前方宣言など（定義でも再宣言でも参照でもない）
		Definition,     // 本体定義
		Redeclaration,  // 既存宣言または既存定義後の再宣言
		Reference       // 参照のみ（型/メンバ/関数参照）
	}

	/// <summary>
	/// 宣言を含むコンテナのインターフェース
	/// </summary>
	public interface IDeclarationContainer
	{
		List<RecordDecl> RecordList { get; }
		List<FunctionDecl> FunctionList { get; }
		List<VariableDecl> VariableList { get; }
		List<EnumDecl> EnumList { get; }
		List<TypeAliasDecl> TypeAliasList { get; }
	}

	public enum TypeKind : byte
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

	public enum AttrKind : byte
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

	public enum ReflectionGenerateKind : byte
	{
		None,
		Reflection,
		IgnoreReflection,
		PrivateReflection
	}

	public enum TypeAttributeFlag : ushort
	{
		None,
		LValueReference = 1 << 0,
		RValueReference = 1 << 1,
		Const = 1 << 2,
		Volatile = 1 << 3,
		Anonymous = 1 << 4,
	}

	public enum RecordAttributeFlag : ushort
	{
		None,
		Class = 1 << 0,
		Struct = 1 << 1,
		Union = 1 << 2,
		Anonymous = 1 << 3,
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

	public struct TypeData
	{

	}

	public struct TypeAlias
	{
		public string Name { get; init; }
		public string FullName { get; init; }
	}

	public abstract class TypeInfo
	{
		public required string Name { get; init; }
		public required string FullName { get; init; }
		public required string Namespace { get; init; }
		public uint DeclHash { get; init; } = 0;

		public required TypeKind TypeKind { get; init; }
		public required TypeAttributeFlag TypeAttributeFlags { get; init; }

		/// <summary>
		/// 
		/// </summary>
		public List<TypeAlias> _TypeDefList { get; } = new List<TypeAlias>();

		//      public static readonly TypeInfo Invalid = new TypeInfo() {
		//          Name = string.Empty,
		//          FullName = string.Empty,
		//          Namespace = string.Empty,
		//	DeclHash = 0,
		//	TypeAttributeFlags = TypeAttributeFlag.None,
		//	TypeKind = TypeKind.Invalid,
		//};
	}

	public sealed class PrimitiveTypeInfo : TypeInfo
	{

	}
	public sealed class InvalidTypeInfo : TypeInfo
	{
	}

	public sealed class UnknownTypeInfo : TypeInfo
	{
	}

	public class TypeAliasInfo : TypeInfo
	{
		public required TypeInfo PointeeType { get; init; }
	}

	public class ElaboratedTypeInfo : TypeInfo
	{
		public required TypeInfo PointeeType { get; init; }
	}

	public class PointerTypeInfo : TypeInfo
	{
		public required TypeInfo PointeeType { get; init; }
	}
	public class MemberPointerType : TypeInfo
	{
		public required TypeInfo PointeeType { get; init; }
	}

	public class FunctionTypeInfo : TypeInfo
	{
		public required TypeInfo[] ArgumentTypeList { get; init; }
		public required TypeInfo ReturnType { get; init; }
	}

	public class RecordTypeInfo : TypeInfo
	{
	}

	public class EnumTypeInfo : TypeInfo
	{
	}
	#endregion

	#region 宣言情報
	[System.Diagnostics.DebuggerDisplay("{DebuggerDisplay,nq}")]
    public class DeclBase 
    {
		public required string Usr { get; init; }

		/// <summary>
		/// ClangSharpのハッシュ
		/// </summary>
		public required uint DeclHash
		{
			get => _DeclHash;
			init
			{
				_DeclHash = value;
				Hash = value.ToString();
			}
		}
		public required uint ParentDeclHash { get; init; }
		private readonly MetaInfo _Meta;
		public required MetaInfo Meta
		{
			get => _Meta;
			init => _Meta = value;
		}
		public ref readonly MetaInfo GetMeta() => ref _Meta;

		public string Hash { get; private init; } = string.Empty;
		public ReflectionGenerateKind ReflectionGenerateKind { get; set; } = ReflectionGenerateKind.None;

		public DeclCategory DeclCategory { get; init; } = DeclCategory.Unknown;

		public override string ToString() => $"{GetType().Name} Hash:{Hash} ParentHash:{ParentDeclHash}";

#if DEBUG
		public required uint DebugHashCode { get; init; }
		protected virtual string DebuggerDisplay => ToString();
#endif
        #region フィールド
        private readonly uint _DeclHash;

		#endregion
	}

	public class NoFoundDecl : DeclBase
    {
#if DEBUG

#endif
	}

	public interface ISpecializationDecl
	{

	}


	public class NamedDecl : DeclBase
    {
        public required string Name { get; init; }
        public required string FullName { get; init; }
		public required AccessLevel AccessLevel { get; init; } 
		public required AttributeDecl[] AttributeList
		{
			private get => _AttributeList;
			init
			{
				_AttributeList = value;
				ReflectionGenerateKind = Local.GetReflectionGenerateKind(_AttributeList);
				if (IsDefaultEnabledReflection && ReflectionGenerateKind == ReflectionGenerateKind.None)
				{
					ReflectionGenerateKind = ReflectionGenerateKind.Reflection;
				}
			}
		}
		public ReadOnlySpan<AttributeDecl> AttributeSpan => _AttributeList;

		protected readonly AttributeDecl[] _AttributeList = Array.Empty<AttributeDecl>();

        public override string ToString() => FullName;
		protected virtual bool IsDefaultEnabledReflection => true;
    }

    public class TypeDecl : NamedDecl
    {
        public required string Namespace { get; init; }
	}

	public class NamespaceDecl : NamedDecl, IDeclarationContainer
	{
		public List<NamespaceDecl> NamespaceList { get; } = [];
        public List<RecordDecl> RecordList { get; } = [];
        public List<FunctionDecl> FunctionList { get; } = [];
        public List<VariableDecl> VariableList { get; } = [];
        public List<EnumDecl> EnumList { get; } = [];
        public List<TypeAliasDecl> TypeAliasList { get; } = [];
    }

    public class NamespaceNode : IDeclarationContainer
    {
        public required string Name { get; init; }
        public required string FullName { get; init; }

		public List<NamespaceNode> Children { get; } = [];

		public List<NamespaceDecl> NamespaceDeclList { get; } = [];

        public List<RecordDecl> RecordList
        {
            get
            {
                List<RecordDecl> classList = new List<RecordDecl>();
                foreach (NamespaceDecl child in NamespaceDeclList)
                {
                    classList.AddRange(child.RecordList);
                }
                return classList;
            }
        }
        public List<FunctionDecl> FunctionList
        {
            get
            {
                List<FunctionDecl> functionList = new List<FunctionDecl>();
                foreach (NamespaceDecl child in NamespaceDeclList)
                {
                    functionList.AddRange(child.FunctionList);
                }
                return functionList;
            }
        }

        public List<VariableDecl> VariableList
        {
            get
            {
                List<VariableDecl> variableList = new List<VariableDecl>();
                foreach (NamespaceDecl child in NamespaceDeclList)
                {
                    variableList.AddRange(child.VariableList);
                }
                return variableList;
            }
        }

        public List<EnumDecl> EnumList
        {
            get
            {
                List<EnumDecl> enumList = new List<EnumDecl>();
                foreach (NamespaceDecl child in NamespaceDeclList)
                {
                    enumList.AddRange(child.EnumList);
                }
                return enumList;
            }
		}

        public List<TypeAliasDecl> TypeAliasList
        {
            get
            {
				List<TypeAliasDecl> enumList = [];
                foreach (NamespaceDecl child in NamespaceDeclList)
                {
                    enumList.AddRange(child.TypeAliasList);
                }
                return enumList;
            }
        }
    }

	public class RecordDecl : TypeDecl, IDeclarationContainer
	{
		public List<RecordDecl> RecordList { get; } = new List<RecordDecl>();
		public List<FunctionDecl> FunctionList { get; } = new List<FunctionDecl>();
		public List<VariableDecl> VariableList { get; } = new List<VariableDecl>();
		public List<EnumDecl> EnumList { get; } = new List<EnumDecl>();
		public List<TypeAliasDecl> TypeAliasList { get; } = [];
		public List<FriendDecl> FriendList { get; } = [];
        public required BaseSpecifierDecl[] BaseList { get; init; } = [];
		public ReadOnlySpan<BaseSpecifierDecl> BaseSpan => BaseList;
		public RecordDecl? ParentRecordDecl { get; set; } = null;
        public required RecordAttributeFlag RecordAttributeFlags { get; init; }
		public bool IsReflectionClass { get; set; }
		public required bool IsNoxObject { get; init; }
		protected override bool IsDefaultEnabledReflection => false;
	}

    public class TypeAliasDecl : TypeDecl
    {
        public required string PointeeDeclUsr { get; init; }
        public required uint PointeeDeclHash { get; init; } = 0;
		private readonly MetaInfo _PointeeMeta;
		public required MetaInfo PointeeMeta
		{
			init => _PointeeMeta = value;
		}
		public ref readonly MetaInfo GetPointeeMeta() => ref _PointeeMeta;
		public required TypeInfo PointeeType { get; init; }
	}

	public class TemplateTypeAliasDecl : TypeAliasDecl
	{
		public struct TemplateArgumentInfo
		{

		}

		public required TemplateArgumentInfo[] TemplateArgumentList { private get; init; }
		public ReadOnlySpan<TemplateArgumentInfo> TemplateArgumentSpan => TemplateArgumentList;
		public required int NumDefaultArgument { get; init; }
	}

	public class FriendDecl : DeclBase
	{

	}

	public class BaseSpecifierDecl : TypeDecl
    {
        public required uint PointeeDeclHash { get; init; }
        public required string PointeeDeclUsr { get; init; }
		private readonly MetaInfo _PointeeMeta;
		public required MetaInfo PointeeMeta
		{
			init => _PointeeMeta = value;
		}
		public ref readonly MetaInfo GetPointeeMeta() => ref _PointeeMeta;

		public required bool IsVirtualBase { get; init; }
    }

	public class TemplateClassDecl : RecordDecl
	{
		public struct TemplateArgumentInfo
		{
			public required string Name { get; init; }
			public required TypeInfo TypeInfo { get; init; }
			public required bool IsType { get; init; }
			public required bool IsNonType { get; init; }
			public required bool IsTemplate { get; init; }

			public required bool IsDefault { get; init; }
		}

		public required TemplateArgumentInfo[] TemplateArgumentList { private get; init; }
		public ReadOnlySpan<TemplateArgumentInfo> TemplateArgumentSpan => TemplateArgumentList;
		public required int NumDefaultArgument { get; init; }
	}

	public class FunctionDecl : TypeDecl
	{
		public readonly struct ArgumentInfo
		{
			public required string Name { readonly get; init; }
			public required bool IsDefault { readonly get; init; }
			public required TypeInfo TypeInfo { readonly get; init; }
			public required AttributeDecl[] AttributeList { readonly get; init; }

			public ArgumentInfo() { }
		}

		public required FunctionTypeInfo TypeInfo { get; init; }
		public required FunctionAttributeFlag FunctionAttributeFlags { get; init; }
		public required ArgumentInfo[] ArgumentList { private get; init; } = [];
		public required int NumDefaultArgument { get; init; }

		public ReadOnlySpan<ArgumentInfo> ArgumentSpan => ArgumentList;
	}

	public class VariableDecl: TypeDecl
	{
		public required TypeInfo Type { get; init; }
		public required VariableAttributeFlag VariableAttributeFlags { get; init; }
		public required long OffsetBits { get; init; }
		public required int BitFieldWidth { get; init; }
	}

    public class EnumDecl : TypeDecl
	{
        public readonly struct EnumeratorInfo
        {
			#region 公開プロパティ
			/// <summary>
			/// 名前
			/// </summary>
			public required string Name { readonly get; init; }

			public required bool IsUnsigned { readonly get; init; }

			public required long Int64 { readonly get; init; }
			public readonly ulong Uint64 => unchecked((ulong)Int64);

			/// <summary>
			/// 属性リスト
			/// </summary>
			public required AttributeDecl[] AttributeList { readonly get; init; }

			public readonly ReadOnlySpan<AttributeDecl> AttributeSpan => AttributeList;
			#endregion
		}

		public required EnumTypeInfo TypeInfo { get; init; }
		public required TypeInfo UnderlyingTypeInfo { get; init; }
		public EnumeratorInfo[] EnumeratorInfoList { get; init; } = [];
		public ReadOnlySpan<EnumeratorInfo> EnumeratorSpan => EnumeratorInfoList;
	}

	public class AttributeDecl
	{
		// <summary>
		/// ClangSharpのハッシュ
		/// </summary>
		public required uint DeclHash
		{
			get => _DeclHash;
			init
			{
				_DeclHash = value;
				Hash = value.ToString();
			}
		}
		public string Hash { get; private init; } = string.Empty;

		public required AttrKind AttrKind { get; init; }
        public required string AttrName { get; init; }
        public required string Value { get; init; }

		private readonly uint _DeclHash;
	}
	#endregion

	/// <summary>
	/// Cpp解析
	/// </summary>
	public unsafe class CppParser
    {
		#region クラス定義
		public readonly struct SetupDesc
		{
			/// <summary>
			/// 解析対象ファイルパス
			/// </summary>
			public required string SourceFilePath { get; init; }

			/// <summary>
			/// 
			/// </summary>
			public required string SolutionPath { get; init; }

			/// <summary>
			/// 
			/// </summary>
			public required string ProjectFilePath { get; init; }


			/// <summary>
			/// プラットフォーム名
			/// </summary>
			public required string Platform { get; init; }

			/// <summary>
			/// ビルド構成
			/// </summary>
			public required string Configuration { get; init; }

			/// <summary>
			/// MSBuildのパス
			/// </summary>
			public required string MSBuildBinPath { get; init; }

			/// <summary>
			/// c++バージョン
			/// clang用の定義で入っている(-std=c++2bなど)
			/// </summary>
			public required string CppVersion { get; init; }

			/// <summary>
			/// 最適化オプション
			/// </summary>
			public required string Optimization { get; init; }

			/// <summary>
			/// プリプロセッサマクロ定義群
			/// ;区切りで入っている
			/// </summary>
			public required string PreprocessorMacro { get; init; }

			/// <summary>
			/// 解析対象外のroot namespaceリスト
			/// </summary>
			public required IReadOnlyList<string> IgnoreNamespaceList { get; init; }

			/// <summary>
			/// 解析対象のnamespace
			/// nullの場合、全てが対象
			/// </summary>
			public required IReadOnlyList<string> EnableRootNamespaceList { get; init; }

			/// <summary>
			/// 追加インクルードディレクトリ
			/// </summary>
			public required string AdditionalIncludeDirectories { get; init; }

			/// <summary>
			/// 追加オプション
			/// </summary>
			public required string AdditionalOptions { get; init; }

			/// <summary>
			/// 実行時型情報を使用するか
			/// </summary>
			public required bool UseRTTI { get; init; }

			/// <summary>
			/// モジュールごとのヘッダーファイルリスト
			/// </summary>
			public required IReadOnlyDictionary<string, IReadOnlyList<string>> IncludeHeaderListWithArtifact { get; init; }

			/// <summary>
			/// 中間ディレクトリ
			/// </summary>
			public required string IntermediateOutputPath { get; init; }
		}
		#endregion

		#region フィールド
		private const string REFLECTION_OBJECT_FQN = "nox::reflection::ReflectionObject";

        private string _ProjectRootDirectory = string.Empty;

		private readonly Dictionary<int, TypeInfo> _TypeDict = new();
        private NamespaceNode NamespaceNodeRoot => _NamespaceNodeDict[string.Empty];
		private readonly Dictionary<string, NamespaceNode> _NamespaceNodeDict = new Dictionary<string, NamespaceNode>();
		/// <summary>
		/// key:Cursor.Hash, Value:DeclBase
		/// </summary>
        private readonly Dictionary<uint, DeclBase> _DeclWithHashDict = new();
        private readonly Dictionary<UniqueDeclKey, DeclBase> _DeclWithUsrDict = new();
		private readonly Dictionary<string, List<NamespaceDecl>> _NamespaceDeclListWithProjectName = new();
		private readonly List<NamespaceDecl> _NamespaceDeclList = new();
		private readonly HashSet<uint> _ResolvedHashSet = new();
		#endregion

		#region 公開プロパティ
		public IReadOnlyList<NamespaceDecl> NamespaceDeclList => _NamespaceDeclList;
		public IReadOnlyDictionary<string, List<NamespaceDecl>> NamespaceDeclListWithProjectNameDict => _NamespaceDeclListWithProjectName;
		#endregion

		#region 公開メソッド
		public void DumpTrace()
        {
            //MergedNamespace rootNamespaceDecl = _NamespaceDeclDict[string.Empty];
            probeNamespaceDecl(NamespaceNodeRoot, 0);

            static void traceDepth(int depth)
            {
                Span<char> tab = stackalloc char[depth];
                for (int i = 0; i < depth; i++)
                {
                    tab[i] = '\t';
                }
                Trace.Info(null, tab);
            }

            static void probe(IDeclarationContainer container, int depth)
            {
                foreach (RecordDecl decl in container.RecordList)
                {
                    traceDepth(depth);
                    Trace.InfoLine(null, $"[Class] {decl.FullName}");
                    probe(decl, depth);
                }

                foreach (FunctionDecl decl in container.FunctionList)
                {
                    traceDepth(depth);
                    Trace.InfoLine(null, $"[Function] {decl.FullName}");
                }

                foreach (VariableDecl decl in container.VariableList)
                {
                    traceDepth(depth);
                    Trace.InfoLine(null, $"[Variable] {decl.FullName}");
                }

                foreach (EnumDecl decl in container.EnumList)
                {
                    traceDepth(depth);
                    Trace.InfoLine(null, $"[Enum] {decl.FullName}");
                }
            }

            static void probeNamespaceDecl(NamespaceNode node, int depth)
            {
				traceDepth(depth);
				Trace.InfoLine(null, $"[Namespace] {node.FullName}");
				probe(node, depth);

				foreach (NamespaceNode child in node.Children)
                {
                    if(child.FullName.Contains("nox") == false)
                    {
                        continue;
                    }

                    probeNamespaceDecl(child, depth+1);
				}
			}
		}

        public bool Parse(in SetupDesc setupParam)
		{
			_ProjectRootDirectory = System.IO.Path.GetDirectoryName(setupParam.SolutionPath) ?? string.Empty;

			ClangSharp.Interop.CXIndex rootIndex = default;
			ClangSharp.Interop.CXTranslationUnit translationUnit = default;

			bool success = Parse(in setupParam, ref rootIndex, ref translationUnit);

			rootIndex.Dispose();
			translationUnit.Dispose();

			return success;
		}

		private bool Parse(in SetupDesc setupParam, ref ClangSharp.Interop.CXIndex rootIndex, ref ClangSharp.Interop.CXTranslationUnit translationUnit)
        {
			{
				//  パースするソースファイルを作成
#if false
                string? parseSourceFilePath = CreateParseSourceFile(setupParam.SolutionPath, setupParam.IncludeHeaderListWithArtifact);
                if(parseSourceFilePath == null)
                {
                    Trace.Error(this, "パース用ソースファイルの作成に失敗しました");
                    return false;
                }
#endif
				string parseSourceFilePath = setupParam.SourceFilePath;

				//  引数
				List<string> parseCommandLineList = new()
				{
					 "-fsyntax-only",
					//"-w",
					"-fno-caret-diagnostics",
					"-fno-spell-checking",
				};

				//  解析時にのみ有効にするマクロ
				parseCommandLineList.Add($"-D {Define.RUNTIME_REFLECTION_GENERATOR_DEFINE}");

				//  カスタムタスクで解析した情報をセット
				parseCommandLineList.Add(setupParam.CppVersion);

				//  ビルド構成 プラットフォーム

				//  最適化オプション
				//  msvcの定義からclangの定義に変換
				//string optimizationOption = setupParam.Optimization switch
				//{
				//	"Disabled" => "-O0",
				//	"MinSpace" => "-O1",
				//	"MaxSpeed" => "-O2",
				//	"Full" => "-O3",
				//	_ => string.Empty,
				//};

				//if (optimizationOption != string.Empty)
				//{
				//	parseCommandLineList.Add(optimizationOption);
				//}
				//else
				//{
				//	Trace.ErrorLine(null, $"不明な最適化オプションです:{setupParam.Optimization}");
				//}

				//  define
				ReadOnlySpan<string> macros = setupParam.PreprocessorMacro.Split(';', StringSplitOptions.RemoveEmptyEntries);
				foreach (string macro in macros)
				{
					parseCommandLineList.Add($"-D {macro}");
				}

				//  追加インクルードディレクトリ
				ReadOnlySpan<string> additionalIncludeDirectories = setupParam.AdditionalIncludeDirectories.Split(';', StringSplitOptions.RemoveEmptyEntries);
				foreach (string additionalIncludeDirectory in additionalIncludeDirectories)
				{
					parseCommandLineList.Add($"-I {additionalIncludeDirectory}");
				}

				//  end

				//Example.Exe(parseSourceFilePath, parseCommandLineList.ToArray());

				//  不明な属性を無視しない
				rootIndex = ClangSharp.Interop.CXIndex.Create(true, true);
				ClangSharp.Interop.CXErrorCode cxErrorCode;
				//  コンパイル
				//for (int i = 0; i < 10; ++i)
				{
					using (new ScopeProfiler() { Tag = "ClangCompile" })
					{
						cxErrorCode = ClangSharp.Interop.CXTranslationUnit.TryParse(
						 rootIndex,
						 parseSourceFilePath,
						 parseCommandLineList.ToArray(),
						 default,
						// 関数の中身を解析しないことで、高速化を試みる
						ClangSharp.Interop.CXTranslationUnit_Flags.CXTranslationUnit_SkipFunctionBodies |
						ClangSharp.Interop.CXTranslationUnit_Flags.CXTranslationUnit_PrecompiledPreamble |
						ClangSharp.Interop.CXTranslationUnit_Flags.CXTranslationUnit_CacheCompletionResults |
						ClangSharp.Interop.CXTranslationUnit_Flags.CXTranslationUnit_KeepGoing |
						ClangSharp.Interop.CXTranslationUnit_Flags.CXTranslationUnit_IgnoreNonErrorsFromIncludedFiles
						,
						 //  ClangSharp.Interop.CXTranslationUnit_Flags.CXTranslationUnit_None,
						 out translationUnit
						 );
					}

					if (cxErrorCode != ClangSharp.Interop.CXErrorCode.CXError_Success)
					{
						Trace.ErrorLine(this, "failed parse");
						return false;
					}
				}

				//  ビルドエラーの解析
				{
					bool isSuccess = true;
					for (uint i = 0; i < translationUnit.NumDiagnostics; ++i)
					{
						ClangSharp.Interop.CXDiagnostic diagnostic = translationUnit.GetDiagnostic(i);
						switch (diagnostic.Severity)
						{
							case ClangSharp.Interop.CXDiagnosticSeverity.CXDiagnostic_Error:
							case ClangSharp.Interop.CXDiagnosticSeverity.CXDiagnostic_Fatal:
								Trace.ErrorLine(this, diagnostic.Format(ClangSharp.Interop.CXDiagnosticDisplayOptions.CXDiagnostic_DisplaySourceLocation).CString);
								isSuccess = false;
								return false;
							case ClangSharp.Interop.CXDiagnosticSeverity.CXDiagnostic_Warning:
								Trace.WarningLine(this, diagnostic.Format(ClangSharp.Interop.CXDiagnosticDisplayOptions.CXDiagnostic_DisplaySourceLocation).CString);
								break;

							case ClangSharp.Interop.CXDiagnosticSeverity.CXDiagnostic_Note:
							case ClangSharp.Interop.CXDiagnosticSeverity.CXDiagnostic_Ignored:
								Trace.InfoLine(this, diagnostic.Format(ClangSharp.Interop.CXDiagnosticDisplayOptions.CXDiagnostic_DisplaySourceLocation).CString);
								break;
						}
					}

					if (isSuccess == false)
					{
						Trace.ErrorLine(this, "clang compile結果にError, Faitalが発生しています ログを確認してください");
						return false;
					}
				}

				ParseUnit(translationUnit);
				return true;
			}
		}

        public void ParseUnit(in ClangSharp.Interop.CXTranslationUnit translationUnit)
        {
            ClangSharp.Interop.CXCursor cursor = translationUnit.Cursor;
            uint hash = cursor.Hash;
            uint parentDeclHash = 0;

            NamespaceDecl namespaceDecl = new NamespaceDecl()
            {
				Usr = cursor.GetNormalizedUsr(),
                Name = string.Empty,
                FullName = string.Empty,
                DeclHash = hash,
                ParentDeclHash = parentDeclHash,
				Meta = default,
				AttributeList = CreateAttributeDeclList(cursor),
				AccessLevel = AccessLevel.Public,
				DeclCategory = cursor.GetDeclCategory(),
#if DEBUG
				DebugHashCode = cursor.MakeDebugHashCode(),
#endif
			};
            AddDecl(namespaceDecl);

            NamespaceNode node = _NamespaceNodeDict[""] = new NamespaceNode()
            {
                Name = string.Empty,
                FullName = string.Empty,
            };
            node.NamespaceDeclList.Add(namespaceDecl);

            List<(ClangSharp.Interop.CXCursor, long, int index)> msList = new();
            System.Diagnostics.Stopwatch sw = new();

            int visitCounter = 0;

			using (new ScopeProfiler() { Tag = "VisitChildren" })
			{
				cursor.VisitChildren(
					(ClangSharp.Interop.CXCursor cursor, ClangSharp.Interop.CXCursor parent, void* _) =>
					{
						if (visitCounter == 423)
						{
							Util.BreakPoint();
						}

						sw.Reset();
						sw.Start();
						VisitCursor(cursor);
						msList.Add((cursor, sw.ElapsedMilliseconds, visitCounter++));

						return ClangSharp.Interop.CXChildVisitResult.CXChildVisit_Recurse;
					}, default);
			}
            msList.Sort((x,y) => (int)y.Item2 - (int)x.Item2);

			// Top 10 slowest cursors
            Trace.InfoLine(null, "Top 10 slowest cursors:");
            int count = 0;
            foreach ((var c, long ms, int index) in msList)
            {
                if (count >= 10)
                {
                    break;
                }
                count++;
                Trace.InfoLine(null, $"[{index}]Cursor: {c.ToString()} - {ms}ms");
			}

			using (new ScopeProfiler() { Tag = "PostProcess" })
            {
                PostProcess();
            }
        }

        private void PostProcess()
        {
			foreach (DeclBase decl in _DeclWithHashDict.Values)
			{
				if (decl.ParentDeclHash == 0)
				{
					continue;
				}

				DeclBase parentDecl = _DeclWithHashDict[decl.ParentDeclHash];

				if (decl is NamespaceDecl namespaceDecl)
				{
					if (_NamespaceNodeDict.TryGetValue(namespaceDecl.FullName, out NamespaceNode? node) == false)
					{
						node = new NamespaceNode()
						{
							Name = namespaceDecl.Name,
							FullName = namespaceDecl.FullName,
						};
						_NamespaceNodeDict.Add(namespaceDecl.FullName, node);

						string parentNamespaceName;
						if (namespaceDecl.FullName.Contains("::") == false)
						{
							parentNamespaceName = string.Empty;
						}
						else
						{
							int index = namespaceDecl.FullName.LastIndexOf("::");
							parentNamespaceName = namespaceDecl.FullName.Substring(0, index);
						}

                        _NamespaceNodeDict[parentNamespaceName].Children.Add(node);
					}

					((NamespaceDecl)parentDecl).NamespaceList.Add(namespaceDecl);
					node.NamespaceDeclList.Add(namespaceDecl);
				}
				else
				{
					IDeclarationContainer container = (IDeclarationContainer)parentDecl;
					switch (decl)
					{
						case RecordDecl declImpl:
							container.RecordList.Add(declImpl);
							break;
						case FunctionDecl declImpl:
							container.FunctionList.Add(declImpl);
							break;
						case VariableDecl declImpl:
							container.VariableList.Add(declImpl);
							break;
						case EnumDecl declImpl:
							container.EnumList.Add(declImpl);
							break;
						case TypeAliasDecl declImpl:
							container.TypeAliasList.Add(declImpl);
							break;
                    }
				}
			}
		}

		private void ParseNamespaceDecl(in ClangSharp.Interop.CXCursor cursor)
        {
			string usr = cursor.GetNormalizedUsr();
			uint hash = cursor.Hash;
            if (ContainsDecl(hash) == true)
            {
                return;
			}

			if (cursor.IsDeleted == true || cursor.IsDefined == true || cursor.IsDefinition == false)
            {
                return;
            }

			Util.Assert(cursor.GetDeclCategory() == DeclCategory.Definition);

			uint parentDeclHash = GetParentDeclCursor(cursor).Hash;

			NamespaceDecl namespaceDecl;
			if (_DeclWithHashDict.TryGetValue(hash, out DeclBase? outDecl) == true)
            {
                namespaceDecl = (NamespaceDecl)outDecl;
			}
            else
            {
				namespaceDecl = new NamespaceDecl()
				{
					Usr = usr,
					Name = cursor.Spelling.CString,
					FullName = cursor.GetFQN(),
					DeclHash = hash,
					ParentDeclHash = parentDeclHash,
					Meta = CreateMetaData(cursor),
					AttributeList = CreateAttributeDeclList(cursor),
					AccessLevel = AccessLevel.Public,
					DeclCategory = cursor.GetDeclCategory(),
#if DEBUG
					DebugHashCode = cursor.MakeDebugHashCode(),
#endif
				};

				//	nox空間以外はデフォルトでリフレクション無効
				if (namespaceDecl.FullName.Contains("nox")==false)
				{
					namespaceDecl.ReflectionGenerateKind = ReflectionGenerateKind.IgnoreReflection;
				}

				if (_NamespaceDeclListWithProjectName.TryGetValue(namespaceDecl.Meta.ProjectName, out List<NamespaceDecl>? declList) == false)
				{
					_NamespaceDeclListWithProjectName.Add(namespaceDecl.Meta.ProjectName, declList = new List<NamespaceDecl>());
				}
				declList.Add(namespaceDecl);
				_NamespaceDeclList.Add(namespaceDecl);
				AddDecl(namespaceDecl);
			}

            cursor.VisitChildren(
                (child, parent, data) =>
                {
                    VisitCursor(child);
                    return ClangSharp.Interop.CXChildVisitResult.CXChildVisit_Continue;
                }
                , default
                );
        }

        private void ParseRecordDecl(in ClangSharp.Interop.CXCursor cursor)
        {
			string usr = cursor.GetNormalizedUsr();
			uint hash = cursor.Hash;
			if (ContainsDecl(hash) == true)
            {
                return;
            }

			if (cursor.IsDefined == true)
            {
                return;
            }

            if (cursor.IsDefinition == false)
            {
                return;
            }

			Util.Assert(cursor.GetDeclCategory() == DeclCategory.Definition);

			Util.Assert(usr != string.Empty);

			//  ラムダ式の場合はスキップ
			if (cursor.LambdaCallOperator != ClangSharp.Interop.CXCursor.Null)
            {
                return;
            }

            //  関数内定義の型はスキップ
            if (cursor.ParentFunctionOrMethod.IsNull == false)
            {
                return;
            }

			//	匿名構造体/クラスはスキップ
			if (cursor.IsAnonymous == true)
			{
			//	return;
			}

			var meta = CreateMetaData(cursor);
			if (ContainsDecl(meta.UniqueDeclKey) == true)
			{
				return;
			}

		//	Util.Assert(GetDeclList(usr).Count == 0, "既に同じUsrのDeclが存在しています");

			if (cursor.Type.kind != ClangSharp.Interop.CXTypeKind.CXType_Invalid)
            {
                GetOrCreateTypeInfo(cursor.Type);
            }

			uint parentDeclHash = GetParentDeclCursor(cursor).Hash;

            BaseSpecifierDecl[] baseTypeList = CreateBaseSpecifierDeclList(cursor, out bool isBaseReflectionClass, out bool inheritedFromNoxObject);

            if (ContainsDecl(hash) == true)
            {
                return;
            }

            string fqn = cursor.GetFQN();
			
			RecordDecl classDecl = new ()
            {
				Usr = usr,
				Name = cursor.Spelling.CString,
                FullName = fqn,
                Namespace = cursor.GetNamespace(),
				DeclHash = hash,
                ParentDeclHash = parentDeclHash,
                BaseList = baseTypeList,
				RecordAttributeFlags = GetRecordAttributeFlags(cursor),
				AttributeList = CreateAttributeDeclList(cursor),
				IsNoxObject = fqn == "nox::Object" || inheritedFromNoxObject,
				AccessLevel = cursor.CXXAccessSpecifier.GetAccessLevel(),
				Meta = meta,
#if DEBUG
				DebugHashCode = cursor.MakeDebugHashCode(),
#endif
			};
			AddDecl(classDecl);
		
            if(isBaseReflectionClass == false)
            {
                classDecl.IsReflectionClass = classDecl.FullName == REFLECTION_OBJECT_FQN;
			}
            else
            {
				if ((classDecl.IsReflectionClass = isBaseReflectionClass) == true)
				{
					if (classDecl.ReflectionGenerateKind != ReflectionGenerateKind.IgnoreReflection)
					{
						classDecl.ReflectionGenerateKind = ReflectionGenerateKind.Reflection;
					}
				}
			}

			for (uint i = 0, length = (uint)cursor.NumDecls; i < length; i++)
            {
                VisitCursor(cursor.GetDecl(i));
			}
        }

        private void ParseAliasDecl(in ClangSharp.Interop.CXCursor cursor)
        {
			string usr = cursor.GetNormalizedUsr();
			uint hash = cursor.Hash;
			if (ContainsDecl(hash) == true)
			{
				return;
			}

			if (cursor.IsDeleted == true || cursor.IsDefined == true || cursor.IsDefinition == false)
			{
				return;
			}

			//  関数内定義の型はスキップ
			if (cursor.ParentFunctionOrMethod.IsNull == false)
			{
				return;
			}

			ClangSharp.Interop.CXCursor pointeeCursor = cursor.Type.CanonicalType.Declaration;
			//Util.Assert(pointeeCursor.kind != ClangSharp.Interop.CXCursorKind.CXCursor_NoDeclFound, "no found decl");
			uint pointeeDeclHash = pointeeCursor.Hash;
            ClangSharp.Interop.CXCursor parentCursor = GetParentDeclCursor(cursor);

            TypeAliasDecl decl = new ()
            {
				Usr = usr,
				Name = cursor.Spelling.CString,
                FullName = cursor.GetFQN(),
                Namespace = cursor.GetNamespace(),
                DeclHash = hash,
                ParentDeclHash = parentCursor.Hash,
                PointeeType = GetOrCreateTypeInfo(cursor.Type.CanonicalType),
				PointeeDeclUsr = pointeeCursor.GetNormalizedUsr(),
                PointeeDeclHash = pointeeDeclHash,
				AttributeList = CreateAttributeDeclList(cursor),
				AccessLevel = cursor.CXXAccessSpecifier.GetAccessLevel(),
				Meta = CreateMetaData(cursor),
				PointeeMeta = CreateMetaData(pointeeCursor),
#if DEBUG
				DebugHashCode = cursor.MakeDebugHashCode(),
#endif
			};
            AddDecl(decl);

			if (decl.Name.Contains("conditional_t"))
			{
				Util.BreakPoint();
			}

            VisitCursor(cursor.Type.CanonicalType.Declaration);
            VisitCursor(parentCursor);

		}

		private void ParseTemplateAliasDecl(in ClangSharp.Interop.CXCursor cursor)
		{
			string usr = cursor.GetNormalizedUsr();
			uint hash = cursor.Hash;
			if (ContainsDecl(hash) == true)
			{
				return;
			}

			if (cursor.IsDeleted == true || cursor.IsDefined == true || cursor.IsDefinition == false)
			{
				return;
			}

			//  関数内定義の型はスキップ
			if (cursor.ParentFunctionOrMethod.IsNull == false)
			{
				return;
			}

			
			ClangSharp.Interop.CXCursor parentCursor = GetParentDeclCursor(cursor);

			TemplateTypeAliasDecl.TemplateArgumentInfo[] templateArgumentList = new TemplateTypeAliasDecl.TemplateArgumentInfo[cursor.NumTemplateParameterLists];
			for (uint listIndex = 0, listLength = (uint)templateArgumentList.Length; listIndex < listLength; ++listIndex)
			{
				int paramLength = cursor.GetNumTemplateParameters(listIndex);
				if (paramLength <= 0)
				{
					Util.BreakPoint();
					continue;
				}
				for (int paramIndex = 0; paramIndex < paramLength; ++paramIndex)
				{
					ClangSharp.Interop.CXCursor templateParamCursor = cursor.GetTemplateParameter(listIndex, (uint)paramIndex);
					//	TODO:	未実装
				}
			}
			int numDefaultArgument = 0;

			Util.Assert(!cursor.TemplatedDecl.IsNull, "template decl is null");

			//			uint pointeeDeclHash = cursor.Type.CanonicalType.Declaration.Hash;
			ClangSharp.Interop.CXCursor pointeeCursor = cursor.TemplatedDecl.UnderlyingDecl;
			uint pointeeDeclHash = pointeeCursor.Hash;


			TemplateTypeAliasDecl decl = new ()
			{
				Usr = usr,
				Name = cursor.Spelling.CString,
				FullName = cursor.GetFQN(),
				Namespace = cursor.GetNamespace(),
				DeclHash = hash,
				PointeeDeclUsr = pointeeCursor.GetNormalizedUsr(),
				ParentDeclHash = parentCursor.Hash,
				PointeeType = GetOrCreateTypeInfo(pointeeCursor.Type),
				PointeeDeclHash = pointeeDeclHash,
				AttributeList = CreateAttributeDeclList(cursor),
				AccessLevel = cursor.CXXAccessSpecifier.GetAccessLevel(),
				TemplateArgumentList = templateArgumentList,
				NumDefaultArgument = numDefaultArgument,
				Meta = CreateMetaData(cursor),
				PointeeMeta = CreateMetaData(pointeeCursor),
#if DEBUG
				DebugHashCode = cursor.MakeDebugHashCode(),
#endif
			};
			AddDecl(decl);

			VisitCursor(cursor.Type.CanonicalType.Declaration);
			VisitCursor(parentCursor);
		}

        private void ParseBaseSpecifierDecl(in ClangSharp.Interop.CXCursor cursor)
        {
			string usr = cursor.GetNormalizedUsr();
			uint hash = cursor.Hash;
			if (ContainsDecl(hash) == true)
			{
				return;
			}

			if (cursor.IsDeleted == true || cursor.IsDefined == true)
            {
                return;
            }

            ClangSharp.Interop.CXCursor referCursor = cursor.Referenced;

			BaseSpecifierDecl decl = new BaseSpecifierDecl()
            {
				Usr = usr,
				Name = cursor.Spelling.CString,
                FullName = cursor.Definition.GetFQN(),
                Namespace = cursor.GetNamespace(),
                DeclHash = hash,
                ParentDeclHash = 0,
				PointeeDeclUsr = referCursor.GetNormalizedUsr(),
                PointeeDeclHash = referCursor.Hash,
				AttributeList = [],
				IsVirtualBase = cursor.IsVirtualBase,
				AccessLevel = cursor.CXXAccessSpecifier.GetAccessLevel(),
				Meta = CreateMetaData(cursor),
				PointeeMeta = CreateMetaData(referCursor),
#if DEBUG
				DebugHashCode = cursor.MakeDebugHashCode(),
#endif
			};

            AddDecl(decl);

			VisitCursor(referCursor);
		}

		private void ParseEnumDecl(in ClangSharp.Interop.CXCursor cursor)
		{
			if (cursor.IsDeleted == true || cursor.IsDefined == true || cursor.IsDefinition == false)
			{
				return;
			}

			if (cursor.IsAnonymous == true)
			{
		//		return;
			}

			string usr = cursor.GetNormalizedUsr();
			uint hash = cursor.Hash;
			if (ContainsDecl(hash) == true)
			{
				return;
			}

			int numEnumerator = cursor.NumEnumerators;
			EnumDecl.EnumeratorInfo[] enumeratorInfoList = new EnumDecl.EnumeratorInfo[numEnumerator];

			uint parentDeclHash = GetParentDeclCursor(cursor).Hash;

			EnumDecl decl = new EnumDecl()
							{
				Usr = usr,
				Name = cursor.Spelling.CString,
				FullName = cursor.GetFQN(),
				Namespace = cursor.GetNamespace(),
				DeclHash = cursor.Hash,
				ParentDeclHash = parentDeclHash,
				EnumeratorInfoList = enumeratorInfoList,
				AttributeList = CreateAttributeDeclList(cursor),
				TypeInfo = GetOrCreateTypeInfo< EnumTypeInfo>(cursor.Type),
				UnderlyingTypeInfo = GetOrCreateTypeInfo(cursor.EnumDecl_IntegerType),
				AccessLevel = cursor.CXXAccessSpecifier.GetAccessLevel(),
				Meta = CreateMetaData(cursor),
#if DEBUG
				DebugHashCode = cursor.MakeDebugHashCode(),
#endif
			};

			AddDecl(decl);

			for (uint i = 0; i < numEnumerator; ++i)
			{
				ClangSharp.Interop.CXCursor enumCursor = cursor.GetEnumerator(i);

				long integer64;
				if (enumCursor.IsUnsigned)
				{
					integer64 = unchecked((long)enumCursor.EnumConstantDeclUnsignedValue);
				}
				else
				{
					integer64 = enumCursor.EnumConstantDeclValue;
				}

				enumeratorInfoList[i] = new EnumDecl.EnumeratorInfo()
				{
					IsUnsigned = enumCursor.IsUnsigned,
					Int64 = integer64,
					AttributeList = CreateAttributeDeclList(cursor),
					Name = enumCursor.Spelling.CString
				};
			}
		}

		private void ParseFriendDecl(in ClangSharp.Interop.CXCursor cursor)
		{
			if (cursor.ParentFunctionOrMethod.IsNull == false)
			{
				return;
			}

			string usr = cursor.GetNormalizedUsr();
			uint hash = cursor.Hash;
			if (ContainsDecl(hash) == true)
			{
				return;
			}

			uint parentHash = cursor.ParentFunctionOrMethod.Hash;
			ClangSharp.Interop.CXCursor parentCursor = GetParentDeclCursor(cursor);
			VisitCursor(parentCursor);

			FriendDecl decl = new FriendDecl()
			{
				Usr = usr,
				DeclHash = cursor.Hash,
				ParentDeclHash = parentCursor.Hash,
				Meta = CreateMetaData(cursor),
#if DEBUG
				DebugHashCode = cursor.MakeDebugHashCode(),
#endif
			};
			AddDecl(decl);

			if (parentHash != 0)
			{
				ClangSharp.Interop.CXCursor fiendDecl = cursor.FriendDecl;

				switch(fiendDecl.kind)
				{
					case ClangSharp.Interop.CXCursorKind.CXCursor_ClassDecl:
						RecordDecl parentClassDecl = GetDecl<RecordDecl>(parentCursor.Hash);
						if(parentClassDecl.ReflectionGenerateKind != ReflectionGenerateKind.IgnoreReflection &&
							fiendDecl.GetFQN().Contains("nox::reflection::ReflectionGeneratedHolder") == true)
						{
							parentClassDecl.ReflectionGenerateKind = ReflectionGenerateKind.PrivateReflection;
						}
						break;
				}
				
			}
		}

		private void ParseNoDeclFound(in ClangSharp.Interop.CXCursor cursor)
		{
			string usr = cursor.GetNormalizedUsr();
			uint hash = cursor.Hash;
			if (ContainsDecl(hash) == true)
			{
				return;
			}

			AddDecl(new NoFoundDecl()
			{
				Usr = usr,
				DeclHash = cursor.Hash,
				ParentDeclHash = 0,
				Meta = CreateMetaData(cursor),
#if DEBUG
				DebugHashCode = cursor.MakeDebugHashCode(),
#endif
			}
			);
		}

		private void ParseTemplateRecordDecl(in ClangSharp.Interop.CXCursor cursor)
        {
			string usr = cursor.GetNormalizedUsr();
			uint hash = cursor.Hash;
			if (ContainsDecl(hash) == true)
			{
				return;
			}

			TemplateClassDecl.TemplateArgumentInfo[] templateArgumentList = new TemplateClassDecl.TemplateArgumentInfo[cursor.NumTemplateParameterLists];
			for(uint listIndex = 0, listLength = (uint)templateArgumentList.Length; listIndex < listLength; ++listIndex)
			{
				int paramLength = cursor.GetNumTemplateParameters(listIndex);
				if(paramLength <= 0)
				{
					Util.BreakPoint();
					continue;
				}
				for (int paramIndex = 0; paramIndex < paramLength; ++paramIndex)
				{
					ClangSharp.Interop.CXCursor templateParamCursor = cursor.GetTemplateParameter(listIndex, (uint)paramIndex);
					//	TODO:	未実装
				}
			}
			int numDefaultArgument = 0;

			BaseSpecifierDecl[] baseTypeList = CreateBaseSpecifierDeclList(cursor, out bool isBaseReflectionClass, out bool inheritedFromNoxObject);

            TemplateClassDecl classDecl = new ()
            {
				Usr = usr,
                Name = cursor.Spelling.CString,
                FullName = cursor.GetFQN(),
                Namespace = cursor.GetNamespace(),
                DeclHash = hash,
                ParentDeclHash = GetParentDeclCursor(cursor).Hash,
				RecordAttributeFlags = GetRecordAttributeFlags(cursor),
				AttributeList = CreateAttributeDeclList(cursor),
				TemplateArgumentList = templateArgumentList,
				NumDefaultArgument = numDefaultArgument,
				BaseList = baseTypeList,
				IsNoxObject = inheritedFromNoxObject,
				AccessLevel = cursor.CXXAccessSpecifier.GetAccessLevel(),
				Meta = CreateMetaData(cursor),
#if DEBUG
				DebugHashCode = cursor.MakeDebugHashCode(),
#endif
			};

            AddDecl(classDecl);

			int numSpecialization = cursor.NumSpecializations;
            for(uint i = 0; i < numSpecialization; i++)
            {
                ClangSharp.Interop.CXCursor specializationCursor = cursor.GetSpecialization(i);
                VisitCursor(specializationCursor);
			}
		}

		private void ParseFunctionDecl(in ClangSharp.Interop.CXCursor cursor)
		{
			string usr = cursor.GetNormalizedUsr();
			uint hash = cursor.Hash;
			if (ContainsDecl(hash) == true)
			{
				return;
			}

			if (cursor.IsDeleted == true || cursor.IsDefined == true)
			{
				return;
			}

			if (cursor.ParentFunctionOrMethod.IsNull == false)
			{
				//  関数内定義の変数はスキップ
				return;
			}

			uint parentDeclHash = GetParentDeclCursor(cursor).Hash;

			ClangSharp.Interop.CXType thisType = cursor.Type;
			ClangSharp.Interop.CXType returnType = cursor.ReturnType;

			FunctionTypeInfo thisTypeInfo = GetOrCreateTypeInfo<FunctionTypeInfo>(thisType);
			TypeInfo returnTypeInfo = GetOrCreateTypeInfo(returnType);

			int numArgument = cursor.NumArguments;
			int numDefaultArgument = 0;
			FunctionDecl.ArgumentInfo[] argumentList = new FunctionDecl.ArgumentInfo[numArgument];
			for (uint i = 0; i < numArgument; i++)
			{
				ClangSharp.Interop.CXCursor argumentCursor = cursor.GetArgument(i);

				argumentList[i] = new FunctionDecl.ArgumentInfo()
				{
					Name = argumentCursor.Spelling.CString,
					IsDefault = argumentCursor.HasDefaultArg,
					TypeInfo = GetOrCreateTypeInfo(argumentCursor.Type),
					AttributeList = CreateAttributeDeclList(argumentCursor),
				};

				if (argumentList[i].IsDefault)
				{
					++numDefaultArgument;
				}

			}

			FunctionAttributeFlag functionAttributeFlag = default;
			if (cursor.CXXMethod_IsStatic ||
				cursor.kind == ClangSharp.Interop.CXCursorKind.CXCursor_FunctionDecl ||
				cursor.kind == ClangSharp.Interop.CXCursorKind.CXCursor_FunctionTemplate
				)
			{
				functionAttributeFlag |= FunctionAttributeFlag.Static;
			}
			if (cursor.CXXMethod_IsVirtual)
			{
				functionAttributeFlag |= FunctionAttributeFlag.Virtual;
			}
			if (cursor.CXXMethod_IsPureVirtual)
			{
				functionAttributeFlag |= FunctionAttributeFlag.Abstract;
			}
			if (cursor.CXXMethod_IsConst)
			{
				functionAttributeFlag |= FunctionAttributeFlag.Const;
			}

			switch (thisType.ExceptionSpecificationType)
			{
				case ClangSharp.Interop.CXCursor_ExceptionSpecificationKind.CXCursor_ExceptionSpecificationKind_BasicNoexcept:
				case ClangSharp.Interop.CXCursor_ExceptionSpecificationKind.CXCursor_ExceptionSpecificationKind_ComputedNoexcept:
					functionAttributeFlag |= FunctionAttributeFlag.Noexcept;
					break;
			}
			if (cursor.IsConstexpr == true)
			{
				functionAttributeFlag |= FunctionAttributeFlag.Constexpr;
			}
			if (cursor.IsFunctionInlined == true)
			{
				functionAttributeFlag |= FunctionAttributeFlag.Inline;
			}
			if (cursor.CXXConstructor_IsConvertingConstructor == false)
			{
				functionAttributeFlag |= FunctionAttributeFlag.Explicit;
			}
			switch (thisType.CXXRefQualifier)
			{
				case ClangSharp.Interop.CXRefQualifierKind.CXRefQualifier_LValue:
					functionAttributeFlag |= FunctionAttributeFlag.LValueReference;
					break;
				case ClangSharp.Interop.CXRefQualifierKind.CXRefQualifier_RValue:
					functionAttributeFlag |= FunctionAttributeFlag.RValueReference;
					break;
			}

			if (cursor.kind == ClangSharp.Interop.CXCursorKind.CXCursor_Constructor)
			{
				if (cursor.CXXConstructor_IsDefaultConstructor)
				{
					functionAttributeFlag |= FunctionAttributeFlag.DefaultConstructor;
				}
				else if (cursor.CXXConstructor_IsMoveConstructor)
				{
					functionAttributeFlag |= FunctionAttributeFlag.MoveConstructor;
				}
				else if (cursor.CXXConstructor_IsCopyConstructor)
				{
					functionAttributeFlag |= FunctionAttributeFlag.CopyConstructor;
				}
				else if (cursor.CXXConstructor_IsCopyConstructor)
				{
					functionAttributeFlag |= FunctionAttributeFlag.CopyConstructor;
				}
			}
			else if (cursor.kind == ClangSharp.Interop.CXCursorKind.CXCursor_Destructor)
			{
				functionAttributeFlag |= FunctionAttributeFlag.Destructor;
			}

			FunctionDecl functionDecl = new FunctionDecl()
			{
				Usr = usr,
				Name = cursor.Spelling.CString,
				FullName = cursor.GetFQN(),
				Namespace = cursor.GetNamespace(),
				ArgumentList = argumentList,
				TypeInfo = thisTypeInfo,
				AttributeList = CreateAttributeDeclList(cursor),
				DeclHash = hash,
				ParentDeclHash = parentDeclHash,
				FunctionAttributeFlags = functionAttributeFlag,
				NumDefaultArgument = numDefaultArgument,
				AccessLevel = cursor.CXXAccessSpecifier.GetAccessLevel(),
				Meta = CreateMetaData(cursor),
#if DEBUG
				DebugHashCode = cursor.MakeDebugHashCode(),
#endif
			};

			AddDecl(functionDecl);
		}

        private void ParseVariableDecl(in ClangSharp.Interop.CXCursor cursor)
        {
			string usr = cursor.GetNormalizedUsr();
			uint hash = cursor.Hash;
			if (ContainsDecl(hash) == true)
			{
				return;
			}

			if (cursor.IsDeleted == true || cursor.IsDefined == true || cursor.IsDefinition == false)
			{
				return;
			}

            if (cursor.ParentFunctionOrMethod.IsNull == false)
            {
                //  関数内定義の変数はスキップ
                return;
			}

			uint parentDeclHash = GetParentDeclCursor(cursor).Hash;

			ClangSharp.Interop.CXType type = cursor.Type;
			TypeInfo typeInfo = GetOrCreateTypeInfo(cursor.Type);

			VariableAttributeFlag variableAttributeFlag = default;
			if (cursor.IsConstexpr)
			{
				variableAttributeFlag |= VariableAttributeFlag.Constexpr;
			}
			if (cursor.IsStatic || cursor.kind == ClangSharp.Interop.CXCursorKind.CXCursor_VarDecl
				/*|| cursor.ParentFunctionOrMethod.IsNull == true*/
				)
			{
				variableAttributeFlag |= VariableAttributeFlag.Static;
			}
			if (cursor.CXXField_IsMutable)
			{
				variableAttributeFlag |= VariableAttributeFlag.Mutable;
			}
			switch(cursor.TlsKind)
			{
				case ClangSharp.Interop.CXTLSKind.CXTLS_Static:
					variableAttributeFlag |= VariableAttributeFlag.ThreadLocal_Static;
					break;
				case ClangSharp.Interop.CXTLSKind.CXTLS_Dynamic:
					variableAttributeFlag |= VariableAttributeFlag.ThreadLocal_Dynamic;
					break;
			}

			VariableDecl variableDecl = new ()
            {
				Usr = usr,
				Name = cursor.Spelling.CString,
                FullName = cursor.GetFQN(),
                Namespace = cursor.GetNamespace(),
                Type = typeInfo,
                AttributeList = CreateAttributeDeclList(cursor),
				DeclHash = hash,
                ParentDeclHash = parentDeclHash,
				VariableAttributeFlags = variableAttributeFlag,
				Meta = CreateMetaData(cursor),
				OffsetBits = cursor.OffsetOfField,
				BitFieldWidth = cursor.FieldDeclBitWidth,
				AccessLevel = cursor.CXXAccessSpecifier.GetAccessLevel(),
#if DEBUG
                DebugHashCode = cursor.MakeDebugHashCode(),
#endif
			};

			if (variableDecl.Name.Contains("instance_type"))
			{
				Util.BreakPoint();
			}

			AddDecl(variableDecl);
		}

		private void VisitCursor(in ClangSharp.Interop.CXCursor cursor)
        {
            switch (cursor.kind)
            {
                case ClangSharp.Interop.CXCursorKind.CXCursor_Namespace:
                    ParseNamespaceDecl(cursor);
					break;

                case ClangSharp.Interop.CXCursorKind.CXCursor_ClassDecl:
                case ClangSharp.Interop.CXCursorKind.CXCursor_StructDecl:
                case ClangSharp.Interop.CXCursorKind.CXCursor_UnionDecl:
                    ParseRecordDecl(cursor);
					break;

                case ClangSharp.Interop.CXCursorKind.CXCursor_TypeAliasDecl:
                    ParseAliasDecl(cursor);
                    break;

				case ClangSharp.Interop.CXCursorKind.CXCursor_TypeAliasTemplateDecl:
					ParseTemplateAliasDecl(cursor);
					break;

                case ClangSharp.Interop.CXCursorKind.CXCursor_ClassTemplate:
				//	部分特殊化もtemplate recordとして扱う
				/*
				// テンプレート
				template<class T>
				struct A{};

				//	部分特殊化
				template<class T>
				struct A<T*>{};

				// 全特殊化(これは通常recordとして扱う)
				template<>
				struct A<int*>{};
				 */
				case ClangSharp.Interop.CXCursorKind.CXCursor_ClassTemplatePartialSpecialization:
					ParseTemplateRecordDecl(cursor);
					break;

                case ClangSharp.Interop.CXCursorKind.CXCursor_CXXMethod:
                case ClangSharp.Interop.CXCursorKind.CXCursor_FunctionDecl:
                    ParseFunctionDecl(cursor);
					break;

                case ClangSharp.Interop.CXCursorKind.CXCursor_VarDecl:
				case ClangSharp.Interop.CXCursorKind.CXCursor_FieldDecl:
					ParseVariableDecl(cursor);
					break;

                case ClangSharp.Interop.CXCursorKind.CXCursor_CXXBaseSpecifier:
                    ParseBaseSpecifierDecl(cursor);
					break;

				case ClangSharp.Interop.CXCursorKind.CXCursor_EnumDecl:
					ParseEnumDecl(cursor);
					break;

				case ClangSharp.Interop.CXCursorKind.CXCursor_MacroDefinition:
				case ClangSharp.Interop.CXCursorKind.CXCursor_MacroExpansion:
				//case ClangSharp.Interop.CXCursorKind.CXCursor_MacroInstantiation:
					break;

				case ClangSharp.Interop.CXCursorKind.CXCursor_FriendDecl:
					ParseFriendDecl(cursor);
					break;

                case ClangSharp.Interop.CXCursorKind.CXCursor_NoDeclFound:
					ParseNoDeclFound(cursor);
                    break;
            }
		}

		//     private void RegisterTypeInfo(in ClangSharp.Interop.CXCursor cursor, TypeInfo typeInfo)
		//     {
		//ClangSharp.Interop.CXCursor parentCursor = cursor.SemanticParent;
		//         if(parentCursor.IsNull)
		//         {
		//             _NamespaceDeclDict[string.Empty].ClassList.Add((ClassTypeInfo)typeInfo);
		//             return;
		//         }

		//switch (parentCursor.kind)
		//         {
		//             case ClangSharp.Interop.CXCursorKind.CXCursor_Namespace:
		//		_NamespaceDeclDict[string.Empty].ClassList.Add((ClassTypeInfo)typeInfo);
		//		break;

		//             case ClangSharp.Interop.CXCursorKind.CXCursor_ClassDecl:

		//                 break;
		//         }
		//     }

		private T GetOrCreateTypeInfo<T>(in ClangSharp.Interop.CXType type) where T : TypeInfo
		{
			TypeInfo typeInfo = GetOrCreateTypeInfo(type);
#if DEBUG
			System.Diagnostics.Debug.Assert(typeInfo is T, $"TypeInfo type mismatch. Expected: {typeof(T).Name}, Actual: {typeInfo.GetType().Name}");
#endif
			return (T)typeInfo;
		}

		private TypeInfo GetOrCreateTypeInfo(in ClangSharp.Interop.CXType type)
        {
			if (type.Spelling.CString.Contains("const InstanceType"))
			{
				Util.BreakPoint();
			}

            int hash = type.GetHashCode();

			if (_TypeDict.TryGetValue(hash, out TypeInfo? typeInfo) == true)
            {
                return typeInfo;
			}

			TypeAttributeFlag typeAttributeFlags = GetTypeAttributeFlags(type);
			TypeKind typeKind = Local.ToTypeKind(type.kind);

			switch (type.TypeClass)
            {
				case ClangSharp.Interop.CX_TypeClass.CX_TypeClass_Builtin:
					typeInfo = new PrimitiveTypeInfo()
                    {
						TypeAttributeFlags = typeAttributeFlags,
						Name = type.Spelling.CString,
                        FullName = type.GetFQN(),
                        Namespace = string.Empty,
						TypeKind = typeKind,
					};
                    break;

                case ClangSharp.Interop.CX_TypeClass.CX_TypeClass_Record:
					if (type.Declaration.IsAnonymous)
					{
						typeAttributeFlags |= TypeAttributeFlag.Anonymous;
					}
                    typeInfo = new RecordTypeInfo()
                    {
						TypeAttributeFlags = typeAttributeFlags,
						Name = type.Declaration.Spelling.CString,
                        FullName = type.GetFQN(),
                        Namespace = type.GetNamespace(),
                        DeclHash = type.Declaration.Hash,
						TypeKind = typeKind,
					};
					break;

                case ClangSharp.Interop.CX_TypeClass.CX_TypeClass_FunctionNoProto:
                case ClangSharp.Interop.CX_TypeClass.CX_TypeClass_FunctionProto:
					{
						int numArgTypes = type.NumArgTypes;
						TypeInfo[] typeList = new TypeInfo[numArgTypes];
						for(int i = 0; i < numArgTypes; ++i)
						{
							typeList[i] = GetOrCreateTypeInfo(type.GetArgType((uint)i));
						}

						typeInfo = new FunctionTypeInfo()
						{
							TypeAttributeFlags = typeAttributeFlags,
							Name = type.Declaration.Spelling.CString,
							FullName = type.GetFQN(),
							Namespace = type.GetNamespace(),
							ReturnType = GetOrCreateTypeInfo(type.ResultType),
							ArgumentTypeList = typeList,
							TypeKind = typeKind,
						};
					}
					break;

                case ClangSharp.Interop.CX_TypeClass.CX_TypeClass_MemberPointer:
                    typeInfo = new MemberPointerType()
                    {
						TypeAttributeFlags = typeAttributeFlags,
						Name = type.Spelling.CString,
                        FullName = type.GetFQN(),
                        Namespace = type.GetNamespace(),
                        PointeeType = GetOrCreateTypeInfo(type.PointeeType),
                        DeclHash = type.Declaration.Hash,
						TypeKind = typeKind,
					};

					break;

                case ClangSharp.Interop.CX_TypeClass.CX_TypeClass_Pointer:
                case ClangSharp.Interop.CX_TypeClass.CX_TypeClass_LValueReference:
                case ClangSharp.Interop.CX_TypeClass.CX_TypeClass_RValueReference:
                    typeInfo = new PointerTypeInfo()
                    {
						TypeAttributeFlags = typeAttributeFlags,
						Name = type.Spelling.CString,
                        FullName = type.GetFQN(),
                        Namespace = type.GetNamespace(),
                        PointeeType = GetOrCreateTypeInfo(type.PointeeType),
						DeclHash = type.Declaration.Hash,
						TypeKind = typeKind,
					};
					break;
					

                case ClangSharp.Interop.CX_TypeClass.CX_TypeClass_Elaborated:
					typeInfo = new ElaboratedTypeInfo()
					{
						TypeAttributeFlags = typeAttributeFlags,
						Name = type.Spelling.CString,
						FullName = type.CanonicalType.Spelling.CString,
						Namespace = type.GetNamespace(),
						PointeeType = GetOrCreateTypeInfo(type.CanonicalType),
						DeclHash = type.Declaration.Hash,
						TypeKind = typeKind,
					};
					break;
                case ClangSharp.Interop.CX_TypeClass.CX_TypeClass_Typedef:
                case ClangSharp.Interop.CX_TypeClass.CX_TypeClass_Using:
                    typeInfo = new TypeAliasInfo()
                    {
						TypeAttributeFlags = typeAttributeFlags,
						Name = type.Spelling.CString,
                        FullName = type.GetFQN(),
                        Namespace = type.GetNamespace(),
                        PointeeType = GetOrCreateTypeInfo(type.CanonicalType),
						DeclHash = type.Declaration.Hash,
						TypeKind = typeKind,
					};
                    break;

                case ClangSharp.Interop.CX_TypeClass.CX_TypeClass_Enum:
					if (type.Declaration.Spelling.CString.Contains("InstanceType"))
					{
						Util.BreakPoint();
					}
                    typeInfo = new EnumTypeInfo()
                    {
						TypeAttributeFlags = typeAttributeFlags,
						Name = type.Declaration.Spelling.CString,
                        FullName = type.GetFQN(),
                        Namespace = type.GetNamespace(),
						DeclHash = type.Declaration.Hash,
						TypeKind = typeKind,
					};
					break;

                case ClangSharp.Interop.CX_TypeClass.CX_TypeClass_TemplateTypeParm:
                case ClangSharp.Interop.CX_TypeClass.CX_TypeClass_SubstTemplateTypeParm:
					typeInfo = new PrimitiveTypeInfo()
					{
						TypeAttributeFlags = typeAttributeFlags,
						Name = type.Spelling.CString,
						FullName = type.GetFQN(),
						Namespace = string.Empty,
						TypeKind = typeKind,
					};
					break;

                case ClangSharp.Interop.CX_TypeClass.CX_TypeClass_Auto:
					typeInfo = new PrimitiveTypeInfo()
					{
						TypeAttributeFlags = typeAttributeFlags,
						Name = type.Spelling.CString,
						FullName = type.GetFQN(),
						Namespace = string.Empty,
						TypeKind = typeKind,
					};
					break;

                case ClangSharp.Interop.CX_TypeClass.CX_TypeClass_InjectedClassName:
					typeInfo = new PrimitiveTypeInfo()
					{
						TypeAttributeFlags = typeAttributeFlags,
						Name = type.Spelling.CString,
						FullName = type.GetFQN(),
						Namespace = string.Empty,
						TypeKind = typeKind,
					};
					break;

                case ClangSharp.Interop.CX_TypeClass.CX_TypeClass_DependentSizedArray:
                case ClangSharp.Interop.CX_TypeClass.CX_TypeClass_ArrayParameter:
                case ClangSharp.Interop.CX_TypeClass.CX_TypeClass_ConstantArray:
                case ClangSharp.Interop.CX_TypeClass.CX_TypeClass_IncompleteArray:
                case ClangSharp.Interop.CX_TypeClass.CX_TypeClass_VariableArray:
                case ClangSharp.Interop.CX_TypeClass.CX_TypeClass_PackExpansion:
					typeInfo = new UnknownTypeInfo()
					{
						TypeAttributeFlags = typeAttributeFlags,
						Name = type.Spelling.CString,
						FullName = type.GetFQN(),
						Namespace = string.Empty,
						TypeKind = typeKind,
					};
					break;

                case ClangSharp.Interop.CX_TypeClass.CX_TypeClass_DependentName:
					typeInfo = new UnknownTypeInfo()
					{
						TypeAttributeFlags = typeAttributeFlags,
						Name = type.Spelling.CString,
						FullName = type.GetFQN(),
						Namespace = string.Empty,
						TypeKind = typeKind,
					};
					break;

                case ClangSharp.Interop.CX_TypeClass.CX_TypeClass_Decltype:
					typeInfo = new UnknownTypeInfo()
					{
						TypeAttributeFlags = typeAttributeFlags,
						Name = type.Spelling.CString,
						FullName = type.GetFQN(),
						Namespace = string.Empty,
						TypeKind = typeKind,
					};
					break;

                case ClangSharp.Interop.CX_TypeClass.CX_TypeClass_TemplateSpecialization:
                case ClangSharp.Interop.CX_TypeClass.CX_TypeClass_DeducedTemplateSpecialization:
                case ClangSharp.Interop.CX_TypeClass.CX_TypeClass_DependentTemplateSpecialization:
					typeInfo = new UnknownTypeInfo()
					{
						TypeAttributeFlags = typeAttributeFlags,
						Name = type.Spelling.CString,
						FullName = type.GetFQN(),
						Namespace = string.Empty,
						TypeKind = typeKind,
					};
					break;

                case ClangSharp.Interop.CX_TypeClass.CX_TypeClass_UnaryTransform:
					typeInfo = new UnknownTypeInfo()
					{
						TypeAttributeFlags = typeAttributeFlags,
						Name = type.Spelling.CString,
						FullName = type.GetFQN(),
						Namespace = string.Empty,
						TypeKind = typeKind,
					};
					break;

				default:
                    Util.Assert(false, "Unsupported type class encountered while creating TypeInfo.");
					break;
            }

            _TypeDict[hash] = typeInfo;
			return typeInfo;
		}
		#endregion

		#region 非公開メソッド
		private void AddDecl(DeclBase decl)
		{
			bool success = _DeclWithHashDict.TryAdd(decl.DeclHash, decl);
#if DEBUG
			if(success == false)
			{
				DeclBase d = GetDecl(decl.DeclHash);
                Util.Assert(false, $"追加済みのDeclです: {d.ToString()}");
            }
#endif

			{
				ref readonly var meta = ref decl.GetMeta();
				success = _DeclWithUsrDict.TryAdd(meta.UniqueDeclKey, decl);
#if DEBUG
				if (success == false)
				{
					DeclBase d = GetDecl(meta.UniqueDeclKey);
			//		Trace.Warning(this, $"追加済みのDeclです: {d.ToString()}");
				}
#endif
			}
		}

		private bool ContainsDecl(uint hash)
		{
			return _DeclWithHashDict.ContainsKey(hash);
		}

		private bool ContainsDecl(in UniqueDeclKey key)
		{
			if (_DeclWithUsrDict.TryGetValue(key, out var _) == false)
			{
				return false;
			}

			return true;
		}

		private T? FindDecl<T>(uint hash) where T : DeclBase
        {
            _DeclWithHashDict.TryGetValue(hash, out DeclBase? decl);
            return decl as T;
		}

		private DeclBase GetDecl(uint hash)
        {
            Util.Assert(_DeclWithHashDict.ContainsKey(hash));
            return _DeclWithHashDict[hash];
		}

		private T GetDecl<T>(uint hash) where T : DeclBase
		{
			_DeclWithHashDict.TryGetValue(hash, out DeclBase? decl);
			Util.Assert(decl != null, $"Declaration with hash {hash} not found.");
			T? v = decl as T;
			Util.Assert(v != null, $"Declaration type mismatch. Expected: {typeof(T).Name}, Actual: {decl.GetType().Name}");
			return v;
		}

		public DeclBase GetDecl(in UniqueDeclKey key)
		{
			Util.Assert(_DeclWithUsrDict.ContainsKey(key));
			return _DeclWithUsrDict[key];
		}

		public T GetDecl<T>(in UniqueDeclKey key) where T : DeclBase
		{
			_DeclWithUsrDict.TryGetValue(key, out DeclBase? decl);
			Util.Assert(decl != null, $"Declaration with usr {key} not found.");
			T? v = decl as T;
			Util.Assert(v != null, $"Declaration type mismatch. Expected: {typeof(T).Name}, Actual: {decl.GetType().Name}");
			return v;
		}

		private ClangSharp.Interop.CXCursor GetParentDeclCursor(ClangSharp.Interop.CXCursor cursor, bool skipAnonymouse=false)
        {
			cursor = cursor.SemanticParent;

			while (cursor.IsNull == false)
			{
				switch (cursor.kind)
				{
					case ClangSharp.Interop.CXCursorKind.CXCursor_ClassDecl:
					case ClangSharp.Interop.CXCursorKind.CXCursor_UnionDecl:
					case ClangSharp.Interop.CXCursorKind.CXCursor_StructDecl:
					case ClangSharp.Interop.CXCursorKind.CXCursor_Namespace:
					case ClangSharp.Interop.CXCursorKind.CXCursor_TranslationUnit:
					case ClangSharp.Interop.CXCursorKind.CXCursor_ClassTemplate:
					case ClangSharp.Interop.CXCursorKind.CXCursor_ClassTemplatePartialSpecialization:
						//	匿名ならスキップ
						if(skipAnonymouse && cursor.IsAnonymous)
						{
							cursor = cursor.SemanticParent;
							break;
						}
						return cursor;
					default:
						cursor = cursor.SemanticParent;
						break;
				}
			}

			Util.Assert(false, "Parent declaration not found.");
			throw new InvalidOperationException("Parent declaration not found.");
		}

		private DeclBase GetParentDecl(ClangSharp.Interop.CXCursor cursor)
        {
			return GetDecl(GetParentDeclCursor(cursor).Hash);
		}

        private AttributeDecl[] CreateAttributeDeclList(in ClangSharp.Interop.CXCursor cursor)
        {
            int numAttr = cursor.NumAttrs;

			Util.Assert(numAttr >= 0, "Number of attributes cannot be negative.");
			AttributeDecl[] attrDeclList = new AttributeDecl[numAttr];

            for(uint i = 0; i < numAttr; i++)
            {
				ClangSharp.Interop.CXCursor attrCursor = cursor.GetAttr(i);
				uint hash = attrCursor.Hash;

				string value = attrCursor.Spelling.CString;

				AttrKind attrKind;
				switch (attrCursor.Kind)
				{ 
					case ClangSharp.Interop.CXCursorKind.CXCursor_AnnotateAttr:
						//  エンジン外の、clang::annoate属性
						if (attrCursor.Spelling.CString == Define.RUNTIME_REFLECTION_GENERATOR_DEFINE)
						{
							attrKind = AttrKind.Annotate;
						}
                        else
                        {
							if (value.Contains("IgnoreReflection()") == true)
							{
								attrKind = AttrKind.IgnoreReflectionTarget;
							}
							else if (value.Contains("Reflection()") == true)
							{
								attrKind = AttrKind.ReflectionTarget;
							}
							else
							{
								attrKind = AttrKind.EngineAnnotate;
							}
						}
						break;

					default:
						attrKind = AttrKind.Standard;
						break;
				}

				attrDeclList[i] = new AttributeDecl()
				{
					DeclHash = hash,
					AttrKind = attrKind,
					AttrName = attrCursor.AttrKind.ToString(),
					Value = value,
				};
			}

			return attrDeclList;
		}

		private BaseSpecifierDecl[] CreateBaseSpecifierDeclList(in ClangSharp.Interop.CXCursor cursor, out bool isBaseReflectionClass, out bool inheritedFromNoxObject)
		{
			int numBase = cursor.NumBases;
			if (numBase <= 0)
			{
				isBaseReflectionClass = false;
				inheritedFromNoxObject = false;
				return [];
			}
            BaseSpecifierDecl[] baseTypeList = new BaseSpecifierDecl[numBase];
            isBaseReflectionClass = false;
            inheritedFromNoxObject = false;
            for (int i = 0, length = numBase; i < length; ++i)
            {
                ClangSharp.Interop.CXCursor baseCursor = cursor.GetBase((uint)i);
				
                VisitCursor(baseCursor);
                GetOrCreateTypeInfo(baseCursor.Type);

                BaseSpecifierDecl baseSpecifierDecl = baseTypeList[i] = GetDecl< BaseSpecifierDecl>(baseCursor.Hash);
                if (baseSpecifierDecl.PointeeDeclHash == 0)
                {
                    continue;
                }

                DeclBase tmpBaseDecl = GetDecl(baseSpecifierDecl.GetPointeeMeta().UniqueDeclKey);
                while (true)
                {
                    if (tmpBaseDecl is RecordDecl tmpClassDecl)
                    {
                        if (tmpClassDecl.IsReflectionClass == true)
                        {
                            isBaseReflectionClass = true;
                        }
                        if (tmpClassDecl.IsNoxObject == true)
                        {
                            inheritedFromNoxObject = true;
                        }
                        break;
                    }
					else if (tmpBaseDecl is TemplateTypeAliasDecl tmpTemplateTypeAliasDecl)
					{
						break;
					}
					else if (tmpBaseDecl is TypeAliasDecl tmpTypeAliasDecl)
					{
						tmpBaseDecl = GetDecl<TypeDecl>(tmpTypeAliasDecl.GetPointeeMeta().UniqueDeclKey);
					}
					else if (tmpBaseDecl is NoFoundDecl)
					{
						break;
					}
					else
					{
						Util.Assert(false, $"Unsupported base type declaration: {tmpBaseDecl.GetType().Name}");
					}
                }
            }
			
			
			return baseTypeList;
        }

        private MetaInfo CreateMetaData(in ClangSharp.Interop.CXCursor cursor)
		{
			//  ソリューションディレクトまで辿って、.vcxprojを探す
			//  なければ、unnamedとして扱う
			var location = cursor.Location;
			location.GetFileLocation(out ClangSharp.Interop.CXFile outFile, out uint outLine, out uint outColumn, out uint outOffset);
			int locationHash = location.GetHashCode();

			UniqueDeclKey uniqueDeclKey = new ()
			{ 
				LocationHash = locationHash,
				Usr = cursor.GetNormalizedUsr()
			};

			if (outFile.Handle == 0)
			{
				return new () { 
					ProjectName = Define.UNKNOWN_MODULE_NAME,
					SourceFilePath = outFile.Name.CString,
					SourceLine = outLine,
					SourceColumn = outColumn,
					UniqueDeclKey = uniqueDeclKey,
				};
			}
			string? path = System.IO.Path.GetDirectoryName(outFile.Name.CString);

			string moduleName = Define.UNKNOWN_MODULE_NAME;

			//  ルートディレクトリ内か？
			if (path == null || path.StartsWith(_ProjectRootDirectory) == false)
			{
				return new () {
					ProjectName = moduleName,
					SourceFilePath = outFile.Name.CString,
					SourceLine = outLine,
					SourceColumn = outColumn,
					UniqueDeclKey = uniqueDeclKey,
				};
			}

			while (path != null)
			{
				string[] vcxProjectFileList = Directory.GetFiles(path, "*.vcxproj");
				if (vcxProjectFileList.Length > 0)
				{
					if (vcxProjectFileList.Length > 1)
					{
						Trace.WarningLine(this, $".vcxprojファイルが複数見つかりました　最初に見つかったファイルをmodule名として扱います\n{vcxProjectFileList}");
					}

					moduleName = Path.GetFileNameWithoutExtension(vcxProjectFileList[0]);
					break;
				}

				if (_ProjectRootDirectory == path)
				{
					break;
				}

				path = Path.GetDirectoryName(path);
			}

			MetaInfo module = new () {
				ProjectName = moduleName,
				SourceFilePath = outFile.Name.CString,
				SourceLine = outLine,
				SourceColumn = outColumn,
				UniqueDeclKey = uniqueDeclKey,
			};

			return module;
		}

		private static RecordAttributeFlag GetRecordAttributeFlags(in ClangSharp.Interop.CXCursor cursor)
		{
			RecordAttributeFlag recordAttributeFlags = default;

			switch (cursor.kind)
			{
				case ClangSharp.Interop.CXCursorKind.CXCursor_ClassDecl:
				case ClangSharp.Interop.CXCursorKind.CXCursor_ClassTemplate:
					recordAttributeFlags |= RecordAttributeFlag.Class;
					break;
				case ClangSharp.Interop.CXCursorKind.CXCursor_StructDecl:
					recordAttributeFlags |= RecordAttributeFlag.Struct;
					break;
				case ClangSharp.Interop.CXCursorKind.CXCursor_UnionDecl:
					recordAttributeFlags |= RecordAttributeFlag.Union;
					break;
			}

			if (cursor.IsAnonymousRecordDecl || cursor.IsAnonymousStructOrUnion)
			{
				recordAttributeFlags |= RecordAttributeFlag.Anonymous;
			}

			return recordAttributeFlags;
		}

		private static TypeAttributeFlag GetTypeAttributeFlags(in ClangSharp.Interop.CXType type)
		{
			TypeAttributeFlag typeAttributeFlags = default;
			if (type.IsConstQualified)
			{
				typeAttributeFlags |= TypeAttributeFlag.Const;
			}
			if (type.IsVolatileQualified)
			{
				typeAttributeFlags |= TypeAttributeFlag.Volatile;
			}
			switch (type.CXXRefQualifier)
			{
				case ClangSharp.Interop.CXRefQualifierKind.CXRefQualifier_LValue:
					typeAttributeFlags |= TypeAttributeFlag.LValueReference;
					break;
				case ClangSharp.Interop.CXRefQualifierKind.CXRefQualifier_RValue:
					typeAttributeFlags |= TypeAttributeFlag.RValueReference;
					break;
			}

			return typeAttributeFlags;
		}
		#endregion
	}
}
