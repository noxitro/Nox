//	Copyright (C) 2026 NOX ENGINE All rights reserved.

///	@file	socket_stream_utility.h
///	@brief	socket_stream_utility
#pragma once
#if NOX_DEVELOP

namespace nox
{
	class ManagedObject;
}

namespace nox::dev::editor_remote
{
	bool IsRemoteVariable(const nox::reflection::VariableInfo& variable_info)noexcept;
	bool IsRemoteFunction(const nox::reflection::FunctionInfo& function_info)noexcept;

	//constexpr nox::uint32 k_default_get_variables_limit = 64;

	///// @brief editorと共有する変数情報のリストを取得
	//nox::FixedVector<const nox::reflection::VariableInfo*, k_default_get_variables_limit> GetRemoteVariableInfoList(const nox::reflection::ClassInfo& class_info);

	///// @brief editorと共有する関数情報のリストを取得
	//std::span<std::reference_wrapper<const nox::reflection::FunctionInfo>> GetRemoteFunctionInfoList(const nox::reflection::ClassInfo& class_info)noexcept;

	std::span<nox::uint8> GetPropertiesBytes(std::span<nox::uint8> buffer, const nox::ManagedObject& obj);
	void SetPropertiesFromBytes(const std::span<const nox::uint8> bytes, nox::ManagedObject& obj);
}

#endif // NOX_DEVELOP