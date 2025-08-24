//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	resource_manager.h
///	@brief	resource_manager
#pragma once
#include	"object.h"
namespace nox
{
	class Resource;
	
	class ResourceManager : public nox::Object
	{
		NOX_DECLARE_OBJECT(ResourceManager, nox::Object);
	public:
		template<std::derived_from<nox::Resource> T>
		inline nox::IntrusivePtr<T> GetResource(std::u32string_view file_path)
		{
			return nox::IntrusivePtr<T>(GetResourceImpl(file_path));
		}

		inline nox::IntrusivePtr<Resource> GetResource(std::u32string_view file_path)
		{
			return nox::IntrusivePtr(GetResourceImpl(file_path));
		}

	private:
		Resource* GetResourceImpl(std::u32string_view file_path);
	};
}