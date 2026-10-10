//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	component.h
///	@brief	component
#pragma once

namespace nox
{
	namespace detail
	{
		/// @brief componentタグ
		struct IComponentData
		{

		};
	}

	namespace concepts
	{
		template<class T>
		concept Component =
			std::is_same_v<T, nox::detail::IComponentData> == false &&
			std::is_base_of_v<nox::detail::IComponentData, T>;
	}

	template<class T>
	struct Component : public nox::detail::IComponentData
	{
	protected:
		static consteval bool StaticDeclareVerify()noexcept
		{
			static_assert(std::is_polymorphic_v<T> == false, "Componentは仮想関数を持てません");
			static_assert(std::is_final_v<T>, "Componentはfinalである必要があります");
			static_assert(std::is_trivially_destructible_v<T>, "Componentはtrivially destructibleである必要があります");
			static_assert(alignof(T) <= 16, "Componentのアラインメントは16byte以下である必要があります");
			return true;
		}
	};
}