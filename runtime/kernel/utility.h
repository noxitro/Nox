//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	utility.h
///	@brief	utility
#pragma once
#include <functional>
#include <type_traits>

#include "advanced_definition.h"
#include "assertion_kernel.h"

namespace nox::util
{
	/// @brief		遅延参照
	/// @details	初期化時はnullptrで、アクセス時には参照先が存在することが保証される
	template<class T>
	class InitOnceRef
	{
	public:
		using Type = T;

	public:
		inline constexpr InitOnceRef() noexcept :
			ptr_(nullptr) {
		}

		inline InitOnceRef& operator=(T& value)
		{
			NOX_ASSERT_KERNEL(ptr_ == nullptr, u8"すでに値が設定されています");
			ptr_ = &value;
			return *this;
		}

		inline T& Get() const
		{
			return nox::util::Deref(ptr_);
		}

		inline operator T& () const
		{
			return Get();
		}
	private:
		T* ptr_;
	};

	class ScopeExit
	{
	public:
		inline explicit ScopeExit(const std::function<void()> func)noexcept :
			func_(func)
		{

		}

		inline ~ScopeExit()
		{
			func_();
		}

	private:
		std::function<void()> func_;
	};

	struct INewDeleteDisabled
	{
	/*	static void* operator new(std::size_t) = delete;
		static void* operator new(std::size_t, const std::nothrow_t&) = delete;
		static void* operator new(std::size_t, std::align_val_t) = delete;
		static void* operator new(std::size_t, std::align_val_t, const std::nothrow_t&) = delete;

		static void* operator new[](std::size_t) = delete;
		static void* operator new[](std::size_t, const std::nothrow_t&) = delete;
		static void* operator new[](std::size_t, std::align_val_t) = delete;
		static void* operator new[](std::size_t, std::align_val_t, const std::nothrow_t&) = delete;
								 
		static void  operator delete(void*) = delete;
		static void  operator delete[](void*) = delete;*/
	};

	struct ICopyMoveDisabled
	{
		inline constexpr ICopyMoveDisabled() noexcept = default;
		inline constexpr ICopyMoveDisabled(const ICopyMoveDisabled&) noexcept = delete;
		inline constexpr ICopyMoveDisabled(ICopyMoveDisabled&&) noexcept = delete;
		inline constexpr ICopyMoveDisabled& operator=(const ICopyMoveDisabled&) noexcept = delete;
		inline constexpr ICopyMoveDisabled& operator=(ICopyMoveDisabled&&) noexcept = delete;
	};
}