///	@file	mutex.h
///	@brief	mutex
#pragma once
#include	<type_traits>

namespace nox
{
	namespace os::detail
	{
		class MutexBase
		{

		};
	}

	namespace concepts::detail
	{
		/**
		 * @brief Mutex用
		*/
		template<class T>
		concept MutexConcept = requires(T& x) {
			std::is_base_of_v<os::detail::MutexBase, T>;
			x.Lock();
			x.Unlock();
		};
	}

	namespace os
	{
		/**
			 * @brief 単一スコープロック
			 * @details ミューテックスのLock()/Unlock()処理を
						コンストラクタとデストラクタで確実に実行する
			*/
		template<class _MutexType> requires(
			std::is_base_of_v<os::detail::MutexBase, _MutexType> &&
			std::is_invocable_v<decltype(&_MutexType::Lock), _MutexType> &&
			std::is_invocable_v<decltype(&_MutexType::Unlock), _MutexType> 
			)
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
			inline explicit ScopedLock(_MutexType& mutex)noexcept :
				mutex_(mutex)
			{
				mutex_.Lock();
			}

			/**
			 * @brief デストラクタ
			 * @details	ロック終了
			*/
			inline ~ScopedLock()
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
}