///	@file	type_name.h
///	@brief	type_name
#pragma once

#include	<type_traits>
#include	<string_view>

#include	"../basic_definition.h"
#include	"../basic_type.h"
#include	"../crc32.h"
#include	"type_traits.h"
#include	"../preprocessor/cat.h"

namespace nox::util
{
#if defined(__clang__)
	/**
	 * @brief 型名を取得
	*/
	template <class T>// requires(!std::is_const_v<T> && !std::is_volatile_v<T>)
	[[nodiscard]] constexpr auto GetTypeName(void)noexcept
	{
		constexpr std::string_view signature = __PRETTY_FUNCTION__;
		constexpr auto signatureSize = signature.size();

		constexpr std::string_view preSignature = "auto nox::util::GetTypeName()[T = ";
		constexpr auto preSignatureSize = preSignature.size();

		constexpr std::string_view lastSignature = "]";
		constexpr auto lastSignatureSize = lastSignature.size();

		constexpr std::string_view contentsName(signature.data() + preSignature.size(),
			signatureSize - (preSignatureSize + lastSignatureSize));

		return contentsName;
	}
#else
	/**
	 * @brief 型名を取得
	*/
	template <class T>// requires(!std::is_const_v<T> && !std::is_volatile_v<T>)
	[[nodiscard]] constexpr auto GetTypeName(void)noexcept
	{

		constexpr std::string_view signature = __FUNCSIG__;
		constexpr auto signatureSize = signature.size();

		constexpr std::string_view preSignature = "auto __cdecl nox::util::GetTypeName<";
		constexpr auto preSignatureSize = preSignature.size();

		constexpr std::string_view lastSignature = ">(void) noexcept";
		constexpr auto lastSignatureSize = lastSignature.size();

		constexpr std::string_view contentsName(signature.data() + preSignature.size(),
			signatureSize - (preSignatureSize + lastSignatureSize));

		return contentsName;
	}
#endif

	

	/// @brief 型IDを取得する
	/// @tparam T 型
	/// @return ID(4byte)
	template<class T>
	inline constexpr uint32 GetUniqueTypeID()noexcept
	{
#if defined(__clang__)
		return util::Crc32(std::string_view(__PRETTY_FUNCTION__));
#else
		//	crc32で余分な計算をしないように__FUNCSIG__ではなく型名を渡す
		return util::Crc32(GetTypeName<T>());
#endif // defined(__clang__)
	}

//	namespace detail
//	{
//	}
//
//	/// @brief 関数ポインタからIDを取得する
//	/// @tparam obj 関数ポインタ
//	/// @return ID
//	template<auto obj> requires(IsEveryFunctionV<decltype(obj)> || IsLambdaValue<decltype(obj)>)
//	inline constexpr u32 GetFunctionID()noexcept
//	{
//#if defined(__clang__)
//		return util::crc32(__PRETTY_FUNCTION__);
//#else
//		return util::crc32(NOX_DETAIL_TO_U8STRING(__FUNCSIG__));
//#endif // defined(__clang__)
//	}
}