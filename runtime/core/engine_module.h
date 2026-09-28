//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	engine_module.h
///	@brief	engine_module
#pragma once
#include	"object.h"
#include	"system_phase_type.h"

namespace nox
{
	class EngineModule;
	class SystemBase;
	class World;

	/// @brief		モジュールエントリ基底クラス
	///	@details	nox::Worldで収集され、各フェーズで呼び出される関数を登録するための基底クラス
	class EngineModule : public nox::Object
	{
		NOX_DECLARE_OBJECT(nox::EngineModule, nox::Object);
	public:
		inline constexpr EngineModule()noexcept = default;
		virtual ~EngineModule() = default;

		/// @brief モジュールが持つServiceを登録する。 nox::World::RegisterService を呼ぶ。
		/// @details World::Init が CreateEngineSystems より前に全モジュールで呼ぶ。
		///          全モジュールの登録が済んでから、Depends の順に初期化される(登録順は使わない)。
		virtual void RegisterServices([[maybe_unused]] nox::World& world)const {}

		virtual void CreateEngineSystems([[maybe_unused]] nox::PmrVector<nox::SystemBase*>& out)const {}
	};
}