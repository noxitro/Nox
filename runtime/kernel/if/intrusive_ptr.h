///	@file	intrusive_ptr.h
///	@brief	intrusive_ptr
#pragma once
#include	"assertion.h"

namespace nox
{
	namespace concepts
	{
		template<class T>
		concept IntrusivePtrConcept = requires(T& x)
		{
			IntrusivePtrAddReference(x);
			IntrusivePtrReleaseReference(x);
		};
	}

	/// @brief		侵入型スマートポインタ
	/// @details	リソースカウンタアクセサ、解放メソッドは各自用意
	template<class T>
	class IntrusivePtr
	{
	public:
		inline constexpr IntrusivePtr()noexcept :
			ptr_(nullptr) {}

		inline ~IntrusivePtr()
		{
			if (ptr_ == nullptr)
			{
				return;
			}

			if constexpr (concepts::IntrusivePtrConcept<T> == true)
			{
				IntrusivePtrReleaseReference(*ptr_);
			}
			else
			{
				static_assert(concepts::IntrusivePtrConcept<T> == true);
				NOX_ASSERT(false, u"ここには来ないはず");
			}
		}

	private:
		T* ptr_;
	};
}