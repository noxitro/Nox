//	Copyright (C) 2026 NOX ENGINE All rights reserved.

///	@file	ring_buffer.h
///	@brief	ring_buffer
#pragma once
#include    "advanced_type.h"

namespace nox
{
    template<std::integral T>
    inline constexpr bool IsPowOf(T value, T base)noexcept
    {
        if (base < 2 || value < 1) return false;
        // 2のべき乗はビット演算で高速化
        if (base == 2)
            return (value & (value - 1)) == 0;
        // 一般のべき乗判定
        while (value % base == 0) {
            value /= base;
        }
        return value == 1;
    }

    template<class T>
    class RingBuffer
    {
    public:
		using value_type = T;
    public:
        inline explicit RingBuffer(nox::uint32 capacity)  :
            buffer_(new T[capacity]),
            capacity_(capacity),
            head_(0),
            tail_(0)
        {
        }

        inline ~RingBuffer() 
        {
            delete[] buffer_;
		}

        [[nodiscard]] constexpr bool Empty() const noexcept { return head_ == tail_; }
        [[nodiscard]] constexpr bool Full() const noexcept { return ((tail_ + 1) % capacity_) == head_; }
		[[nodiscard]] constexpr nox::uint32 Size() const noexcept { return (tail_ + capacity_ - head_) % capacity_; }

        inline constexpr bool Push(const T& value) noexcept
        {
            if (Full()) return false;
            buffer_[tail_] = value;
            tail_ = (tail_ + 1) & GetMask();
            return true;
        }
        inline constexpr  bool Push(T&& value) noexcept
        {
            if (Full()) return false;
            buffer_[tail_] = std::move(value);
            tail_ = (tail_ + 1) & GetMask();
            return true;
        }
        inline constexpr   bool Pop(T& out) noexcept
        {
            if (Empty()) return false;
            out = std::move(buffer_[head_]);
            head_ = (head_ + 1) & GetMask();
            return true;
        }
        inline constexpr void Clear() noexcept { head_ = tail_ = 0; }
    private:
		inline constexpr nox::uint32 GetMask() const noexcept { return capacity_ - 1; }
    private:
        T*const buffer_;
		const nox::uint32 capacity_;
        nox::uint32 head_;
        nox::uint32 tail_;
    };

	template<class T, nox::uint32 _Capacity> requires(nox::IsPowOf(_Capacity, 2U) == true)
    class FixedRingBuffer
    {
    public:
		using value_type = T;
		static constexpr nox::uint32 Capacity = _Capacity;
    public:
        inline constexpr FixedRingBuffer() noexcept :
            buffer_{},
			head_(0),
			tail_(0)
        {
        }

        [[nodiscard]] constexpr bool Empty() const noexcept { return head_ == tail_; }
        [[nodiscard]] constexpr bool Full() const noexcept { return ((tail_ + 1) & GetMask()) == head_; }
        [[nodiscard]] constexpr nox::uint32 Size() const noexcept { return (tail_ - head_) & GetMask(); }

        inline constexpr bool Push(const T& value) noexcept
        {
            if (Full()) return false;
            buffer_[tail_] = value;
            tail_ = (tail_ + 1) & GetMask();
            return true;
        }
        inline constexpr  bool Push(T&& value) noexcept
        {
            if (Full()) return false;
            buffer_[tail_] = std::move(value);
            tail_ = (tail_ + 1) & GetMask();
            return true;
        }
        inline constexpr   bool Pop(T& out) noexcept
        {
            if (Empty()) return false;
            out = std::move(buffer_[head_]);
            head_ = (head_ + 1) & GetMask();
            return true;
        }
        inline constexpr void Clear() noexcept { head_ = tail_ = 0; }

    private:
		inline constexpr nox::uint32 GetMask() const noexcept { return _Capacity - 1; }

    private:
        std::array<T, _Capacity> buffer_;
        nox::uint32 head_;
        nox::uint32 tail_;
    };
}