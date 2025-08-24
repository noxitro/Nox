//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	reflection_generated_register.h
///	@brief	reflection_generatedプロジェクトからのみ参照する
#pragma once

namespace nox::reflection
{
	//	前方宣言
	class ClassInfo;
	class EnumInfo;
	class VariableInfo;
	class FunctionInfo;

	/// @brief 初期化
	void	InitializeGen();

	/// @brief 破棄
	void	FinalizeGen();

#pragma region 登録処理
	void Register(const nox::reflection::ClassInfo& data);
	void Unregister(const nox::reflection::ClassInfo& data);

	void Register(const nox::reflection::EnumInfo& data);
	void Unregister(const nox::reflection::EnumInfo& data);

	void Register(const nox::reflection::VariableInfo& data);
	void Unregister(const nox::reflection::VariableInfo& data);

	void Register(const nox::reflection::FunctionInfo& data);
	void Unregister(const nox::reflection::FunctionInfo& data);

#pragma endregion
}