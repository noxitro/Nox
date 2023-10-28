///	@file	memory_util.h
///	@brief	memory_util
#pragma once
#include	<type_traits>

namespace nox::memory
{
	namespace detail
	{
		void	ZeroMemImpl(void* ptr, size_t size);
	}

	/*!********************************************************************
		* @brief	メモリの0クリア
		* @param [in] ptr アドレス
		* @param [in] size アドレスのサイズ
		* @details	型チェック、vtableを破壊しないためのstatic_assertを提供します
		**********************************************************************/
	template<class T> requires(std::is_pointer_v<T> && !std::is_const_v<T> && !std::is_polymorphic_v<T>)
		inline void	ZeroMem(T ptr, size_t size) 
	{
		detail::ZeroMemImpl(ptr, size);
	}
}