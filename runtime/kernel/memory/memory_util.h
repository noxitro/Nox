///	@file	memory_util.h
///	@brief	memory_util
#pragma once
#include	<type_traits>
#include	<memory>

#include	"../basic_definition.h"
#include	"../basic_type.h"
#include	"../type_traits/concepts.h"

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

	nox::int32	Copy(not_null<void*> dest, size_t dest_size, not_null<const void*> src, size_t source_size);

	nox::int32	WideCopy(not_null<nox::wchar16*> dest, size_t dest_size, not_null<const nox::wchar16*> source, size_t source_size);

	template<class Dest, class Source>
	inline not_null<void*>	Copy(Dest& dest, const Source& src, size_t size = sizeof(Source))
	{
		return Copy(&dest, &src, size);
	}
}

namespace nox::util
{
	/// @brief		左辺地参照の値のアドレスを取得
	/// @details	ポインタの場合はそのまま返す
	/// @return		左辺地参照の値のアドレスまたは、ポインタ
	template<class T>
	inline constexpr decltype(auto) AddressOfLvalueReference(T& value)noexcept
	{
		if constexpr (std::is_pointer_v<std::decay_t<T>> == true)
		{
			return value;
		}
		else
		{
			return std::addressof(value);
		}
	}

	/// @brief 一時オブジェクトのアドレス取得ができないようにする
	/// @return none
	template<class T>
	inline constexpr const T* AddressOfLvalueReference(const T&&)noexcept = delete;


	/// @brief std::reference_wrapperが指すインスタンスのアドレスを取得する
	/// @tparam T 
	/// @param v 
	/// @return 
	template<class T> requires(!std::is_pointer_v<T> && !nox::concepts::detail::HasOperatorArrow<T>)
	inline constexpr decltype(auto) TryToAddress(std::reference_wrapper<T> v)noexcept
	{
		return std::addressof(v.get());
	}

	/// @brief 値のアドレスを取得する
	/// @tparam T 
	/// @param v 
	/// @return 
	template<class T> requires(!std::is_pointer_v<T> && !nox::concepts::detail::HasOperatorArrow<T>)
		inline constexpr T* TryToAddress(T& v)noexcept
	{
		return std::addressof<T>(v);
	}

	/// @brief ポインタやoperator->()を持つ型のアドレスを取得する
	/// @tparam T 
	/// @param v 
	/// @return 
	template<class T > requires(std::is_pointer_v<T> || nox::concepts::detail::HasOperatorArrow<T>)
		inline constexpr decltype(auto) TryToAddress(T&& v)noexcept
	{
		return std::to_address(std::forward<T>(v));
	}
}