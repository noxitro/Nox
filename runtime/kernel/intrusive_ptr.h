///	@file	intrusive_ptr.h
///	@brief	intrusive_ptr
#pragma once
#include	"assertion.h"
#include	"type_traits/type_name.h"
namespace nox
{
	template<class T>
	inline void IntrusivePtrAddReference(T&) ;

//	/// @brief 関数を登録した時に呼ばれる
//	inline constexpr void IntrusivePtrAddReference(auto)noexcept;

	/// @brief 関数を登録解除した時に呼ばれる
	template<class T>
	inline constexpr void IntrusivePtrReleaseReference(T&) ;

	/// @brief		侵入型スマートポインタ
	/// @details	リソースカウンタアクセサ、解放メソッドは各自用意
	template<class T>
	class IntrusivePtr
	{
	public:
		inline constexpr IntrusivePtr()noexcept :
			instance_(nullptr) {}

		inline explicit IntrusivePtr(T*& instance)noexcept :
			instance_(instance)
		{
			IntrusivePtrAddReference(*instance_);
		}

		template<std::derived_from<T> U>
		inline constexpr IntrusivePtr(const IntrusivePtr<U>& rhs)noexcept
		{
			instance_ = rhs.Get();
			IntrusivePtrAddReference(*instance_);
		}

		inline ~IntrusivePtr()
		{
			if (instance_ == nullptr)
			{
				return;
			}

			IntrusivePtrReleaseReference(*instance_);
			{
			//	static_assert(nox::detail::IsCallable_IntrusivePtr<T>::value == true);
			//	NOX_ASSERT(false, u"ここには来ないはず");
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