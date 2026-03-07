///	@file	intrusive_ptr.h
///	@brief	intrusive_ptr
#pragma once
#include	"type_traits/type_name.h"
#include	"utility.h"

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

		void IntrusivePtrAbort();

		template<class T>
		inline void IntrusivePtrAddReferenceWrapper(T* ptr)
		{
			if constexpr (IntrusivePtrAddReferenceConcept<T>)
			{
				if constexpr (std::is_const_v<T>)
				{
					IntrusivePtrAddReference(*const_cast<std::remove_const_t<T>*>(ptr));
				}
				else
				{
					IntrusivePtrAddReference(*ptr);
				}
			}
			else
			{
				//	ここには来ないはず
				nox::detail::IntrusivePtrAbort();
			}
		}

		template<class T>
		inline void IntrusivePtrReleaseReferenceWrapper(T* ptr)
		{
			if constexpr (IntrusivePtrReleaseReferenceConcept<T>)
			{
				if constexpr (std::is_const_v<T>)
				{
					IntrusivePtrReleaseReference(*const_cast<std::remove_const_t<T>*>(ptr));
				}
				else
				{
					IntrusivePtrReleaseReference(*ptr);
				}
			}
			else
			{
				//	ここには来ないはず
				nox::detail::IntrusivePtrAbort();
			}
		}

	}

	/// @brief		侵入型スマートポインタ
	/// @details	リソースカウンタアクセサ、解放メソッドは各自用意
	template<class T>
	class IntrusivePtr final
	{
		template<class U>
		friend class IntrusivePtr;
	public:
		inline constexpr IntrusivePtr()noexcept :
			instance_(nullptr) {}

		inline IntrusivePtr(T*const& instance)noexcept :
			instance_(instance)
		{
		}

		inline constexpr IntrusivePtr(const IntrusivePtr& ths)noexcept :
			instance_(ths.instance_)
		{
		}

		inline constexpr IntrusivePtr(IntrusivePtr&& ths)noexcept :
			instance_(ths.instance_)
		{
			ths.instance_ = nullptr;
		}


		template<std::derived_from<T> U>
		inline constexpr IntrusivePtr(U*const& instance)noexcept :
			instance_(static_cast<U*>(instance))
		{
		}

		template<std::derived_from<T> U>
		inline constexpr IntrusivePtr(const IntrusivePtr<U>& rhs):
			instance_(static_cast<T*>(rhs.instance_))
		{
			nox::detail::IntrusivePtrAddReferenceWrapper(instance_);
		}

		template<std::derived_from<T> U>
		inline constexpr IntrusivePtr(IntrusivePtr<U>&& rhs)noexcept:
			instance_(static_cast<T*>(rhs.instance_))
		{
			rhs.instance_ = nullptr;
		}

		template<class U> requires(std::is_base_of_v<T, U>)
			inline constexpr IntrusivePtr(const IntrusivePtr<U>& rhs, nox::detail::IntrusivePtrDownCastTag tag):
			instance_(static_cast<T*>(rhs.instance_))
		{
			nox::detail::IntrusivePtrAddReferenceWrapper(instance_);
		}

		///// @brief 親から子の型へのキャストmove
		///// @tparam U 
		///// @param rhs 
		///// @param tag 
		//template<class U> requires(std::is_base_of_v<U, T>)
		//	inline constexpr IntrusivePtr(IntrusivePtr<U>&& rhs, nox::detail::IntrusivePtrDownCastTag _)noexcept:
		//	IntrusivePtr(std::forward<reinterpret_cast<IntrusivePtr<T>>(rhs))
		//{
		//}

		inline ~IntrusivePtr()
		{
			if (instance_ == nullptr)
			{
				return;
			}

			nox::detail::IntrusivePtrReleaseReferenceWrapper(instance_);
		}

		inline void Reset()
		{
			if (instance_ == nullptr)
			{
				return;
			}
			nox::detail::IntrusivePtrReleaseReferenceWrapper(instance_);

			instance_ = nullptr;
		}

		inline void Reset(T* const instance)
		{
			this->Reset();
			instance_ = instance;
			if (instance_ != nullptr)
			{
				nox::detail::IntrusivePtrAddReferenceWrapper(instance_);
			}
		}

		inline constexpr bool operator==(std::nullptr_t)const noexcept
		{
			return instance_ == nullptr;
		}

		inline constexpr IntrusivePtr& operator=(const IntrusivePtr& rhs)
		{
			instance_ = rhs.instance_;

			// 新しいインスタンスの参照カウントを増やす
			if (instance_ != nullptr)
			{
				nox::detail::IntrusivePtrAddReferenceWrapper(instance_);
			}

			return *this;
		}

		inline constexpr IntrusivePtr& operator=(IntrusivePtr&& rhs)noexcept
		{
			instance_ = rhs.instance_;
			rhs.instance_ = nullptr;

			return *this;
		}

		template<std::derived_from<T> U>
		inline constexpr IntrusivePtr& operator=(const IntrusivePtr<U>& rhs)
		{
			instance_ = static_cast<T*>(rhs.instance_);
			// 新しいインスタンスの参照カウントを増やす
			if (instance_ != nullptr)
			{
				nox::detail::IntrusivePtrAddReferenceWrapper(instance_);
			}
			return *this;
		}

		template<std::derived_from<T> U>
		inline constexpr IntrusivePtr& operator=(IntrusivePtr<U>&& rhs)noexcept
		{
			instance_ = static_cast<T*>(rhs.instance_);
			rhs.instance_ = nullptr;
			return *this;
		}

	/*	template<std::derived_from<T> U>
		inline constexpr IntrusivePtr& operator=(U* const& instance)
		{
			this->Reset(static_cast<T*>(instance));
			return *this;
		}*/

		[[nodiscard]] inline constexpr T* Get()const noexcept { return instance_; }
		[[nodiscard]] inline constexpr T** GetAddressOf()noexcept { return std::addressof(instance_); }
		[[nodiscard]] inline constexpr T*const* GetAddressOf()const noexcept { return std::addressof(instance_); }

		[[nodiscard]] inline constexpr T& operator*()const noexcept { return *instance_; }

		[[nodiscard]]	inline constexpr T* operator->()const noexcept { return instance_; }

		[[nodiscard]] inline constexpr operator T* () const noexcept { return instance_; }

		[[nodiscard]] inline constexpr operator bool()const noexcept { return instance_ != nullptr; }
	private:
		inline constexpr void AddRef()
		{

		}

		inline constexpr void Release()
		{
		
		}

	private:
		T* instance_;
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