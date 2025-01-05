///	@file	intrusive_ptr.h
///	@brief	intrusive_ptr
#pragma once
#include	"assertion.h"
#include	"type_traits/type_name.h"
namespace nox
{
//	template<class T>
//	inline void IntrusivePtrAddReference(T&) ;

//	/// @brief 関数を登録した時に呼ばれる
//	inline constexpr void IntrusivePtrAddReference(auto)noexcept;

	/// @brief 関数を登録解除した時に呼ばれる
//	template<class T>
//	inline void IntrusivePtrReleaseReference(T&) ;

	namespace detail
	{
		template<class T>
		concept IntrusivePtrAddReferenceConcept = requires(T & t)
		{
			IntrusivePtrAddReference(t);
		};

		template<class T>
		concept IntrusivePtrReleaseReferenceConcept = requires(T & t)
		{
			IntrusivePtrReleaseReference(t);
		};

		struct IntrusivePtrDownCastTag {};

		class IntrusivePtrBase
		{
		protected:
			inline constexpr IntrusivePtrBase()noexcept:
				instance_(nullptr) {}

			inline constexpr explicit IntrusivePtrBase(void*const ptr)noexcept :
				instance_(ptr) {}

			inline	constexpr	void	Move(IntrusivePtrBase& rhs)noexcept
			{
				instance_ = rhs.instance_;
				rhs.instance_ = nullptr;
			}

		protected:
			void* instance_;
		};
	}

	/// @brief		侵入型スマートポインタ
	/// @details	リソースカウンタアクセサ、解放メソッドは各自用意
	template<class T>
	class IntrusivePtr : public detail::IntrusivePtrBase
	{
	public:
		inline constexpr IntrusivePtr()noexcept :
			detail::IntrusivePtrBase(nullptr) {}

		inline explicit IntrusivePtr(T*const& instance)noexcept :
			detail::IntrusivePtrBase(static_cast<void*>(instance))
		{
		}

		inline constexpr IntrusivePtr(IntrusivePtr&& ths)noexcept :
			detail::IntrusivePtrBase(ths.instance_)
		{
			ths.instance_ = nullptr;
		}

		template<std::derived_from<T> U>
		inline constexpr IntrusivePtr(const IntrusivePtr<U>& rhs)noexcept
		{
			instance_ = static_cast<void*>(rhs.instance_);

			if constexpr (nox::detail::IntrusivePtrAddReferenceConcept<T> == true)
			{
				IntrusivePtrAddReference(*static_cast<T*>(instance_));
			}
		}

		template<std::derived_from<T> U>
		inline constexpr IntrusivePtr(IntrusivePtr<U>&& rhs)noexcept
		{
			Move(rhs);
		}

		template<class U> requires(std::is_base_of_v<T, U>)
		inline constexpr IntrusivePtr(const IntrusivePtr<U>& rhs, nox::detail::IntrusivePtrDownCastTag tag)noexcept
		{
			instance_ = rhs.instance_;

			if constexpr (nox::detail::IntrusivePtrAddReferenceConcept<T> == true)
			{
				IntrusivePtrAddReference(*static_cast<T*>(instance_));
			}
		}

		/// @brief 親から子の型へのキャストmove
		/// @tparam U 
		/// @param rhs 
		/// @param tag 
		template<class U> requires(std::is_base_of_v<U, T>)
			inline constexpr IntrusivePtr(IntrusivePtr<U>&& rhs, nox::detail::IntrusivePtrDownCastTag tag)noexcept
		{
			Move(rhs);
		}

		inline ~IntrusivePtr()
		{
			if (instance_ == nullptr)
			{
				return;
			}

			static_assert(nox::detail::IntrusivePtrReleaseReferenceConcept<T> == true);
			if constexpr (nox::detail::IntrusivePtrReleaseReferenceConcept<T> == true)
			{
				IntrusivePtrReleaseReference(*static_cast<T*>(instance_));
			}
			else
			{
				NOX_ASSERT(false, U"ここには来ないはず");
			}
		}

		inline constexpr IntrusivePtr& operator=(const IntrusivePtr& rhs)noexcept
		{
			instance_ = rhs.instance_;
			if constexpr (nox::detail::IntrusivePtrReleaseReferenceConcept<T> == true)
			{
				IntrusivePtrReleaseReference(*static_cast<T*>(instance_));
			}

			return *this;
		}

		inline constexpr IntrusivePtr& operator=(IntrusivePtr&& rhs)noexcept
		{
			instance_ = rhs.instance_;
			rhs.instance_ = nullptr;

			return *this;
		}

		[[nodiscard]] inline constexpr T* Get()noexcept { return static_cast<T*>(instance_); }
		[[nodiscard]] inline constexpr const T* Get()const noexcept { return static_cast<const T*>(instance_); }

		[[nodiscard]] inline constexpr T& operator*()noexcept { return *static_cast<T*>(instance_); }
		[[nodiscard]] inline constexpr const T& operator*()const noexcept { return *static_cast<const T*>(instance_); }

		[[nodiscard]]	inline constexpr T* operator->()noexcept { return static_cast<T*>(instance_); }
		[[nodiscard]]	inline constexpr const T* operator->()const noexcept { return static_cast<const T*>(instance_); }

		[[nodiscard]] inline constexpr operator T* () const noexcept { return static_cast<T*>(instance_); }
	private:
		//T* instance_;
	};

	template<class T, class U> requires(std::is_base_of_v<U, T>)
	inline	nox::IntrusivePtr<T> IntrusivePtrDynamicCast(const nox::IntrusivePtr<U>& other)noexcept
	{
		return nox::IntrusivePtr<T>(other, nox::detail::IntrusivePtrDownCastTag{});
	}

	template<class T, class U> requires(std::is_base_of_v<U, T>)
	inline	nox::IntrusivePtr<T> IntrusivePtrDynamicCast(nox::IntrusivePtr<U>&& other)noexcept
	{
		return nox::IntrusivePtr<T>(std::forward<nox::IntrusivePtr<U>>(other), nox::detail::IntrusivePtrDownCastTag{});
	}
}