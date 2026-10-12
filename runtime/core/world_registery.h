//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	world_registery.h
///	@brief	world_registery
#pragma once

namespace nox::legacy
{
	class World;

	/// @brief	TODO: 新しい World が Service を実行するまでの暫定。定義は world_legacy.h
	template<class T>
	void AddRegisteredType(nox::legacy::World& world);
}

namespace nox
{
	struct NodeDescriptor;
	/// @brief		NodeDescriptorの収集
	/// @details	EntitySytem, EntityLogic, Serviceの登録を行う
	class WorldRegistery final
	{
	public:
		/// @param legacy_world	登録した型を legacy::World でも生成するときに渡す。TODO: legacy::World と一緒に消す
		inline constexpr explicit WorldRegistery(nox::PmrVector<const nox::NodeDescriptor*>& node_descriptors, nox::legacy::World* const legacy_world = nullptr)noexcept
			: node_descriptors_(node_descriptors)
			, legacy_world_(legacy_world)
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

			if (legacy_world_ != nullptr)
			{
				nox::legacy::AddRegisteredType<T>(*legacy_world_);
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
		nox::legacy::World* legacy_world_;
	};
}