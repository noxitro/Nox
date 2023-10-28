///	@file	convert_string.h
///	@brief	convert_string
#pragma once
#include	"basic_type.h"
#include	"basic_definition.h"

namespace nox::util
{
	//	安全なキャスト
	template<class To> requires(std::is_same_v<To, w16>)
	constexpr inline const To* CharCast(not_null<const c16*> strPtr)noexcept { return reinterpret_cast<const To*>(strPtr.get()); }

	template<class To> requires(std::is_same_v<To, c16>)
	constexpr inline const To* CharCast(not_null<const w16*> strPtr)noexcept { return reinterpret_cast<const To*>(strPtr.get()); }


#pragma region 変換

#pragma endregion

}