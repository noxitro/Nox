//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	dynamic_array.h
///	@brief	dynamic_array
#pragma once
#include	"advanced_type.h"
#include	"assertion.h"

#include	<algorithm>
#include	<memory>
#include	<span>
#include	<utility>

namespace nox
{
	//template<class T>
	//class DynamicArray
	//{
	//public:
	//	using value_type = T;

	//	inline constexpr DynamicArray() noexcept
	//		: data_(nullptr)
	//		, size_(0)
	//		, capacity_(0)
	//	{
	//	}

	//	// 要素を size 個、値初期化で構築（配置new）
	//	inline explicit DynamicArray(nox::uint32 size)
	//		: data_(nullptr)
	//		, size_(0)
	//		, capacity_(0)
	//	{
	//		if (size == 0) return;
	//		Allocate(size);
	//		std::uninitialized_value_construct_n(data_, size);
	//		size_ = size;
	//	}

	//	// span からコピー構築
	//	inline explicit DynamicArray(std::span<const T> span)
	//		: data_(nullptr)
	//		, size_(0)
	//		, capacity_(0)
	//	{
	//		if (span.size() == 0) return;
	//		Allocate(static_cast<nox::uint32>(span.size()));
	//		std::uninitialized_copy_n(span.data(), span.size(), data_);
	//		size_ = static_cast<nox::uint32>(span.size());
	//	}

	//	// コピーコンストラクタ
	//	inline DynamicArray(const DynamicArray& other)
	//		: data_(nullptr)
	//		, size_(0)
	//		, capacity_(0)
	//	{
	//		if (other.size_ == 0) return;
	//		Allocate(other.size_);
	//		std::uninitialized_copy_n(other.data_, other.size_, data_);
	//		size_ = other.size_;
	//	}

	//	// ムーブコンストラクタ
	//	inline constexpr DynamicArray(DynamicArray&& other) noexcept
	//		: data_(other.data_)
	//		, size_(other.size_)
	//		, capacity_(other.capacity_)
	//	{
	//		other.data_ = nullptr;
	//		other.size_ = 0;
	//		other.capacity_ = 0;
	//	}

	//	inline ~DynamicArray() noexcept
	//	{
	//		DestroyAll();
	//		Deallocate();
	//	}

	//	// コピー代入
	//	inline DynamicArray& operator=(const DynamicArray& other)
	//	{
	//		if (this == &other) return *this;
	//		if (other.size_ == 0)
	//		{
	//			Clear();
	//			return *this;
	//		}

	//		if (other.size_ <= capacity_)
	//		{
	//			// 既存に上書き/追加構築
	//			const nox::uint32 n_common = (std::min)(size_, other.size_);
	//			for (nox::uint32 i = 0; i < n_common; ++i) data_[i] = other.data_[i];
	//			if (size_ < other.size_)
	//			{
	//				std::uninitialized_copy_n(other.data_ + size_, other.size_ - size_, data_ + size_);
	//			}
	//			else if (size_ > other.size_)
	//			{
	//				std::destroy_n(data_ + other.size_, size_ - other.size_);
	//			}
	//			size_ = other.size_;
	//			return *this;
	//		}

	//		// 再確保
	//		DynamicArray tmp(other);
	//		Swap(tmp);
	//		return *this;
	//	}

	//	// ムーブ代入
	//	inline DynamicArray& operator=(DynamicArray&& other) noexcept
	//	{
	//		if (this == &other) return *this;
	//		DestroyAll();
	//		Deallocate();

	//		data_ = other.data_;
	//		size_ = other.size_;
	//		capacity_ = other.capacity_;

	//		other.data_ = nullptr;
	//		other.size_ = 0;
	//		other.capacity_ = 0;
	//		return *this;
	//	}

	//	// 全要素破棄（構築済み size_ のみ破棄）
	//	inline void Clear() noexcept
	//	{
	//		DestroyAll();
	//		size_ = 0;
	//	}

	//	// 容量確保（要素は構築しない）
	//	inline void Reserve(nox::uint32 new_capacity)
	//	{
	//		if (new_capacity <= capacity_) return;
	//		T* new_data = AllocateOnly(new_capacity);

	//		// 既存要素をムーブ構築
	//		std::uninitialized_move_n(data_, size_, new_data);
	//		std::destroy_n(data_, size_);
	//		Deallocate();

	//		data_ = new_data;
	//		capacity_ = new_capacity;
	//	}

	//	// サイズ変更：拡大時は値初期化で新規構築、縮小時は破棄
	//	inline void Resize(nox::uint32 new_size)
	//	{
	//		if (new_size == size_) return;

	//		if (new_size < size_)
	//		{
	//			std::destroy_n(data_ + new_size, size_ - new_size);
	//			size_ = new_size;
	//			return;
	//		}

	//		// 拡大
	//		if (new_size > capacity_)
	//		{
	//			const nox::uint32 new_cap = new_size;
	//			Reserve(new_cap);
	//		}
	//		std::uninitialized_value_construct_n(data_ + size_, new_size - size_);
	//		size_ = new_size;
	//	}

	//	// アクセス
	//	inline constexpr nox::uint32 Size() const noexcept { return size_; }
	//	inline constexpr bool Empty() const noexcept { return size_ == 0; }
	//	inline T* Data() noexcept { return data_; }
	//	inline const T* Data() const noexcept { return data_; }

	//	inline std::span<T> AsSpan() noexcept
	//	{
	//		return std::span<T>(data_, static_cast<size_t>(size_));
	//	}
	//	inline std::span<const T> AsSpan() const noexcept
	//	{
	//		return std::span<const T>(data_, static_cast<size_t>(size_));
	//	}

	//	// range-for
	//	inline T* begin() noexcept { return data_; }
	//	inline T* end() noexcept { return data_ + size_; }
	//	inline const T* begin() const noexcept { return data_; }
	//	inline const T* end() const noexcept { return data_ + size_; }
	//	inline const T* cbegin() const noexcept { return data_; }
	//	inline const T* cend() const noexcept { return data_ + size_; }

	//	inline T& operator[](nox::uint32 index) noexcept
	//	{
	//		NOX_ASSERT(index < size_, nox::assertion::id::OutOfRange{}, u"index over");
	//		return data_[index];
	//	}
	//	inline const T& operator[](nox::uint32 index) const noexcept
	//	{
	//		NOX_ASSERT(index < size_, nox::assertion::id::OutOfRange{}, u"index over");
	//		return data_[index];
	//	}

	//	inline void Swap(DynamicArray& other) noexcept
	//	{
	//		std::swap(data_, other.data_);
	//		std::swap(size_, other.size_);
	//	}

	//private:
	//	// 生メモリ確保（未初期化）
	//	inline void Allocate(nox::uint32 n)
	//	{
	//		const std::size_t bytes = static_cast<std::size_t>(n) * sizeof(T);
	//		void* const p = new(bytes, std::align_val_t(alignof(T)));

	//		data_ = static_cast<T*>(p);
	//		size_ = n;
	//	}

	//private:
	//	T* data_;
	//	nox::uint32 size_;
	//};
}