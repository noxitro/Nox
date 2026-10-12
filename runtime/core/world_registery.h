//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	world_registery.h
///	@brief	world_registery
#pragma once

namespace nox
{
	struct NodeDescriptor;
	/// @brief		NodeDescriptorの収集
	/// @details	EntitySytem, EntityLogic, Serviceの登録を行う
	class WorldRegistery final
	{
	public:
		inline constexpr explicit WorldRegistery(nox::PmrVector<const nox::NodeDescriptor*>& node_descriptors)noexcept
			: node_descriptors_(node_descriptors)
		{
		}

		template<class T> 
		inline void Register()
		{
			// GetNodeDescriptorを持っている
			if constexpr (requires { T::GetNodeDescriptor(); })
			{
				node_descriptors_.emplace_back(&T::GetNodeDescriptor());
			}
		}

		template<class... Types> requires (sizeof...(Types) > 1)
		inline void Register()
		{
			(Register<Types>(), ...); 
		}
	private:
		

	private:
		nox::PmrVector<const nox::NodeDescriptor*>& node_descriptors_;
	};
}