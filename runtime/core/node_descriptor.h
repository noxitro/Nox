//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	node_descriptor.h
///	@brief	node_descriptor
#pragma once
#include	"updater_node.h"

namespace nox
{
	struct NodeDescriptor final
	{

	};

	namespace detail
	{
		template<class T, class List>
		struct NodeDescriptorBuilder;

		template<class T, class... Registers>
		struct NodeDescriptorBuilder<T, std::tuple<Registers...>> final
		{
			static constexpr nox::NodeDescriptor value{};
		};
	}

	template<class T, class... Registers>
	constexpr const nox::NodeDescriptor& node_descriptor_v = nox::detail::NodeDescriptorBuilder<T, Registers...>::value;
}