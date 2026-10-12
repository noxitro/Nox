//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	node_descriptor.h
///	@brief	node_descriptor
#pragma once
#include	"updater_node.h"

namespace nox::detail::updater_graph
{
	template<class T, class... RegisterList>
	struct RegisterTraits;

	template<class T, auto function, nox::detail::updater_graph::NodeTrigger trigger, class... Options>
	struct RegisterTraits<T, nox::detail::updater_graph::Register<function, trigger, Options...>> final
	{
		static constexpr nox::detail::updater_graph::NodeMethod value
		{
			.method_name = nox::util::NameofValue<function>()
		};
	};
}

namespace nox
{
	struct NodeDescriptor final
	{
		/// @brief system/logic/serviceの型情報
		const nox::reflection::Type* type;
		std::span<const nox::detail::updater_graph::NodeMethod> methods;
	};

	namespace detail
	{
		template<class T, class List>
		struct NodeDescriptorBuilder;

		template<class T, class... Registers>
		struct NodeDescriptorBuilder<T, std::tuple<Registers...>> final
		{
		private:
			static constexpr std::array<nox::detail::updater_graph::NodeMethod, sizeof...(Registers)> methods
			{ nox::detail::updater_graph::RegisterTraits<T, Registers>::value... };
		public:
			static constexpr nox::NodeDescriptor value{
				.type = &nox::reflection::Typeof<T>(),
				.methods = methods
			};
		};
	}

	template<class T, class... Registers>
	constexpr const nox::NodeDescriptor& node_descriptor_v = nox::detail::NodeDescriptorBuilder<T, std::tuple<Registers...>>::value;
}