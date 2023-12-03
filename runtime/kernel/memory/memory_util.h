///	@file	memory_util.h
///	@brief	memory_util
#pragma once
#include	<type_traits>
#include	"../if/basic_definition.h"

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

	not_null<void*>	Copy(not_null<void*> dest, not_null<const void*> src, size_t size);

	template<class Dest, class Source>
	inline not_null<void*>	Copy(Dest& dest, const Source& src, size_t size = sizeof(Source))
	{
		return Copy(&dest, &src, size);
	}
}