//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	stack.h
///	@brief	stackクラス
#pragma once
#include	"advanced_type.h"
#include	"os/atomic.h"

namespace nox
{
	/// @brief 固定長スタック（非同期対応版）
	/// @tparam T 型
	/// @tparam _Size サイズ
	template<class T, nox::uint32 _Size>
	class FixedStack
	{
	public:
		using ValueType = T;
		static constexpr nox::uint32 Size = _Size;

	public:
		inline constexpr FixedStack()noexcept :
			stack_{},
			top_(-1)
		{
		}

		inline constexpr FixedStack(const FixedStack&other)noexcept:
			stack_(other.stack_),
			top_(other.top_)
		{
		}
		inline constexpr FixedStack(FixedStack&&other)noexcept:
			stack_(std::move(other.stack_)),
			top_(other.top_)
		{
			other.top_ = -1;
		}

		inline	constexpr explicit FixedStack(const std::array<T, _Size>& other)noexcept :
			stack_(other),
			top_(-1)
		{
		}

		inline	constexpr void PushSyncSafe(const T& value)
		{
			stack_[top_++] = value;
		}

		inline	void PushAsync(const T& value)
		{
			const nox::int32 index = nox::os::atomic::Read(top_);
			NOX_ASSERT(index < static_cast<nox::int32>(Size - 1), U"Stack overflow");
			stack_[nox::os::atomic::Increment(top_)] = value;
		}

		inline	void PushAsync(T&& value)
		{
			const nox::int32 index = nox::os::atomic::Read(top_);
			NOX_ASSERT(index < static_cast<nox::int32>(Size - 1), U"Stack overflow");
			stack_[nox::os::atomic::Increment(top_)] = value;
		}

		inline	T& PopAsync()
		{
			const nox::int32 index = nox::os::atomic::Read(top_);
			NOX_ASSERT(index >= 0, U"Stack underflow");
			nox::os::atomic::Decrement(top_);
			return stack_[index];
		}

		inline	T& Peek()
		{
			const nox::int32 index = nox::os::atomic::Read(top_);
			NOX_ASSERT(index >= 0, U"Stack underflow");
			return stack_[index];
		}

		inline constexpr nox::uint32 GetSize()const noexcept
		{
			return static_cast<nox::uint32>(nox::os::atomic::Read(top_) + 1);
		}
	private:
		std::array<T, Size> stack_;
		nox::int32 top_;
	};
}