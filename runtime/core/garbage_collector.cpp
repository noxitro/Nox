//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	garbage_collector.cpp
///	@brief	garbage_collector
#include	"pch.h"
#include	"garbage_collector.h"

#include	"object.h"

namespace nox
{
	class GarbageCollector::Impl
	{
	public:
		nox::os::Mutex mutex_;
		nox::Vector<std::reference_wrapper<class nox::Object>> destroy_objects_;
		nox::Vector<std::reference_wrapper<class nox::Object>> managed_objects_;
	};
}

void	nox::GarbageCollector::Register(nox::Object& managed_object)
{
	NOX_LOCAL_SCOPE(nox::os::Mutex{ impl_->mutex_ });
	impl_->managed_objects_.emplace_back(managed_object);
}

void	nox::GarbageCollector::Initialize([[maybe_unused]] nox::World&)
{
	impl_ = new Impl();
}

void	nox::GarbageCollector::Finalize([[maybe_unused]] nox::World&)
{
	delete impl_;
	impl_ = nullptr;
}

void	nox::GarbageCollector::FrameGC([[maybe_unused]] nox::World&)
{
	if (impl_->destroy_objects_.size() > 0)
	{
		for (nox::Object& managed_object : impl_->destroy_objects_)
		{
			nox::detail::ObjectImpl::Release(managed_object);
		}
		impl_->destroy_objects_.clear();
	}
	
	const auto result = std::ranges::remove_if(impl_->managed_objects_, +[](const nox::Object& managed_object)noexcept 
		{
			return nox::detail::ObjectImpl::GetRefCount(managed_object) < 0;
		});
	
	impl_->destroy_objects_.insert(impl_->destroy_objects_.end(), result.begin(), result.end());
	impl_->managed_objects_.erase(result.begin(), result.end());
}

std::span<const nox::SystemBase::PhaseRegister> nox::GarbageCollector::GetPhaseRegisterList()const noexcept
{
	static constexpr auto table = std::array{
		PhaseRegister(k_phase_init),
		PhaseRegister(k_phase_gc_update),
		PhaseRegister(k_phase_terminal)
	};
	return table;
}