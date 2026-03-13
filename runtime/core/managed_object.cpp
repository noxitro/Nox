//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	managed_object.cpp
///	@brief	managed_object
#include	"stdafx.h"
#include	"managed_object.h"

#include	"garbage_collector.h"
#include	"log_id.h"
namespace
{
	
}

nox::ManagedObject::ManagedObject()noexcept:ref_count_(0)
{
	
}

nox::ManagedObject::~ManagedObject()
{

}

void	nox::ManagedObject::AddRef()
{
	nox::os::atomic::Increment(ref_count_);
}

void	nox::ManagedObject::ReleaseRef()
{
	const auto ref_count = nox::os::atomic::Decrement(ref_count_);
	NOX_ASSERT(ref_count >= -2, u"");

	switch (ref_count)
	{
	case -1:
		nox::GarbageCollector::Instance().Register(*this);
		break;

	case -2:
		NOX_INFO_LINE(nox::log_id::CoreCommon, u8"delete this:{0}", GetType().GetTypeName());
		delete this;
		break;
	}
}

