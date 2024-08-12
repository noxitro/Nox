//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	multicast_delegate.h
///	@brief	multicast_delegate
#pragma once
#include	"delegate.h"

namespace nox
{
	template<class _FuncType>
	class IMulticastDelegate
	{

	};

	/// @brief マルチキャストデリゲート
	/// @tparam _FuncType 関数型
	/// @tparam _BufferSize 関数型や関数オブジェクト型の最大サイズ
	template<class _FuncType, uint32 _BufferSize = 24>
	class MulticastDelegate : public IMulticastDelegate<_FuncType>
	{
	private:
		using DelegateType = Delegate<_FuncType, _BufferSize>;

	public:
		inline MulticastDelegate()noexcept {}

		inline constexpr MulticastDelegate(const MulticastDelegate& other)noexcept:
			delegate_list_(other.delegate_list_)
		{}

		inline constexpr MulticastDelegate(MulticastDelegate&& other)noexcept
			: delegate_list_(std::move(other.delegate_list_))
		{
		}

		inline explicit MulticastDelegate(const DelegateType& func)
		{
			delegate_list_.emplace_back(func);
		}

		template<class T> requires(std::is_constructible_v< DelegateType, T> == true)
		inline explicit MulticastDelegate(T&& func)
		{
			Add(std::forward<std::decay_t<T>>(func));
		}

		template<class T, class U> requires(std::is_constructible_v< DelegateType, T, U> == true)
			inline explicit MulticastDelegate(T&& func, U&& instance)
		{
			Add(std::forward<std::decay_t<T>>(func), std::forward<std::decay_t<U>>(instance));
		}

		inline	constexpr	void Add(const MulticastDelegate& other)
		{
			delegate_list_.insert_range(delegate_list_.end(), other.delegate_list_);
		}

		/// @brief 直接バインド
		/// @param func 
		inline	constexpr	void Add(const DelegateType& func)
		{
			delegate_list_.emplace_back(func);
		}

		/// @brief メンバ関数のバインド
		template<class T, class U> requires(std::is_constructible_v< DelegateType, std::decay_t<T>, U> == true)
			inline constexpr void Add(T&& func, U&& instance)
		{
			delegate_list_.emplace_back(DelegateType(std::forward<std::decay_t<T>>(func), std::forward<std::decay_t<U>>(instance)));
		}

		/// @brief 通常関数のバインド
		template<class T> requires((nox::IsGlobalFunctionPointerValue<T> || nox::IsFunctionObjectValue<std::decay_t<T>>) && std::is_constructible_v< DelegateType, std::decay_t<T>> == true)
			inline constexpr void Add(T&& func)
		{
			delegate_list_.emplace_back(DelegateType(std::forward<std::decay_t<T>>(func)));
		}

		inline	constexpr	void Remove(const MulticastDelegate& other)
		{
			util::RemoveEraseIf(delegate_list_, [&other](const DelegateType& v) 
				{
					if (std::ranges::find(other.delegate_list_, v) == other.delegate_list_.end())
					{
						return false;
					}
					return true;
				}
			);
		}

		/// @brief 直接登録解除
		/// @param d delegate
		inline	constexpr void Remove(const DelegateType& d)
		{
			util::RemoveErase(delegate_list_, d);
		}

		/// @brief メンバ関数の登録解除
		template<class T, class U> requires(std::is_constructible_v< DelegateType, std::decay_t<T>, U> == true)
		inline void Remove(T&& func, U&& instance)
		{
			util::RemoveEraseIf(delegate_list_, [&func, &instance](const DelegateType& v) {return v.Equal(std::forward<std::decay_t<T>>(func), std::forward<std::decay_t<U>>(instance)) == true; });
		}

		/// @brief 関数、関数オブジェクトの登録解除
		template<class T> requires((nox::IsGlobalFunctionPointerValue<T> || nox::IsFunctionObjectValue<std::decay_t<T>>) && std::is_constructible_v< DelegateType, std::decay_t<T>> == true)
		inline void Remove(T&& func)
		{
			util::RemoveEraseIf(delegate_list_, [&func](const DelegateType& v) {return v.Equal(std::forward<std::decay_t<T>>(func)) == true; });
		}

		inline constexpr uint32 GetSize()const noexcept { return static_cast<uint32>(delegate_list_.size()); }
		inline constexpr MulticastDelegate& operator=(MulticastDelegate&& other)noexcept
		{
			delegate_list_ = std::move(other.delegate_list_);
			return *this;
		}

		inline constexpr bool operator==(const MulticastDelegate& other)const noexcept
		{
			return delegate_list_ == other.delegate_list_;
		}
	private:
		nox::Vector<DelegateType> delegate_list_;
	};
}