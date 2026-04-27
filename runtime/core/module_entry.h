//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	module_entry.h
///	@brief	module_entry
#pragma once
#include	"module_entry_category.h"
#include	"object.h"

namespace nox
{
	class ModuleEntry;
	class Application;
	class EngineSystem;

	/// @brief		モジュールエントリ基底クラス
	///	@details	nox::Applicationで収集され、各フェーズで呼び出される関数を登録するための基底クラス
	class ModuleEntry : public nox::Object
	{
		NOX_DECLARE_OBJECT(nox::ModuleEntry, nox::Object);
	public:
		inline constexpr ModuleEntry()noexcept = default;
		virtual ~ModuleEntry() = default;

		virtual void CreateEngineSystems(nox::PmrVector<nox::EngineSystem*>& out)const {}
	};
}