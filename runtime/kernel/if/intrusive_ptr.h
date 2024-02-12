///	@file	intrusive_ptr.h
///	@brief	intrusive_ptr
#pragma once
#include	"assertion.h"

namespace nox
{
	namespace detail
	{
		template<class T>
		struct is_callable {
		private:
			template<typename U>
			static inline constexpr auto test(int) -> std::conditional_t<std::is_void_v<std::void_t<decltype(IntrusivePtrAddReference(*(U*)nullptr))>>, std::true_type, std::false_type >;
			template<typename U>
			static inline constexpr std::false_type test(...);

		public:
			static constexpr bool value = decltype(test<T>(0))::value;
		};
	}

	/// @brief 関数を登録した時に呼ばれる
	template<class T>
	inline constexpr void IntrusivePtrAddReference(T&)noexcept {}

	/// @brief 関数を登録解除した時に呼ばれる
	template<class T>
	inline constexpr void IntrusivePtrReleaseReference(T&)noexcept = delete;

	namespace detail
	{
		/// @brief IntrusivePtrAddReference, IntrusivePtrReleaseReferenceを呼び出し可能かどうか
		template<class T>
		struct IsCallable_IntrusivePtr
		{
			template<typename U>
			static inline constexpr auto Test(int) -> std::conditional_t<std::is_void_v<std::void_t<decltype(IntrusivePtrAddReference(*(U*)nullptr))>>, std::true_type, std::false_type >;
			template<typename U>
			static inline constexpr std::false_type Test(...);

			template<typename U>
			static inline constexpr auto Test2(int) -> std::conditional_t<std::is_void_v<std::void_t<decltype(IntrusivePtrReleaseReference(*(U*)nullptr))>>, std::true_type, std::false_type >;
			template<typename U>
			static inline constexpr std::false_type Test2(...);

		public:
			static constexpr bool value = 
				decltype(Test<T>(0))::value &&
				decltype(Test2<T>(0))::value
				;
		};

		
	}

	/// @brief		侵入型スマートポインタ
	/// @details	リソースカウンタアクセサ、解放メソッドは各自用意
	template<class T>
	class IntrusivePtr
	{
	public:
		inline constexpr IntrusivePtr()noexcept :
			instance_(nullptr) {}

		inline explicit IntrusivePtr(T& instance)noexcept :
			instance_(&instance)
		{
			IntrusivePtrAddReference(*instance_);
		}

		inline ~IntrusivePtr()
		{
			if (instance_ == nullptr)
			{
				return;
			}

			if constexpr (nox::detail::IsCallable_IntrusivePtr<T>::value == true)
			{
				IntrusivePtrReleaseReference(*instance_);
			}
			else
			{
			//	static_assert(nox::detail::IsCallable_IntrusivePtr<T>::value == true);
				NOX_ASSERT(false, u"ここには来ないはず");
			}
		}

		inline constexpr T* Get()noexcept { return instance_; }
		inline constexpr const T* Get()const noexcept { return instance_; }

		inline constexpr T& operator*()noexcept { return *instance_; }
		inline constexpr const T& operator*()const noexcept { return *instance_; }

		inline constexpr T* operator->()noexcept { return instance_; }
		inline constexpr const T* operator->()const noexcept { return instance_; }

	private:
		T* instance_;
	};
}