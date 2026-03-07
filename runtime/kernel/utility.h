//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	utility.h
///	@brief	utility
#pragma once
#include <functional>
#include <type_traits>

namespace nox::util
{

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