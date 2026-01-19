//	Copyright (C) 2026 NOX ENGINE All rights reserved.

///	@file	placement_object.h
///	@brief	placement_object
#pragma once
#include	<type_traits>
#include	<memory>

namespace nox
{
	namespace detail
	{
		template<class T>
		concept PlacementObjectDestructible = requires(T& x) 
		{
			x.~T();
		};
		void PlacementObjectAbort()noexcept;
	}

	/// @brief 配置オブジェクト管理クラス
	/// @details 指定されたポインタに配置されたオブジェクトを管理し、破棄時にデストラクタを呼び出す
	/// @tparam T 
	template<class T>
	class PlacementObject
	{
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
		inline constexpr PlacementObject(PlacementObject&&) = delete;

		inline constexpr ~PlacementObject()
		{
			if (ptr_ != nullptr)
			{
				if constexpr (nox::detail::PlacementObjectDestructible<T>)
				{
					std::destroy_at(ptr_);
					ptr_ = nullptr;
				}
				else
				{
					nox::detail::PlacementObjectAbort();
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
	private:
		void Abort()noexcept;

	private:
		T* ptr_;
	};
}