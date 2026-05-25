//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	resource_manager.h
///	@brief	resource_manager
#pragma once
#include	"system.h"

namespace nox
{
	class Resource;
	
	class ResourceManager : public nox::SystemBase
	{
		NOX_DECLARE_OBJECT(ResourceManager, nox::SystemBase);
	public:

		template<std::derived_from<nox::Resource> T>
		inline nox::IntrusivePtr<T> GetResource(std::u8string_view file_path)
		{
			return nox::IntrusivePtr<T>(GetResourceImpl(file_path));
		}

		inline nox::IntrusivePtr<nox::Resource> GetResource(std::u8string_view file_path)
		{
			return nox::IntrusivePtr(GetResourceImpl(file_path));
		}

	private:
		nox::Resource* GetResourceImpl(std::u8string_view path);
		std::span<const nox::SystemBase::PhaseRegister> GetPhaseRegisterList()const noexcept override;
	};
}