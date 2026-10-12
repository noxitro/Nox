//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	updater_node.h
///	@brief	UpdaterGraphのノード記述(System/Logic/Service共通)
#pragma once

namespace nox::detail
{
	struct UpdaterNodeOwnerBase {};
}

namespace nox
{
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
	struct RunAfter final {};

	/// @brief このシステムの後に実行されるシステムを指定
	/// @tparam ...Types 
	template<class... Types> //requires ( (std::is_base_of_v<nox::detail::UpdaterNodeOwnerBase, Types> &&  std::is_same_v<nox::detail::UpdaterNodeOwnerBase, Types> == false && ...))
	struct RunBefore final {};

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


namespace nox::detail::updater_graph
{
	enum class NodeTrigger : nox::uint8
	{
		Update,
		Add,
		Remove,
		Init,
		Terminate,
	};

	enum class OrderKind : nox::uint8
	{
		Before,
		After,
	};

	/// @brief 引数またはthisの情報
	struct NodeAccess final
	{
		/// @brief 修飾のない型情報
		const nox::reflection::Type* type;
		/// @brief 非constの書き込み可能
		bool is_write : 1;
		/// @brief 引数がオプションかどうか。オプションの場合、引数が存在しない場合はnullptrが渡される
		bool is_optional : 1;
	};

	/// @brief RunAfter/RunBeforeの情報
	struct NodeOrder final
	{
		const nox::reflection::Type* type;
		nox::detail::updater_graph::OrderKind kind;
		std::string_view method_name;
	};

	/// @brief フ	レームで揮発されるノードの実行コンテキスト情報
	/// @details	ECB(EntityCommandBuffer)など
	struct NodeFrameContext final
	{

	};

	/// @brief 実行メソッド情報
	struct NodeMethod final
	{
		std::string_view method_name;
	};

	template<auto Phase, class... Options>
	struct Register final {};

	template<class T>
	struct RegisterBuilder final
	{

	};
}
