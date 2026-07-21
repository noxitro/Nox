//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	object.h
///	@brief	object
#pragma once
#include	"object_definition.h"

namespace nox
{
	namespace detail
	{
		struct ObjectImpl;
	}

	/// @brief 基底オブジェクト
	class alignas(16) Object : public ::nox::reflection::ReflectionObject
	{
		NOX_DECLARE_OBJECT_ROOT(Object);
		friend struct ::nox::detail::ObjectImpl;
		//	tagged pointerのビット定義 16bytesアラインメントのため下位4ビットが使用可能
		static constexpr nox::uint8 kTaggedPointerBitHeap = 0b0001;
		static constexpr nox::uint8 kTaggedPointerGC = 0b0010;
		static constexpr nox::uint8 kTaggedPointerReserved1 = 0b0100;
		static constexpr nox::uint8 kTaggedPointerReserved2 = 0b1000;
	public:
		constexpr Object() noexcept {}
		virtual constexpr ~Object() override {}

		inline constexpr bool IsHeap()const noexcept
		{
			return TestTaggedPointerBit(kTaggedPointerBitHeap);
		}

		inline constexpr bool IsGC()const noexcept
		{
			return TestTaggedPointerBit(kTaggedPointerGC);
		}

		/// @brief 文字列化　動的メモリ確保
		/// @return 
		nox::U8String	ToString()const;

		/// @brief 文字列化　バッファ指定
		virtual ::nox::U8StringView	ToString(std::span<::nox::char8> dest_buffer)const;

		inline static void* operator new (size_t size)
		{
			return nox::memory::Allocate(size, nox::memory::InstanceType::Object);
		}

		inline static void operator delete(void* ptr) noexcept
		{
			nox::memory::Deallocate(ptr);
		}

		inline static void* operator new[]([[maybe_unused]] size_t size)
		{
			NOX_ASSERT(false, u"配列のnewはサポートされていません");
		}

		inline static void operator delete[]([[maybe_unused]] void* ptr) noexcept
		{
			NOX_ASSERT(false, u"配列のdeleteはサポートされていません");
		}

		inline static void* operator new(size_t size, std::align_val_t align)
		{
			return nox::memory::Allocate(size, static_cast<std::size_t>(align), nox::memory::InstanceType::Object);
		}

		inline static void* operator new(size_t, void* ptr) noexcept
		{
			return ptr;
		}

		inline static void operator delete(void* ptr, std::align_val_t align) noexcept
		{
			nox::memory::Deallocate(ptr, static_cast<std::size_t>(align));
		}

		inline static void operator delete([[maybe_unused]] void*, [[maybe_unused]] void*) noexcept
		{
		}

		inline static void* operator new[]([[maybe_unused]] size_t size, [[maybe_unused]] std::align_val_t align)
		{
			NOX_ASSERT(false, u"配列のnewはサポートされていません");
		}

		inline static void operator delete[]([[maybe_unused]] void* ptr, [[maybe_unused]] std::align_val_t align) noexcept
		{
			NOX_ASSERT(false, u"配列のdeleteはサポートされていません");
		}

		inline static void* operator new([[maybe_unused]] size_t size, [[maybe_unused]] const std::nothrow_t&) noexcept
		{
			NOX_ASSERT(false, u"nothrow newはサポートされていません");
		}

		inline static void operator delete([[maybe_unused]] void* ptr, [[maybe_unused]] const std::nothrow_t&) noexcept
		{
			NOX_ASSERT(false, u"nothrow deleteはサポートされていません");
		}

		inline static void* operator new[]([[maybe_unused]] size_t size, [[maybe_unused]] const std::nothrow_t&) noexcept
		{
			NOX_ASSERT(false, u"nothrow newはサポートされていません");
		}

		inline static void operator delete[]([[maybe_unused]] void* ptr, [[maybe_unused]] const std::nothrow_t&) noexcept
		{
			NOX_ASSERT(false, u"nothrow deleteはサポートされていません");
		}

		inline static void* operator new([[maybe_unused]] size_t size, [[maybe_unused]] std::align_val_t align, [[maybe_unused]] const std::nothrow_t&) noexcept
		{
			NOX_ASSERT(false, u"nothrow newはサポートされていません");
		}

		inline static void operator delete([[maybe_unused]] void* ptr, [[maybe_unused]] std::align_val_t align, [[maybe_unused]] const std::nothrow_t&) noexcept
		{
			NOX_ASSERT(false, u"nothrow deleteはサポートされていません");
		}

		inline static void* operator new[]([[maybe_unused]] size_t size, [[maybe_unused]] std::align_val_t align, [[maybe_unused]] const std::nothrow_t&) noexcept
		{
			NOX_ASSERT(false, u"nothrow newはサポートされていません");
		}

		inline static void operator delete[]([[maybe_unused]] void* ptr, [[maybe_unused]] std::align_val_t align, [[maybe_unused]] const std::nothrow_t&) noexcept
		{
			NOX_ASSERT(false, u"nothrow deleteはサポートされていません");
		}
	protected:
		inline	std::span<void(*)()> GetVTable()const noexcept { return ::nox::util::GetVTable(this); }

	private:
		void AddRef();
		void Release();

		inline constexpr bool TestTaggedPointerBit(const nox::uint8 bit)const noexcept
		{
			return (reinterpret_cast<std::uintptr_t>(this) & bit) == bit;
		}

	private:
		NOX_ATTR(nox::reflection::attr::IgnoreReflection())
		std::atomic_int32_t ref_count_{ -1 };
	};

	namespace detail
	{
		struct ObjectImpl
		{
			static inline void AddRef(Object& v)
			{
				v.AddRef();
			}

			static inline void Release(Object& v)
			{
				v.Release();
			}

			static inline nox::uint32 GetRefCount(const Object& v) noexcept
			{
				return v.ref_count_.load(std::memory_order_relaxed);
			}
		};
	}

	inline void IntrusivePtrAddReference(::nox::Object& v)
	{
		nox::detail::ObjectImpl::AddRef(v);
	}

	inline void IntrusivePtrReleaseReference(::nox::Object& v)
	{
		nox::detail::ObjectImpl::Release(v);
	}
}