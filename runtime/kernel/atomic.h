//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	atomic.h
///	@brief	atomic
#pragma once
#include	<atomic>
#include	"basic_definition.h"
#include	"type_traits/concepts.h"

namespace nox
{
	template<class T>
	using Atomic = std::atomic<T>;
}

namespace nox::atomic
{
	/// @brief 1加算し、加算後の値を返す。
	/// @details 戻り値は加算と同じ不可分操作で得た値。value を読み直すと他スレッドの更新が混ざる。
	///           なお、value は std::atomic_ref<T> の要求するアラインメントを満たし、
	///           同時アクセスはすべて原子的である必要がある。
	template<nox::concepts::Arithmetic T> requires(alignof(T) >= std::atomic_ref<T>::required_alignment)
		inline T Increment(T& value)noexcept
	{
		return ++std::atomic_ref<T>{value};
	}

	template<nox::concepts::Arithmetic T> requires(alignof(T) >= std::atomic_ref<T>::required_alignment)
		[[nodiscard]] inline T Increment(volatile T& value) noexcept
	{
		return nox::atomic::Increment(const_cast<T&>(value));
	}

	/// @brief 1減算し、減算後の値を返す。
	template<nox::concepts::Arithmetic T> requires(alignof(T) >= std::atomic_ref<T>::required_alignment)
		inline T Decrement(T& value)noexcept
	{
		static_assert(alignof(T) >= std::atomic_ref<T>::required_alignment);
		return --std::atomic_ref<T>{value};
	}

	template<nox::concepts::Arithmetic T> requires(alignof(T) >= std::atomic_ref<T>::required_alignment)
		[[nodiscard]] inline T Decrement(volatile T& value) noexcept
	{
		return nox::atomic::Decrement(const_cast<T&>(value));
	}

	/// @brief 不可分に読み出す。
	/// @details const メンバ関数からも呼べるよう const 参照で受ける。load は書き込みを伴わない。
	template<nox::concepts::Arithmetic T> requires(alignof(T) >= std::atomic_ref<T>::required_alignment)
		[[nodiscard]] inline T Read(const T& value)noexcept
	{
		return std::atomic_ref<T>{const_cast<T&>(value)}.load();
	}

	template<nox::concepts::Arithmetic T> requires(alignof(T) >= std::atomic_ref<T>::required_alignment)
		[[nodiscard]] inline T Read(const volatile T& value) noexcept
	{
		return nox::atomic::Read(const_cast<const T&>(value));
	}
}