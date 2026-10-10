//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	dev_entity_logics.h
///	@brief	dev_entity_logics
#pragma once
#include	"entity_logic.h"
#include	"ecs_definitions.h"
#include	"entity.h"

#if NOX_DEVELOP
namespace nox::dev::services
{
	class NameService;
}

namespace nox::dev::entity_logics
{
	class BeforeLogging final : public nox::EntityLogic<BeforeLogging>
	{
		NOX_ECS_DECLARE_VERIFY(BeforeLogging);
	private:
		void OnAdd(nox::Entity) {}
		void OnUpdate(nox::Entity) {}
		void OnRemove(nox::Entity) {}
	public:
		using RegisterList = std::tuple <
			Register<&BeforeLogging::OnAdd, Trigger::Add>,
			Register<&BeforeLogging::OnUpdate, Trigger::Update>,
			Register<&BeforeLogging::OnRemove, Trigger::Remove>
		>;
	};

	class Logging final : public nox::EntityLogic<Logging, 
		nox::RunBefore<nox::dev::entity_logics::BeforeLogging>
	>
	{
		NOX_ECS_DECLARE_VERIFY(Logging);
	private:
		void OnAdd(nox::Entity) {}
		void OnUpdate(nox::Entity) {}
		void OnRemove(nox::Entity) {}
	public:
		using RegisterList = std::tuple <
			Register<&Logging::OnAdd, Trigger::Add>,
			Register<&Logging::OnUpdate, Trigger::Update>,
			Register<&Logging::OnRemove, Trigger::Remove>
		>;
	};
}
#endif // NOX_DEVELOP
