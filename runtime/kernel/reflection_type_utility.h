//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	type_utility.h
///	@brief	type_utility
#pragma once
#include	"reflection_type_definition.h"
#include	"type_traits/type_traits.h"
#include	"type_traits/type_name.h"
#include	"type_traits/object_pointer_signature.h"
#include	"algorithm.h"
#include	"string_format.h"
namespace nox::reflection
{
	///**
	//	 * @brief タイプ識別から名前を取得
	//	 * @param typeKind
	//	 * @return 名前
	//	*/
	//[[nodiscard]] inline	constexpr ReflectionStringView GetTypeKindName(const TypeKind typeKind)noexcept
	//{
	//	switch (typeKind)
	//	{
	//	case TypeKind::Void:				return u8"Void";
	//	case TypeKind::Bool:				return u8"Bool";
	//	case TypeKind::Int8:				return u8"Int8";
	//	case TypeKind::Uint8:				return u8"Uint8";
	//	case TypeKind::Char8:				return u8"Char8";
	//	case TypeKind::Int16:				return u8"Int16";
	//	case TypeKind::Uint16:				return u8"UInt16";
	//	case TypeKind::Char:				return u8"Char";
	//	case TypeKind::SChar:				return u8"SChar";
	//	case TypeKind::UChar:				return u8"UChar";
	//	case TypeKind::Char16:				return u8"Char16";
	//	case TypeKind::Wchar16:				return u8"Wchar16";
	//	case TypeKind::Int32:				return u8"Int32";
	//	case TypeKind::Uint32:				return u8"UInt32";
	//	case TypeKind::Char32:				return u8"Char32";
	//	case TypeKind::Int64:				return u8"Int64";
	//	case TypeKind::UInt64:				return u8"UInt64";
	//	case TypeKind::F32:					return u8"Float";
	//	case TypeKind::F64:					return u8"Double";
	//	case TypeKind::Enum:				return u8"Enum";
	//	case TypeKind::ScopedEnum:			return u8"ScopedEnum";
	//	case TypeKind::Class:				return u8"Class";
	//	case TypeKind::Union:				return u8"Union";
	//	default:break;
	//	}
	//	return u8"Invalid";
	//}

	///**
	// * @brief 名前からタイプ識別を取得
	// * @param name
	// * @return タイプ識別
	//*/
	//[[nodiscard]] inline	constexpr TypeKind	GetTypeKind(const ReflectionStringView name)noexcept
	//{
	//	for (std::underlying_type_t<TypeKind> i = 0; i < nox::util::ToUnderlying(TypeKind::_Max); ++i)
	//	{
	//		if (name == GetTypeKindName(static_cast<TypeKind>(i)))
	//		{
	//			return static_cast<TypeKind>(i);
	//		}
	//	}

	//	return TypeKind::Invalid;
	//}

	/**
	 * @brief 型からタイプ識別を取得
	 * @tparam T
	 * @return タイプ識別
	*/
	template<class T>
	[[nodiscard]] constexpr TypeKind	GetTypeKind()noexcept
	{
		if constexpr (std::is_same_v<T, void> == true)
		{
			return TypeKind::Void;
		}
		else if constexpr (std::is_null_pointer_v<T> == true)
		{
			return TypeKind::NullPtr;
		}
		else if constexpr (std::is_same_v<T, bool> == true)
		{
			return TypeKind::Bool;
		}
		else if constexpr (std::is_same_v<T, std::int8_t> == true)
		{
			return TypeKind::Int8;
		}
		else if constexpr (std::is_same_v<T, std::uint8_t> == true)
		{
			return TypeKind::Uint8;
		}
		else if constexpr (std::is_same_v<T, char8_t> == true)
		{
			return TypeKind::Char8;
		}
		else if constexpr (std::is_same_v<T, std::int16_t> == true)
		{
			return TypeKind::Int16;
		}
		else if constexpr (std::is_same_v<T, std::uint16_t> == true)
		{
			return TypeKind::Uint16;
		}
		else if constexpr (std::is_same_v<T, char> == true)
		{
			return TypeKind::Char;
		}
		else if constexpr (std::is_same_v<T, signed char> == true)
		{
			return TypeKind::SChar;
		}
		else if constexpr (std::is_same_v<T, unsigned char> == true)
		{
			return TypeKind::UChar;
		}
		else if constexpr (std::is_same_v<T, char16_t> == true)
		{
			return TypeKind::Char16;
		}
		else if constexpr (std::is_same_v<T, wchar_t> == true)
		{
			return TypeKind::Wchar16;
		}
		else if constexpr (std::is_same_v<T, std::int32_t> == true)
		{
			return TypeKind::Int32;
		}
		else if constexpr (std::is_same_v<T, std::uint32_t> == true)
		{
			return TypeKind::Uint32;
		}
		else if constexpr (std::is_same_v<T, std::float_t> == true)
		{
			return TypeKind::Float;
		}
		else if constexpr (std::is_same_v<T, std::double_t> == true)
		{
			return TypeKind::Double;
		}
		else if constexpr (std::is_same_v<T, char32_t> == true)
		{
			return TypeKind::Char32;
		}
		else if constexpr (std::is_enum_v<T> == true)
		{
			return TypeKind::Enum;
		}
		//	else if constexpr (std::is_scoped_enum_v<T> == true)
		else if constexpr (IsScopedEnumValue<T> == true)
		{
			return TypeKind::ScopedEnum;
		}
		else if constexpr (std::is_union_v<T> == true)
		{
			return TypeKind::Union;
		}
		else if constexpr (std::is_class_v<T> == true)
		{
			return TypeKind::Class;
		}
		else if constexpr (std::is_function_v<T> == true)
		{
			return TypeKind::Delegate;
		}
		else if constexpr (std::is_member_function_pointer_v<T> == true)
		{
			return TypeKind::MemberFunction;
		}
		else if constexpr (std::is_array_v<T> == true)
		{
			return TypeKind::Array;
		}
		else if constexpr (std::is_unbounded_array_v<T> == true)
		{
			return TypeKind::UnboundedArray;
		}
		else if constexpr (std::is_pointer_v<T> == true)
		{
			return TypeKind::Pointer;
		}
		else if constexpr (std::is_lvalue_reference_v<T> == true)
		{
			return TypeKind::LvalueReference;
		}
		else if constexpr (std::is_rvalue_reference_v<T> == true)
		{
			return TypeKind::RvalueReference;
		}
		else
		{
			NOX_ASSERT(false, nox::util::Format(U"不明な型:{0}", nox::util::GetTypeName<T>()));
			return TypeKind::Invalid;
		}
	}

	template<class T> requires(std::is_const_v<T> || std::is_volatile_v<T>)
		[[nodiscard]] constexpr TypeKind	GetTypeKind()noexcept
	{
		return nox::reflection::GetTypeKind<std::remove_cv_t<T>>();
	}

	/**
	 * @brief
	 * @tparam T
	 * @return
	*/
	template<class T>
	[[nodiscard]] inline constexpr TypeQualifierFlag GetTypeAttributeFlags()noexcept
	{
		TypeQualifierFlag typeAttributeFlags = TypeQualifierFlag::None;
		if constexpr (std::is_const_v<T> == true)
		{
			typeAttributeFlags = util::BitOr(typeAttributeFlags, TypeQualifierFlag::Const);
		}
		if constexpr (std::is_volatile_v<T> == true)
		{
			typeAttributeFlags = util::BitOr(typeAttributeFlags, TypeQualifierFlag::Volatile);
		}
		if constexpr (std::is_final_v<T> == true)
		{
			typeAttributeFlags = util::BitOr(typeAttributeFlags, TypeQualifierFlag::Final);
		}
		if constexpr (std::is_abstract_v<T> == true)
		{
			typeAttributeFlags = util::BitOr(typeAttributeFlags, TypeQualifierFlag::Abstract);
		}
		if constexpr (std::is_unsigned_v<T> == true)
		{
			typeAttributeFlags = util::BitOr(typeAttributeFlags, TypeQualifierFlag::Unsigned);
		}
		if constexpr (std::is_polymorphic_v<T> == true)
		{
			typeAttributeFlags = util::BitOr(typeAttributeFlags, TypeQualifierFlag::Polymorphic);
		}

		return typeAttributeFlags;
	}


	template<class T> //requires(std::is_member_function_pointer_v<T> || std::is_function_v<T>)
	[[nodiscard]] inline constexpr FunctionAttributeFlag GetFunctionAttributeFlags()noexcept
	{
		FunctionAttributeFlag retFlags = FunctionAttributeFlag::None;

		if constexpr (nox::IsFunctionConstValue<T> == true)
		{
			retFlags = nox::util::BitOr(retFlags, FunctionAttributeFlag::Const);
		}

		if constexpr (std::is_member_function_pointer_v<T> == false)
		{
			retFlags = nox::util::BitOr(retFlags, FunctionAttributeFlag::Static);
		}

		if constexpr (nox::IsFunctionVolatileValue<T> == true)
		{
			retFlags = nox::util::BitOr(retFlags, FunctionAttributeFlag::Volatile);
		}

		if constexpr (nox::IsFunctionLValueReference<T> == true)
		{
			retFlags = nox::util::BitOr(retFlags, FunctionAttributeFlag::LvalueRef);
		}

		if constexpr (nox::IsFunctionRValueReference<T> == true)
		{
			retFlags = nox::util::BitOr(retFlags, FunctionAttributeFlag::RvalueRef);
		}

		if constexpr (nox::IsFunctionNoexceptValue<T> == true)
		{
			retFlags = nox::util::BitOr(retFlags, FunctionAttributeFlag::Noexcept);
		}

		return retFlags;
	}

	/**
	 * @brief フィールド型情報から属性を取得する
	 * @tparam T フィールド型
	 * @return 属性
	*/
	template<class T>
	[[nodiscard]] inline	constexpr VariableAttributeFlag GetFieldAttributeFlags()noexcept {
		VariableAttributeFlag retFlags = VariableAttributeFlag::None;

		//	メンバーか
		if constexpr (std::is_member_object_pointer_v<T> == true)
		{
			retFlags = nox::util::BitOr(retFlags, VariableAttributeFlag::Static);
		}

		return retFlags;
	}

}