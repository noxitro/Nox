//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	garbage_collector.cpp
///	@brief	garbage_collector
#include	"stdafx.h"
#include	"garbage_collector.h"

#include	"managed_object.h"

namespace
{
	
}

void	nox::GarbageCollector::Register(nox::ManagedObject& managed_object)
{
	NOX_LOCAL_SCOPE(nox::os::Mutex, mutex_);
	managed_objects_.emplace_back(managed_object);
}

void	nox::GarbageCollector::FrameGC()
{
	if (destroy_objects_.size() > 0)
	{
		for (nox::ManagedObject& managed_object : destroy_objects_)
		{
			managed_object.ReleaseRef();
		}
		destroy_objects_.clear();
	}
	
	const auto result = std::ranges::remove_if(managed_objects_, +[](const nox::ManagedObject& managed_object)noexcept 
		{
			return managed_object.GetRefCount() < 0;
		});
	
	destroy_objects_.insert(destroy_objects_.end(), result.begin(), result.end());
	managed_objects_.erase(result.begin(), result.end());
}