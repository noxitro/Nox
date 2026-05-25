//	Copyright (C) 2026 NOX ENGINE All rights reserved.

///	@file	pmr_buffer.h
///	@brief	PMR アリーナアロケータ
#pragma once
#include	"pmr.h"
#include	"../advanced_type.h"
#include	"../assertion_kernel.h"

namespace nox
{
	template<class T>
	using PmrVector = std::pmr::vector<T>;

	template<class T>
	using PmrDeque = std::pmr::deque<T>;

	template<class T>
	using PmrList = std::pmr::list<T>;

	template<class T>
	using PmrStlString = std::pmr::basic_string<T>;

	namespace detail
	{
		template<std::size_t _Size, std::size_t _Alignment = alignof(std::max_align_t)>
		class StackMemoryResource : public std::pmr::memory_resource
		{
		public:
			static constexpr std::size_t Size = _Size;
		public:
			inline constexpr StackMemoryResource() noexcept :
				pos_(buffer_.data())
			{
			}

			inline constexpr StackMemoryResource(const StackMemoryResource&) noexcept = delete;
			inline constexpr StackMemoryResource(const StackMemoryResource&&) noexcept = delete;

		protected:
			[[nodiscard]] inline constexpr void* do_allocate(std::size_t bytes, std::size_t alignment) override
			{
				const std::uintptr_t current_address = reinterpret_cast<std::uintptr_t>(pos_);
				const std::uintptr_t aligned_address = (current_address + alignment - 1) & ~(alignment - 1);
				const std::size_t padding = aligned_address - current_address;
				if (aligned_address + bytes > reinterpret_cast<std::uintptr_t>(buffer_.data()) + Size)
				{
					NOX_ASSERT_KERNEL(false, u"スタックアロケータの容量を超えました");
				}
				pos_ += padding + bytes;
				return reinterpret_cast<void*>(aligned_address);
			}

			inline void do_deallocate(void* ptr, std::size_t bytes, std::size_t alignment) noexcept override
			{
				// スタックアロケータは個別の解放をサポートしないため、何もしない
			}

			[[nodiscard]] inline constexpr bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override
			{
				return this == &other;
			}

		private:
			alignas(_Alignment) std::array<std::uint8_t, _Size> buffer_;
			std::uint8_t* pos_;
		};

		class SpanMemoryResource : public std::pmr::memory_resource
		{
		public:
			inline constexpr explicit SpanMemoryResource(std::span<std::uint8_t> buffer) noexcept :
				buffer_(buffer),
				pos_(buffer_.data())
			{
			}
			inline constexpr SpanMemoryResource(const SpanMemoryResource&) noexcept = delete;
			inline constexpr SpanMemoryResource(const SpanMemoryResource&&) noexcept = delete;

		protected:
			[[nodiscard]] inline constexpr void* do_allocate(std::size_t bytes, std::size_t alignment) override
			{
				const std::uintptr_t current_address = reinterpret_cast<std::uintptr_t>(pos_);
				const std::uintptr_t aligned_address = (current_address + alignment - 1) & ~(alignment - 1);
				const std::size_t padding = aligned_address - current_address;
				if (aligned_address + bytes > reinterpret_cast<std::uintptr_t>(buffer_.data()) + buffer_.size())
				{
					NOX_ASSERT_KERNEL(false, u"SpanMemoryResource の容量を超えました");
				}
				pos_ += padding + bytes;
				return reinterpret_cast<void*>(aligned_address);
			}

			inline constexpr void do_deallocate(void*, std::size_t, std::size_t) noexcept override
			{
			}

			[[nodiscard]] inline constexpr bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override
			{
				return this == &other;
			}

		private:
			std::span<std::uint8_t> buffer_;
			std::uint8_t* pos_;

		};

		template<class _PmrContainer, std::size_t _Size, std::size_t _Alignment = alignof(std::max_align_t)>
		class StackMemoryContainer
		{
		public:
			using ContainerType = _PmrContainer;

		public:
			inline constexpr explicit StackMemoryContainer() noexcept :
				container_(&memory_resource_)
			{
			}

			inline constexpr _PmrContainer* operator->() noexcept { return &container_; }
			inline constexpr const _PmrContainer* operator->() const noexcept { return &container_; }

			inline constexpr _PmrContainer& GetContainer() noexcept { return container_; }
			inline constexpr const _PmrContainer& GetContainer() const noexcept { return container_; }

		private:
			nox::detail::StackMemoryResource<_Size, _Alignment> memory_resource_;
			_PmrContainer container_;
		};

		template<class _PmrContainer>
		class SpanMemoryContainer
		{
		public:
			using ContainerType = _PmrContainer;
		public:
			inline constexpr explicit SpanMemoryContainer(std::span<std::uint8_t> buffer) noexcept :
				memory_resource_(buffer),
				container_(&memory_resource_)
			{
			}

			inline constexpr _PmrContainer* operator->() noexcept { return &container_; }
			inline constexpr const _PmrContainer* operator->() const noexcept { return &container_; }

			inline constexpr _PmrContainer& GetContainer() noexcept { return container_; }
			inline constexpr const _PmrContainer& GetContainer() const noexcept { return container_; }
		private:
			nox::detail::SpanMemoryResource memory_resource_;
			_PmrContainer container_;
		};
	}

	template<class T, std::size_t Size, std::size_t Alignment = alignof(T)>
	using StackAllocVector = nox::detail::StackMemoryContainer<std::pmr::vector<T>, Size, Alignment>;

	template<class T, std::size_t Size, std::size_t Alignment = alignof(T)>
	using SpanAllocVector = nox::detail::SpanMemoryContainer<std::pmr::vector<T>>;

}