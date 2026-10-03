//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	scoped_lock.h
///	@brief	scoped_lock
#pragma once
#include	<concepts>
#include	<type_traits>
#include	"mutex.h"

namespace nox
{
	namespace concepts::detail
	{
		template<class T>
		concept MutexConcept = requires(T & x) {
			std::is_same_v<nox::Mutex, T>;
			x.Lock();
			x.Unlock();
		};
	}

	/// @brief 単一スコープロック
	/// @details ミューテックスのLock()/Unlock()処理を
	///コンストラクタとデストラクタで確実に実行する
	/// @tparam _MutexType 
	template<nox::concepts::detail::MutexConcept _MutexType>
	class ScopedLock
	{
	public:
		using MutexType = _MutexType;

	public:
		/**
		 * @brief 引数付きコンストラクタ
		 * @details ロック開始
		 * @param mutex ミューテックスオブジェクト
		*/
		inline explicit ScopedLock(_MutexType& mutex)noexcept(noexcept(mutex.Lock())) :
			mutex_(mutex)
		{
			mutex_.Lock();
		}

		/**
		 * @brief デストラクタ
		 * @details	ロック終了
		*/
		inline ~ScopedLock() noexcept(noexcept(mutex_.Unlock()))
		{
			mutex_.Unlock();
		}

	private:
		inline constexpr ScopedLock()noexcept = delete;
		inline constexpr explicit ScopedLock(const ScopedLock&)noexcept = delete;
		inline constexpr explicit ScopedLock(const ScopedLock&&)noexcept = delete;

	private:
		/**
		 * @brief ミューテックスオブジェクト
		*/
		_MutexType& mutex_;
	};
}
