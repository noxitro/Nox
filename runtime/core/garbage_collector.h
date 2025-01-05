//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	garbage_collector.h
///	@brief	garbage_collector
#pragma once
#include	"object.h"

namespace nox
{
	class GarbageCollector : public nox::Object, public nox::ISingleton<nox::GarbageCollector>
	{
		NOX_DECLARE_OBJECT(nox::GarbageCollector, nox::Object);
	public:
		void	Register(class nox::ManagedObject& managed_object);

		void FrameGC();
	private:

	private:
		nox::os::Mutex mutex_;
		nox::Vector<std::reference_wrapper<class nox::ManagedObject>> destroy_objects_;
		nox::Vector<std::reference_wrapper<class nox::ManagedObject>> managed_objects_;
	};
}