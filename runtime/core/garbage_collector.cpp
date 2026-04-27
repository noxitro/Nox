//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	garbage_collector.cpp
///	@brief	garbage_collector
#include	"pch.h"
#include	"garbage_collector.h"

#include	"managed_object.h"

namespace nox
{
	class GarbageCollector::Impl
	{
	public:
		nox::os::Mutex mutex_;
		nox::Vector<std::reference_wrapper<class nox::ManagedObject>> destroy_objects_;
		nox::Vector<std::reference_wrapper<class nox::ManagedObject>> managed_objects_;
	};
}

void	nox::GarbageCollector::Register(nox::ManagedObject& managed_object)
{
	NOX_LOCAL_SCOPE(nox::os::Mutex{ impl_->mutex_ });
	impl_->managed_objects_.emplace_back(managed_object);
}

void	nox::GarbageCollector::Initialize(nox::Application&)
{
	impl_ = new Impl();
}

void	nox::GarbageCollector::Finalize(nox::Application&)
{
	delete impl_;
	impl_ = nullptr;
}

void	nox::GarbageCollector::FrameGC(nox::Application&)
{
	if (impl_->destroy_objects_.size() > 0)
	{
		for (nox::ManagedObject& managed_object : impl_->destroy_objects_)
		{
			managed_object.ReleaseRef();
		}
		impl_->destroy_objects_.clear();
	}
	
	const auto result = std::ranges::remove_if(impl_->managed_objects_, +[](const nox::ManagedObject& managed_object)noexcept 
		{
			return managed_object.GetRefCount() < 0;
		});
	
	impl_->destroy_objects_.insert(impl_->destroy_objects_.end(), result.begin(), result.end());
	impl_->managed_objects_.erase(result.begin(), result.end());
}

std::span<const nox::EngineSystem::PhaseRegister> nox::GarbageCollector::GetPhaseRegisterList()const noexcept
{
	static constexpr auto table = std::array{
		PhaseRegister(k_phase_init),
		PhaseRegister(k_phase_gc_update),
		PhaseRegister(k_phase_terminal)
	};
	return table;
}