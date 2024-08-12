///	@file	convert_string.h
///	@brief	convert_string
#pragma once
#include	"basic_definition.h"
#include	"basic_type.h"

#include	"type_traits/type_traits.h"
#include	"advanced_type.h"

namespace nox::util
{
	/// @brief 安全な文字列キャスト
	/// @tparam To 
	/// @param  
	/// @return 
	template<class To>
	inline const To* CharCast(nox::not_null<const char16*>)noexcept = delete;

	template<class To>
	inline const To* CharCast(nox::not_null<const wchar16*>)noexcept = delete;

	template<class To>
	inline const To* CharCast(nox::not_null<const char*>)noexcept = delete;

	template<class To>
	inline const To* CharCast(nox::not_null<const char8*>)noexcept = delete;

	template<class To>
	inline To* CharCast(char16*)noexcept = delete;

	template<class To>
	inline To* CharCast(wchar16*)noexcept = delete;

	template<class To>
	inline To* CharCast(char*)noexcept = delete;

	template<class To>
	inline To* CharCast(char8*)noexcept = delete;

	template<class To>
	inline To CharCast(char16)noexcept = delete;

	template<class To>
	inline To CharCast(wchar16)noexcept = delete;

	template<class To>
	inline To CharCast(char)noexcept = delete;

	template<class To>
	inline To CharCast(char8)noexcept = delete;

	template<>
	inline const wchar16* CharCast<wchar16>(nox::not_null<const char16*> str)noexcept
	{
		return reinterpret_cast<const wchar16*>(str.get());
	}

	template<>
	inline wchar16* CharCast<wchar16>(char16* str)noexcept
	{
		return reinterpret_cast<wchar16*>(str);
	}

	template<>
	inline wchar16 CharCast<wchar16>(char16 str)noexcept
	{
		return *reinterpret_cast<const wchar16*>(&str);
	}

	template<>
	inline const char16* CharCast<char16>(nox::not_null<const wchar16*> str)noexcept
	{
		return reinterpret_cast<const char16*>(str.get());
	}

	template<>
	inline char16* CharCast<char16>(wchar16* str)noexcept
	{
		return reinterpret_cast<char16*>(str);
	}

	template<>
	inline char16 CharCast<char16>(wchar16 str)noexcept
	{
		return *reinterpret_cast<const char16*>(&str);
	}

	template<>
	inline const char8* CharCast<char8>(nox::not_null<const char*> str)noexcept
	{
		return reinterpret_cast<const char8*>(str.get());
	}

	template<>
	inline char8* CharCast<char8>(char* str)noexcept
	{
		return reinterpret_cast<char8*>(str);
	}

	template<>
	inline char8 CharCast<char8>(char str)noexcept
	{
		return *reinterpret_cast<const char8*>(str);
	}

	template<>
	inline const char* CharCast<char>(nox::not_null<const char8*> str)noexcept
	{
		return reinterpret_cast<const char*>(str.get());
	}

	template<>
	inline char* CharCast<char>(char8* str)noexcept
	{
		return reinterpret_cast<char*>(str);
	}

	template<>
	inline char CharCast<char>(char8 str)noexcept
	{
		return *reinterpret_cast<const char*>(&str);
	}

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