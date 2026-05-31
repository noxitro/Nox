//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	resource_manager.h
///	@brief	resource_manager
#pragma once
#include	"system.h"

namespace nox
{
	namespace util
	{
		/// @brief uriをnative pathに変換する
		/// @param uri 
		/// @return 
		nox::U8FixedString<nox::os::k_max_path_length> GetNativeResourcePath(std::u8string_view uri);
	}

	class Resource;

	class ResourceManager : public nox::SystemBase
	{
		NOX_DECLARE_OBJECT(ResourceManager, nox::SystemBase);
	public:
		template<std::derived_from<nox::Resource> T>
		inline nox::IntrusivePtr<T> GetResource(std::u8string_view file_path)
		{
			nox::IntrusivePtr<T> resource;
			resource.Reset(static_cast<T*>(GetResourceImpl(file_path)));
			return resource;
		}

		inline nox::IntrusivePtr<nox::Resource> GetResource(std::u8string_view file_path)
		{
			nox::IntrusivePtr<nox::Resource> resource;
			resource.Reset(GetResourceImpl(file_path));
			return resource;
		}

	private:
		void Initialize(nox::World& world);
		void Finalize(nox::World& world);
		nox::Resource* GetResourceImpl(std::u8string_view uri);

		std::span<const nox::SystemBase::PhaseRegister> GetPhaseRegisterList()const noexcept override;

		void ProcLoad(nox::World& world);

	public:
		static constexpr SystemPhaseInit k_phase_init{
			&ResourceManager::Initialize,
			u8"ResourceManager::Initialize"
		};

		static constexpr SystemPhaseTerminate k_phase_terminal{
			&ResourceManager::Finalize,
			u8"ResourceManager::Finalize"
		};

	private:
		nox::UnorderedMap<std::u8string_view, nox::Resource*> resource_cache_;
		nox::UnorderedMap<std::u8string_view, std::reference_wrapper<const nox::reflection::ClassInfo>> resource_typeinfo_map_with_extension_;
		bool is_resource_class_cache_built_ = false;

		nox::Vector<nox::Resource*> load_queue_;
		nox::os::Thread load_thread_;
		mutable nox::os::ReadWriteLock rw_lock_;
	};
}