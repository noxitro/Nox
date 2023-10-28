///	@file	any.h
///	@brief	any
#pragma once

#include	"../type_traits/type_name.h"

namespace nox
{
	class AnyRef
	{
	public:
		[[nodiscard]] inline constexpr	AnyRef()noexcept:
			address_(nullptr),
			id_(0)
		{
		}

		template<class T>
		[[nodiscard]] inline	constexpr	explicit	AnyRef(T& address)noexcept
		{
			address_ = &address;
			id_ = util::GetUniqueTypeID<T>();
		}

		[[nodiscard]] inline	constexpr	explicit	AnyRef(std::nullptr_t&)noexcept
		{
			address_ = nullptr;
			id_ = 0;
		}

		template<class T>
		[[nodiscard]] inline	constexpr	T*	Get()const noexcept
		{
			if (id_ != util::GetUniqueTypeID<T>())
			{
				return nullptr;
			}
			return address_;
		}

		[[nodiscard]] inline	constexpr	bool	IsEmpty()const noexcept {
			return address_ == nullptr;
		}

	private:
		void* address_;
		u32 id_;
	};
}