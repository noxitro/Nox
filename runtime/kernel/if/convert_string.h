///	@file	convert_string.h
///	@brief	convert_string
#pragma once
#include	"basic_type.h"
#include	"basic_definition.h"

namespace nox::util
{
	//	安全なキャスト
	template<class To> requires(std::is_same_v<std::remove_cvref_t<std::remove_pointer_t<To>>, w16>)
	constexpr inline To CharCast(not_null<const c16*> strPtr)noexcept { return reinterpret_cast<To>(strPtr.get()); }

	template<class To> requires(std::is_same_v<std::remove_cvref_t<std::remove_pointer_t<To>>, c16>)
	constexpr inline To CharCast(not_null<const w16*> strPtr)noexcept { return reinterpret_cast<To>(strPtr.get()); }


#pragma region 変換

#pragma endregion

}