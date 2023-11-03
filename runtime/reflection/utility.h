///	@file	utility.h
///	@brief	utility
#pragma once
#include	"definition.h"

namespace nox::reflection
{
	/**
		 * @brief タイプ識別から名前を取得
		 * @param typeKind
		 * @return 名前
		*/
	[[nodiscard]] inline	constexpr std::u8string_view GetTypeKindName(const TypeKind typeKind)noexcept
	{
		switch (typeKind)
		{
		case TypeKind::Void:				return u8"Void";
		case TypeKind::Bool:				return u8"Bool";
		case TypeKind::Int8:				return u8"Int8";
		case TypeKind::Uint8:				return u8"Uint8";
		case TypeKind::Char8:				return u8"Char8";
		case TypeKind::Int16:				return u8"Int16";
		case TypeKind::Uint16:				return u8"UInt16";
		case TypeKind::Char:				return u8"Char";
		case TypeKind::SChar:				return u8"SChar";
		case TypeKind::UChar:				return u8"UChar";
		case TypeKind::Char16:				return u8"Char16";
		case TypeKind::Wchar16:				return u8"Wchar16";
		case TypeKind::Int32:				return u8"Int32";
		case TypeKind::Uint32:				return u8"UInt32";
		case TypeKind::Char32:				return u8"Char32";
		case TypeKind::Int64:				return u8"Int64";
		case TypeKind::UInt64:				return u8"UInt64";
		case TypeKind::F32:					return u8"Float";
		case TypeKind::F64:					return u8"Double";
		case TypeKind::Enum:				return u8"Enum";
		case TypeKind::ScopedEnum:			return u8"ScopedEnum";
		case TypeKind::Class:				return u8"Class";
		case TypeKind::Union:				return u8"Union";
		default:break;
		}
		return u8"Invalid";
	}

	/**
	 * @brief 名前からタイプ識別を取得
	 * @param name
	 * @return タイプ識別
	*/
	[[nodiscard]] inline	constexpr TypeKind	GetTypeKind(const std::u8string_view name)noexcept
	{
		for (std::underlying_type_t<TypeKind> i = 0; i < nox::util::ToUnderlying(TypeKind::_Max); ++i)
		{
			if (name == GetTypeKindName(static_cast<TypeKind>(i)))
			{
				return static_cast<TypeKind>(i);
			}
		}

		return TypeKind::Invalid;
	}

	/**
	 * @brief 型からタイプ識別を取得
	 * @tparam T
	 * @return タイプ識別
	*/
	template<class T>
		//requires(std::is_pointer_v<T> == false && std::is_reference_v<T> == false)
	[[nodiscard]] constexpr TypeKind	GetTypeKind()noexcept
	{
		if constexpr (std::is_same_v<T, void> == true)
		{
			return TypeKind::Void;
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
			return TypeKind::F32;
		}
		else if constexpr (std::is_same_v<T, std::double_t> == true)
		{
			return TypeKind::F64;
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
			return TypeKind::Function;
		}
		else if constexpr (std::is_member_function_pointer_v<T> == true)
		{
			return TypeKind::MemberFunction;
		}
		else if constexpr (nox::IsLambdaValue<T> == true)
		{
			return TypeKind::Lambda;
		}
		else if constexpr (nox::IsCaptureLambdaV<T> == true)
		{
			return TypeKind::CaptureLambda;
		}
		else
		{
			//NOX_ASSERT(false, u"不明な型");
			return TypeKind::Invalid;
		}
	}

	/**
	 * @brief
	 * @tparam T
	 * @return
	*/
	template<class T>
	[[nodiscard]] inline constexpr TypeAttributeFlag GetTypeAttributeFlags()noexcept
	{
		TypeAttributeFlag typeAttributeFlags = TypeAttributeFlag::None;
		if constexpr (std::is_pointer_v<T> == true)
		{
			typeAttributeFlags = util::BitOr(typeAttributeFlags, TypeAttributeFlag::Pointer);
		}
		if constexpr (std::is_lvalue_reference_v<T> == true)
		{
			typeAttributeFlags = util::BitOr(typeAttributeFlags, TypeAttributeFlag::LvalueReference);
		}
		else if constexpr (std::is_rvalue_reference_v<T> == true)
		{
			typeAttributeFlags = util::BitOr(typeAttributeFlags, TypeAttributeFlag::RvalueReference);
		}
		if constexpr (std::is_const_v<T> == true)
		{
			typeAttributeFlags = util::BitOr(typeAttributeFlags, TypeAttributeFlag::Const);
		}
		if constexpr (std::is_volatile_v<T> == true)
		{
			typeAttributeFlags = util::BitOr(typeAttributeFlags, TypeAttributeFlag::Volatile);
		}
		if constexpr (std::is_final_v<T> == true)
		{
			typeAttributeFlags = util::BitOr(typeAttributeFlags, TypeAttributeFlag::Final);
		}
		if constexpr (std::is_abstract_v<T> == true)
		{
			typeAttributeFlags = util::BitOr(typeAttributeFlags, TypeAttributeFlag::Abstract);
		}
		if constexpr (std::is_array_v<T> == true)
		{
			typeAttributeFlags = util::BitOr(typeAttributeFlags, TypeAttributeFlag::Array);
		}
		if constexpr (std::is_unbounded_array_v<T> == true)
		{
			typeAttributeFlags = util::BitOr(typeAttributeFlags, TypeAttributeFlag::UnboundedArray);
		}
		if constexpr (std::is_unsigned_v<T> == true)
		{
			typeAttributeFlags = util::BitOr(typeAttributeFlags, TypeAttributeFlag::Unsigned);
		}
		if constexpr (IsConstPointerValue<T> == true)
		{
			typeAttributeFlags = util::BitOr(typeAttributeFlags, TypeAttributeFlag::ConstPointer);
		}
		if constexpr (IsConstLvalueReferenceValue<T> == true)
		{
			typeAttributeFlags = util::BitOr(typeAttributeFlags, TypeAttributeFlag::ConstLvalueReference);
		}
		if constexpr (IsConstRvalueReferenceValue<T> == true)
		{
			typeAttributeFlags = util::BitOr(typeAttributeFlags, TypeAttributeFlag::ConstRvalueReference);
		}

		return typeAttributeFlags;
	}


	template<class T> //requires(std::is_member_function_pointer_v<T> || std::is_function_v<T>)
	[[nodiscard]] inline constexpr MethodAttributeFlag GetMethodAttributeFlags()noexcept
	{
		MethodAttributeFlag retFlags = MethodAttributeFlag::None;

		if constexpr (IsFunctionConstValue<T> == true)
		{
			retFlags = util::BitOr(retFlags, MethodAttributeFlag::Const);
		}

		if constexpr (IsFunctionMemberValue<T> == false)
		{
			retFlags = util::BitOr(retFlags, MethodAttributeFlag::Static);
		}

		if constexpr (IsFunctionVolatileValue<T> == true)
		{
			retFlags = util::BitOr(retFlags, MethodAttributeFlag::Volatile);
		}

		if constexpr (IsFunctionLvalueValue<T> == true)
		{
			retFlags = util::BitOr(retFlags, MethodAttributeFlag::LvalueRef);
		}

		if constexpr (IsFunctionRvalueValue<T> == true)
		{
			retFlags = util::BitOr(retFlags, MethodAttributeFlag::RvalueRef);
		}

		if constexpr (IsFunctionNoexceptValue<T> == true)
		{
			retFlags = util::BitOr(retFlags, MethodAttributeFlag::Noexcept);
		}

		return retFlags;
	}

	/**
	 * @brief フィールド型情報から属性を取得する
	 * @tparam T フィールド型
	 * @return 属性
	*/
	template<class T>
	[[nodiscard]] inline	constexpr FieldAttributeFlag GetFieldAttributeFlags()noexcept {
		FieldAttributeFlag retFlags = FieldAttributeFlag::None;

		//	メンバーか
		if constexpr (IsFieldMemberVariableValue<T> == true)
		{
			retFlags = util::BitOr(retFlags, FieldAttributeFlag::Member);
		}

		return retFlags;
	}

}