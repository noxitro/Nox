//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	type_id.h
///	@brief	type_id
#pragma once

namespace nox
{
	struct TypeId
	{
	protected:
		constexpr TypeId() noexcept = default;
	};

	namespace detail
	{
		struct TypeIdInvalid : TypeId
		{
		};

		constexpr TypeIdInvalid TypeIdInvalidValue;

		template<class T>
		struct TypeIdImpl : TypeId
		{
		};

		template<class T>
		struct TypeIdHolder
		{
			static constexpr TypeIdImpl<T> value;
			inline consteval TypeIdHolder()noexcept = delete;
			inline constexpr ~TypeIdHolder()noexcept = delete;
		};
	}

	template<class T>
	inline consteval const nox::TypeId& GetTypeId()noexcept
	{
		return nox::detail::TypeIdHolder<T>::value;
	}

	inline consteval const nox::TypeId& GetInvalidTypeId()noexcept
	{
		return nox::detail::TypeIdInvalidValue;
	}
}