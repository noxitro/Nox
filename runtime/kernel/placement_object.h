//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	placement_object.h
///	@brief	placement_object
#pragma once
#include	<type_traits>
#include	<memory>
#include	"assertion_kernel.h"
#include	"basic_type.h"

namespace nox
{
	namespace detail
	{
		template<class T>
		concept PlacementObjectDestructible = requires(T& x) 
		{
			x.~T();
		};
	}

	/// @brief 配置オブジェクト管理クラス
	/// @details 指定されたポインタに配置されたオブジェクトを管理し、破棄時にデストラクタを呼び出す
	/// @tparam T 
	template<class T>
	class PlacementObject final
	{
		template<class U>
		friend class PlacementObject;

	public:
		inline constexpr PlacementObject()noexcept :
			ptr_(nullptr)
		{
		}

		inline constexpr PlacementObject(T* ptr)noexcept:
			ptr_(ptr)
		{
		}

		inline constexpr PlacementObject(const PlacementObject&) = delete;
		inline constexpr PlacementObject(PlacementObject&& other)noexcept :
			ptr_(other.ptr_)
		{
			other.ptr_ = nullptr;
		}

		template<std::derived_from<T> U>
		inline constexpr PlacementObject(PlacementObject<U>&& other)noexcept :
			ptr_(other)
		{
			other.ptr_ = nullptr;
		}

		inline constexpr ~PlacementObject()
		{
			if (ptr_ != nullptr)
			{
				//	ここでコンパイルエラーになるなら、このメンバを持つデストラクタがヘッダで実装されている可能性があります
				if constexpr (nox::detail::PlacementObjectDestructible<T>)
				{
					std::destroy_at(ptr_);
					ptr_ = nullptr;
				}
				else
				{
					NOX_ASSERT_KERNEL(false, u"ここには来ないはず");
				}
			}
		}

		inline constexpr operator const T* ()const noexcept
		{
			return ptr_;
		}

		inline constexpr operator T*() noexcept
		{
			return ptr_;
		}

		inline constexpr T* operator->() noexcept
		{
			return ptr_;
		}

		inline constexpr const T* operator->()const noexcept
		{
			return ptr_;
		}

		/// @brief span バッファ上にオブジェクトを配置構築する
		/// @details サイズ・アラインメントを検証し、PlacementObject で自動破棄を保証する
		template<class... Args>
		[[nodiscard]] static inline PlacementObject<T> Construct(std::span<nox::uint8> storage, Args&&... args)
		{
			NOX_ASSERT_KERNEL(storage.size() >= sizeof(T), u"PlacementObject::Construct: バッファサイズが不足しています");
			NOX_ASSERT_KERNEL(reinterpret_cast<std::uintptr_t>(storage.data()) % alignof(T) == 0, u"PlacementObject::Construct: アラインメントが不正です");

			T* ptr = std::construct_at(reinterpret_cast<T*>(storage.data()), std::forward<Args>(args)...);
			return ptr;
		}
	private:
		T* ptr_;
	};
}