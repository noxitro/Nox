//	Copyright (c) 2025 NOX ENGINE All rights reserved.

///	@file	resource_manager.cpp
///	@brief	resource_manager
#include	"pch.h"
#include	"resource_manager.h"

#include	<filesystem>

namespace nox::io
{
	
}

nox::Resource* nox::ResourceManager::GetResourceImpl(std::u8string_view path)
{
	//	拡張子を取得
	//std::filesystem::path file_path = std::filesystem::path(path);

	//const char16* n = file_path.extension().c_str();

	return nullptr;
}

std::span<const nox::SystemBase::PhaseRegister> nox::ResourceManager::GetPhaseRegisterList()const noexcept
{
	return {};
}