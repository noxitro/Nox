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
	class World;

	/// @brief		モジュールエントリ基底クラス
	///	@details	nox::World が起動時(World::Init)に派生型を全て生成し、各モジュールの Service を登録させる。
	///				毎フレームの処理は Service の属性付きメソッドなどのノードとして書く(UpdaterGraph が順序を決める)。
	class EngineModule : public nox::Object
	{
		NOX_DECLARE_OBJECT(nox::EngineModule, nox::Object);
	public:
		inline constexpr EngineModule()noexcept = default;
		virtual ~EngineModule() = default;

		/// @brief モジュールが持つServiceを登録する。 nox::World::RegisterService を呼ぶ。
		/// @details World::Init が全モジュールで呼ぶ。
		///          全モジュールの登録が済んでから、Depends の順に初期化される(登録順は使わない)。
		virtual void RegisterServices([[maybe_unused]] nox::World& world)const {}
	};
}