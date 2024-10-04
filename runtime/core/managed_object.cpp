//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	managed_object.cpp
///	@brief	managed_object
#include	"stdafx.h"
#include	"managed_object.h"

nox::ManagedObject::ManagedObject():ref_count_(1)
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
	nox::os::atomic::Decrement(ref_count_);
}

