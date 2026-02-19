//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	type_utility.h
///	@brief	type_utility
#pragma once
#include	"reflection_type_definition.h"
#include	"type_traits/type_traits.h"
#include	"type_traits/type_name.h"
#include	"type_traits/object_pointer_signature.h"
#include	"algorithm.h"

namespace nox
{
	struct Interface;
}

namespace nox::reflection
{
	template<class T>
	using ReflectionOptional = std::optional<
		std::conditional_t<std::is_void_v<T>, std::monostate,
		std::conditional_t<std::is_reference_v<T>, std::reference_wrapper<std::remove_reference_t<T>>, T>
		>
	>;

	///**
	//	 * @brief タイプ識別から名前を取得
	//	 * @param typeKind
	//	 * @return 名前
	//	*/
	//[[nodiscard]] inline	constexpr std::u8string_view GetTypeKindName(const TypeKind typeKind)noexcept
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
	//[[nodiscard]] inline	constexpr TypeKind	GetTypeKind(const std::u8string_view name)noexcept
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

	namespace detail
	{
		template<class T>
		struct TypeKindHolder;

		template<nox::reflection::TypeKind _value>
		struct ITypeKindHolder
		{
			static constexpr nox::reflection::TypeKind value = _value;
		};

		template<class T>
		struct TypeKindHolder : ITypeKindHolder<nox::reflection::TypeKind::Unknown> {};

		template<>
		struct TypeKindHolder<void> : ITypeKindHolder<nox::reflection::TypeKind::Void> {};

		template<>
		struct TypeKindHolder<bool> : ITypeKindHolder<nox::reflection::TypeKind::Bool> {};

		template<>
		struct TypeKindHolder<char> : ITypeKindHolder<nox::reflection::TypeKind::Char> {};

		template<>
		struct TypeKindHolder<signed char> : ITypeKindHolder<nox::reflection::TypeKind::Int8> {};

		template<>
		struct TypeKindHolder<unsigned char> : ITypeKindHolder<nox::reflection::TypeKind::UInt8> {};

		template<>
		struct TypeKindHolder<char8_t> : ITypeKindHolder<nox::reflection::TypeKind::Char8> {};

		template<>
		struct TypeKindHolder<char16_t> : ITypeKindHolder<nox::reflection::TypeKind::Char16> {};

		template<>
		struct TypeKindHolder<char32_t> : ITypeKindHolder<nox::reflection::TypeKind::Char32> {};

		template<>
		struct TypeKindHolder<wchar_t> : ITypeKindHolder<nox::reflection::TypeKind::WideChar> {};

		template<>
		struct TypeKindHolder<std::int16_t> : ITypeKindHolder<nox::reflection::TypeKind::Int16> {};

		template<>
		struct TypeKindHolder<std::uint16_t> : ITypeKindHolder<nox::reflection::TypeKind::UInt16> {};

		template<>
		struct TypeKindHolder<std::int32_t> : ITypeKindHolder<nox::reflection::TypeKind::Int32> {};

		template<>
		struct TypeKindHolder<std::uint32_t> : ITypeKindHolder<nox::reflection::TypeKind::UInt32> {};

		template<>
		struct TypeKindHolder<std::int64_t> : ITypeKindHolder<nox::reflection::TypeKind::Int64> {};

		template<>
		struct TypeKindHolder<std::uint64_t> : ITypeKindHolder<nox::reflection::TypeKind::UInt64> {};

//#if defined(__SIZEOF_INT128__)
//		template<>
//		struct TypeKindHolder<__int128> : ITypeKindHolder < nox::reflection::TypeKind::Int128> {};
//
//		template<>
//		struct TypeKindHolder<unsigned __int128> : ITypeKindHolder<nox::reflection::TypeKind::UInt128> {};
//
//
//#endif // defined(__SIZEOF_INT128__)


		template<>
		struct TypeKindHolder<std::float_t> : ITypeKindHolder<nox::reflection::TypeKind::Float> {};

		template<>
		struct TypeKindHolder<std::double_t> : ITypeKindHolder<nox::reflection::TypeKind::Double> {};

		template<>
		struct TypeKindHolder<long double> : ITypeKindHolder<nox::reflection::TypeKind::LongDouble> {};

		template<>
		struct TypeKindHolder<long> : ITypeKindHolder<nox::reflection::TypeKind::Long> {};

		template<>
		struct TypeKindHolder<unsigned long> : ITypeKindHolder<nox::reflection::TypeKind::UnsignedLong> {};

		//	--- nullptr ---
		template<>
		struct TypeKindHolder<std::nullptr_t> : ITypeKindHolder<nox::reflection::TypeKind::Nullptr> {};

		//	--- user defined categories ---
		template<class T> requires(std::is_union_v<T>)
			struct TypeKindHolder<T> : ITypeKindHolder<nox::reflection::TypeKind::Union> {};

		template<class T> requires(std::is_class_v<T>)
			struct TypeKindHolder<T> : ITypeKindHolder<nox::reflection::TypeKind::Class> {};

		template<class T> requires(std::is_enum_v<T>&& nox::IsScopedEnumValue<T>)
			struct TypeKindHolder<T> : ITypeKindHolder<nox::reflection::TypeKind::ScopedEnum> {};

		template<class T> requires(std::is_enum_v<T> && !nox::IsScopedEnumValue<T>)
			struct TypeKindHolder<T> : ITypeKindHolder<nox::reflection::TypeKind::Enum> {};

		//	--- function ---
		template<class T> requires(std::is_function_v<T>)
			struct TypeKindHolder<T> : ITypeKindHolder<nox::reflection::TypeKind::Function> {};

		template<class T> requires(std::is_member_function_pointer_v<T>)
			struct TypeKindHolder<T> : ITypeKindHolder<nox::reflection::TypeKind::MemberFunctionPointer> {};

		template<class T> requires(std::is_pointer_v<T>&& std::is_function_v<std::remove_pointer_t<T>>)
			struct TypeKindHolder<T> : ITypeKindHolder<nox::reflection::TypeKind::FunctionPointer> {};

		template<class T> requires(std::is_member_object_pointer_v<T>)
			struct TypeKindHolder<T> : ITypeKindHolder<nox::reflection::TypeKind::MemberObjectPointer> {};

		//	--- pointer / reference ---
		template<class T>
		struct TypeKindHolder<T*> : ITypeKindHolder<nox::reflection::TypeKind::Pointer> {};

		template<class T>
		struct TypeKindHolder<T&> : ITypeKindHolder<nox::reflection::TypeKind::LValueReference> {};

		template<class T>
		struct TypeKindHolder<T&&> : ITypeKindHolder<nox::reflection::TypeKind::RValueReference> {};

		//	--- array ---
		template<class T, std::size_t N>
		struct TypeKindHolder<T[N]> : ITypeKindHolder<nox::reflection::TypeKind::Array> {};

		template<class T>
		struct TypeKindHolder<T[]> : ITypeKindHolder<nox::reflection::TypeKind::UnboundedArray> {};
	}

	/// @brief 型からタイプ識別を取得
	/// @tparam T 
	/// @return 
	template<class T>
	[[nodiscard]] constexpr nox::reflection::TypeKind	GetTypeKind()noexcept
	{
		using TT = std::remove_cv_t<T>;
		return nox::reflection::detail::TypeKindHolder<TT>::value;
	}

	template<class T>
	[[nodiscard]] inline constexpr TypeAttributeFlag GetTypeAttributeFlags()noexcept
	{
		TypeAttributeFlag type_attr_flags = TypeAttributeFlag::None;

		type_attr_flags = nox::util::BitOrConditional<std::is_const_v<T>, TypeAttributeFlag::Const>(type_attr_flags);
		type_attr_flags = nox::util::BitOrConditional<std::is_volatile_v<T>, TypeAttributeFlag::Volatile>(type_attr_flags);
		type_attr_flags = nox::util::BitOrConditional<std::is_final_v<T>, TypeAttributeFlag::Final>(type_attr_flags);
		type_attr_flags = nox::util::BitOrConditional<std::is_abstract_v<T>, TypeAttributeFlag::Abstract>(type_attr_flags);
		type_attr_flags = nox::util::BitOrConditional<std::is_unsigned_v<T>, TypeAttributeFlag::Unsigned>(type_attr_flags);
		type_attr_flags = nox::util::BitOrConditional<std::is_polymorphic_v<T>, TypeAttributeFlag::Polymorphic>(type_attr_flags);
		type_attr_flags = nox::util::BitOrConditional<std::is_base_of_v<nox::Interface, T>, TypeAttributeFlag::Interface>(type_attr_flags);
		type_attr_flags = nox::util::BitOrConditional<std::is_trivially_copyable_v<T>, TypeAttributeFlag::TrivialCopyable>(type_attr_flags);

		return type_attr_flags;
	}


	template<class T> //requires(std::is_member_function_pointer_v<T> || std::is_function_v<T>)
	[[nodiscard]] inline constexpr FunctionAttributeFlag GetFunctionAttributeFlags()noexcept
	{
		FunctionAttributeFlag attr_flags = FunctionAttributeFlag::None;
		attr_flags = nox::util::BitOrConditional<nox::IsFunctionConstValue<T>, FunctionAttributeFlag::Const>(attr_flags);
		attr_flags = nox::util::BitOrConditional<std::is_member_function_pointer_v<T>, FunctionAttributeFlag::Static>(attr_flags);
		attr_flags = nox::util::BitOrConditional<nox::IsFunctionVolatileValue<T>, FunctionAttributeFlag::Volatile>(attr_flags);
		attr_flags = nox::util::BitOrConditional<nox::IsFunctionLValueReference<T>, FunctionAttributeFlag::LvalueRef>(attr_flags);
		attr_flags = nox::util::BitOrConditional<nox::IsFunctionRValueReference<T>, FunctionAttributeFlag::RvalueRef>(attr_flags);
		attr_flags = nox::util::BitOrConditional<nox::IsFunctionNoexceptValue<T>, FunctionAttributeFlag::Noexcept>(attr_flags);
		attr_flags = nox::util::BitOrConditional<std::is_base_of_v<nox::Interface, T>, FunctionAttributeFlag::Noexcept>(attr_flags);

		return attr_flags;
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