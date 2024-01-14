///	@file	convert_string.h
///	@brief	convert_string
#pragma once
#include	"basic_definition.h"
#include	"basic_type.h"

#include	"../type_traits/type_traits.h"
#include	"advanced_type.h"

namespace nox::util
{
	//	安全なキャスト
	template<class To> requires(std::is_same_v<std::remove_cvref_t<std::remove_pointer_t<To>>, w16>)
	constexpr inline To CharCast(not_null<const c16*> strPtr)noexcept { return reinterpret_cast<To>(strPtr.get()); }

	template<class To> requires(std::is_same_v<std::remove_cvref_t<std::remove_pointer_t<To>>, c16>)
	constexpr inline To CharCast(not_null<const w16*> strPtr)noexcept { return reinterpret_cast<To>(strPtr.get()); }

	template<class To> requires(std::is_same_v<std::remove_cvref_t<std::remove_pointer_t<To>>, char>)
	constexpr inline To CharCast(not_null<const c8*> strPtr)noexcept { return reinterpret_cast<To>(strPtr.get()); }

	template<class To> requires(std::is_same_v<std::remove_cvref_t<std::remove_pointer_t<To>>, c8>)
	constexpr inline To CharCast(not_null<const char*> strPtr)noexcept { return reinterpret_cast<To>(strPtr.get()); }


	template<class T> requires(nox::IsCharTypeValue<T>)
	constexpr inline size_t GetStrLength(const T* strPtr)noexcept
	{
		return std::char_traits<T>::length(strPtr);
	}

#pragma region 変換



	namespace detail
	{
		template<class T, class U>
		struct IsSameStringClass : std::false_type {};

		template<class T, class U> requires(std::is_same_v<T, U>)
			struct IsSameStringClass<T, U> : std::true_type {};

		template<class T, class U> requires(std::is_same_v<typename T::value_type, U>)
			struct IsSameStringClass<T, U> : std::true_type {};

		template<class T, class U> requires(std::is_same_v<T, typename U::value_type>)
			struct IsSameStringClass<T, U> : std::true_type {};

		template<class T, class U> requires(std::is_same_v<typename T::value_type, typename U::value_type>)
			struct IsSameStringClass<T, U> : std::true_type {};

		template<class T, class U>
		constexpr bool IsSameStringClassValue = IsSameStringClass<T, U>::value;
	}

	//	formatで異なる文字列を変換するための関数

	///// @brief from other
	///// @tparam To 
	///// @tparam From 
	///// @param object 
	///// @return 
	//template<class To, class From>
	////	requires(
	////nox::IsStringClassValue<To> &&
	////!nox::IsCharTypeValue<std::remove_cvref_t<std::remove_pointer_t<From>>> &&
	////!nox::IsStringClassValue<std::remove_cvref_t<From>> &&
	////	!nox::IsStringViewClassValue<std::remove_cvref_t<From>>
	////	)
	//inline constexpr decltype(auto) ConvertStringSafe(From&& object)noexcept { return object; }

	///// @brief from pointer
	///// @tparam To 
	///// @tparam From 
	///// @param object 
	///// @return 
	//template<class To, class From> requires(nox::IsCharTypeValue<From>)
	//inline To ConvertStringSafe(const From* object) { 
	////	return util::ConvertString<To>(object); 
	//	return To();
	//}

	///// @brief from string class
	///// @tparam To 
	///// @tparam From 
	///// @param object 
	///// @return 
	//template<class To, class From> requires(nox::IsStringClassValue<To>&& nox::IsCharTypeValue<From>)
	//inline To ConvertStringSafe(const nox::BasicString<From>& object) { 
	//	return nox::util::ConvertString<To>(object); 
	//}

	///// @brief	from string_view
	///// @tparam To 
	///// @tparam From 
	///// @param object 
	///// @return 
	//template<class To, class From> requires(nox::IsStringClassValue<To>&& nox::IsCharTypeValue<From>)
	//inline To ConvertStringSafe(const std::basic_string_view<From> object) { return nox::util::ConvertString<To>(object); }

#pragma endregion

}