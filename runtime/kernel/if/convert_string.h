///	@file	convert_string.h
///	@brief	convert_string
#pragma once
#include	"basic_type.h"
#include	"basic_definition.h"

#include	"../type_traits/type_traits.h"
namespace nox::util
{
	//	安全なキャスト
	template<class To> requires(std::is_same_v<std::remove_cvref_t<std::remove_pointer_t<To>>, w16>)
	constexpr inline To CharCast(not_null<const c16*> strPtr)noexcept { return reinterpret_cast<To>(strPtr.get()); }

	template<class To> requires(std::is_same_v<std::remove_cvref_t<std::remove_pointer_t<To>>, c16>)
	constexpr inline To CharCast(not_null<const w16*> strPtr)noexcept { return reinterpret_cast<To>(strPtr.get()); }


#pragma region 変換
	nox::WString	ConvertWString(const c16* const from);

	template<std::same_as<nox::WString> To, class From> requires(nox::IsCharTypeValue<From>)
	inline	To	ConvertString(const From* const str)
	{
		return util::ConvertWString(str);
	}

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
	template<class To, class From>
	inline From& ConvertStringSafe(From&& object) { return object; }

	template<class To, class From> requires((nox::IsStringClassAllValue<std::remove_cvref_t<From>> || nox::IsCharTypeValue< std::remove_cvref_t<From>>) && !util::detail::IsSameStringClassValue<To, std::remove_cvref_t<From>>)
	inline To ConvertStringSafe(From&& object) { return nox::util::ConvertString<To>(object); }

	template<class To, class From>
	inline From& CastStringSafe(From&& object) { return object; }

	template<class To, class From> requires(nox::IsCharTypeValue<std::remove_cvref_t<std::remove_pointer_t<From>>>)
	inline To CastStringSafe(const From* object) { return CharCast<To>(object); }

	template<class To, class From> requires(nox::IsStringClassAllValue<std::remove_cvref_t<From>>)
	inline To CastStringSafe(From&& object) { return CharCast<To>(object.c_str()); }

#pragma endregion

}