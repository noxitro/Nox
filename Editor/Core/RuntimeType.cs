// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;
using System.Collections.Generic;
using System.Text;

namespace Core;

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

		DataMember,
		IgnoreDataMember,
		ResourcePath
	}

	public readonly struct AttributeDecl
	{
		public required AttributeKind Kind { get; init; }
		public required string AttrName { get; init; }
		public required string Value { get; init; }

		public AttributeDecl() { }
	}

	public enum RuntimeAccessLevel : byte
	{
		Private,
		Protected,
		Public
	}

	public enum RuntimeVariableAttributeFlag : ushort
	{
		None,
		Constexpr = 1 << 0,
		Static = 1 << 1,
		Constinit = 1 << 2,
		ThreadLocal_Static = 1 << 3,
		ThreadLocal_Dynamic = 1 << 4,
		Mutable = 1 << 5,
	}

	public enum RuntimeFunctionAttributeFlag : uint
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

	#region 型情報
	public class RuntimeTypeInfo
	{
		#region フィールド

		#endregion

		#region 公開プロパティ
		public required RuntimeTypeKind TypeKind { get; init; }
		public required long Size { get; init; }
		public required long Alignment { get; init; }
		public required string Name { get; init; }
		public required string FullName { get; init; }
		public required string Namespace { get; init; }

		public RuntimeTypeInfo PointeeTypeInfo { get; init; } = Invalid;
		public RuntimeTypeInfo UnderlyingTypeInfo { get; init; } = Invalid;
		public RuntimeTypeInfo ReturnTypeInfo { get; init; } = Invalid;
		public RuntimeTypeInfo[] ArgumentTypeList { get; init; } = [];
		public DeclBase Decl { get; set; } = RuntimeInvalidDecl.Invalid;
		#endregion

		public static readonly Core.RuntimeTypeInfo Invalid = new Core.RuntimeTypeInfo()
		{
			TypeKind = Core.RuntimeTypeKind.Invalid,
			Size = 0,
			Alignment = 0,
			Name = "Invalid",
			FullName = "Invalid",
			Namespace = ""
		};
	}

	internal sealed class InvalidTypeInfo : RuntimeTypeInfo
	{

	}
	#endregion

	#region 宣言情報
	public abstract class DeclBase
	{
	}

	public sealed class RuntimeInvalidDecl : DeclBase
	{
		public static readonly RuntimeInvalidDecl Invalid = new();
	}

	public abstract class NamedDecl : DeclBase
	{
		public required string Name { get; init; }
		public required string FullName { get; init; }
		public required System.Attribute[] AttributeList { get; init; }
	}

	public abstract class RuntimeTypeDecl : NamedDecl
	{
		public required string Namespace { get; init; }
	}

	public sealed class RuntimeRecordDecl : RuntimeTypeDecl
	{
		#region 公開プロパティ
		public required RuntimeRecordDecl[] RecordList { get; init; }
		public required RuntimeEnumDecl[] EnumList { get; init; }
		public required RuntimeVariableDecl[] VariableList { get; init; }
		public required RuntimeFunctionDecl[] FunctionList { get; init; }
		public RuntimePropertyDecl[] PropertyList { get; set; } = [];
		//	未実装
		public bool IsReflectionClass { get; set; }
		//	未実装
		public bool IsNoxObject { get; init; }
		public required RuntimeTypeInfo TypeInfo { get; init; }
		#endregion
	}

	public sealed class RuntimeFunctionDecl : RuntimeTypeDecl
	{
		public readonly struct ArgumentInfo
		{
			public required string Name { readonly get; init; } 
			public required bool IsDefault { readonly get; init; } 
			public required RuntimeTypeInfo TypeInfo { readonly get; init; } 
			public required System.Attribute[] AttributeList { readonly get; init; }
			public ArgumentInfo() { }
		}

		public required RuntimeTypeInfo TypeInfo { get; init; } 
		public required RuntimeFunctionAttributeFlag FunctionAttributeFlags { get; init; }
		public required ArgumentInfo[] ArgumentList { private get; init; }
		public ReadOnlySpan<ArgumentInfo> GetArgumentList() => ArgumentList;
		public required int NumDefaultArgument { get; init; } 
	}

	public sealed class RuntimePropertyDecl : NamedDecl
	{
		public required RuntimeTypeInfo TypeInfo { get; init; }
		public required RuntimeVariableDecl? VariableDecl { get; init; }
		public required RuntimeFunctionDecl? GetterFunctionDecl { get; init; }
		public required RuntimeFunctionDecl? SetterFunctionDecl { get; init; }
	}

	public sealed class RuntimeVariableDecl : RuntimeTypeDecl
	{
		public required RuntimeTypeInfo TypeInfo { get; init; }
		public required RuntimeVariableAttributeFlag VariableAttributeFlags { get; init; }
		public required long OffsetBits { get; set; }
		public required int BitFieldWidth { get; set; }
	}

	public sealed class RuntimeEnumDecl : RuntimeTypeDecl
	{
		public readonly struct EnumeratorInfo
		{
			public required string Name { readonly get; init; } = string.Empty;
			public required bool IsUnsigned { readonly get; init; } = false;
			public required long Int64 { readonly get; init; } = 0;
			public required System.Attribute[] AttributeList { readonly get; init; } = [];
			public readonly ReadOnlySpan<System.Attribute> GetAttributeList() => AttributeList;
			public readonly ulong Uint64 => unchecked((ulong)Int64);
			public EnumeratorInfo() { }
		}

		public required RuntimeTypeInfo TypeInfo { get; init; }
		public required EnumeratorInfo[] EnumeratorInfoList { get; init; } = [];
		public required bool FixedUnderlyingType { get; init; } = false;
		public ReadOnlySpan<EnumeratorInfo> GetEnumeratorList() => EnumeratorInfoList;
	}

	public sealed class RuntimeNamespaceDecl : NamedDecl
	{
		public required RuntimeNamespaceDecl[] NamespaceList { get; init; }
		public required RuntimeRecordDecl[] RecordList { get; init; }
		public required RuntimeEnumDecl[] EnumList { get; init; }
		public required RuntimeVariableDecl[] FieldList { get; init; }
		public required RuntimeFunctionDecl[] MethodList { get; init; }
	}
	#endregion
