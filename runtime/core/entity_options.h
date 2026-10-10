//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	entity_options.h
///	@brief	entity_options
#pragma once

namespace nox
{
	namespace detail
	{
		struct UpdaterNodeOwnerBase {};
	}

	struct IComponentData;
	

	/// @brief 必要なコンポーネントを指定
	/// @tparam ...Types 
	template<class... Types> //requires ( (std::is_base_of_v<nox::detail::IComponentData, Types> &&  std::is_same_v<nox::detail::IComponentData, Types> == false && ...))
	struct RequireComponents final {};

	/// @brief 除外するコンポーネントを指定
	/// @tparam ...Types 
	template<class... Types> //requires ( (std::is_base_of_v<nox::detail::IComponentData, Types> &&  std::is_same_v<nox::detail::IComponentData, Types> == false && ...))
	struct ExcludeComponents final {};

	/// @brief このシステムの前に実行されるシステムを指定
	/// @tparam ...Types 
	template<class... Types> //requires ( (std::is_base_of_v<nox::detail::UpdaterNodeOwnerBase, Types> &&  std::is_same_v<nox::detail::UpdaterNodeOwnerBase, Types> == false && ...))
	struct RunAfter final{};
	
	/// @brief このシステムの後に実行されるシステムを指定
	/// @tparam ...Types 
	template<class... Types> //requires ( (std::is_base_of_v<nox::detail::UpdaterNodeOwnerBase, Types> &&  std::is_same_v<nox::detail::UpdaterNodeOwnerBase, Types> == false && ...))
	struct RunBefore final{};

	namespace detail
	{
		template<class>
		struct is_require_components : std::false_type {};

		template<class... Types>
		struct is_require_components<nox::RequireComponents<Types...>> : std::true_type {};

		template<class>
		struct is_exclude_components : std::false_type {};

		template<class... Types>
		struct is_exclude_components<nox::ExcludeComponents<Types...>> : std::true_type {};

		template<class>
		struct is_run_after : std::false_type {};

		template<class... Types>
		struct is_run_after<nox::RunAfter<Types...>> : std::true_type {};

		template<class>
		struct is_run_before : std::false_type {};

		template<class... Types>
		struct is_run_before<nox::RunBefore<Types...>> : std::true_type {};

		template<class T>
		constexpr static bool is_require_components_v = nox::detail::is_require_components<T>::value;

		template<class T>
		constexpr static bool is_exclude_components_v = nox::detail::is_exclude_components<T>::value;

		template<class T>
		constexpr static bool is_run_after_v = nox::detail::is_run_after<T>::value;

		template<class T>
		constexpr static bool is_run_before_v = nox::detail::is_run_before<T>::value;
	}

}