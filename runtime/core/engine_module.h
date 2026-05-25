//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	engine_module.h
///	@brief	engine_module
#pragma once
#include	"object.h"
#include	"system_phase_type.h"

namespace nox
{
	class EngineModule;
	class SystemBase;

	/// @brief		モジュールエントリ基底クラス
	///	@details	nox::Worldで収集され、各フェーズで呼び出される関数を登録するための基底クラス
	class EngineModule : public nox::Object
	{
		NOX_DECLARE_OBJECT(nox::EngineModule, nox::Object);
	public:
		inline constexpr EngineModule()noexcept = default;
		virtual ~EngineModule() = default;

		virtual void CreateEngineSystems([[maybe_unused]] nox::PmrVector<nox::SystemBase*>& out)const {}
	};
}