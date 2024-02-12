//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	managed_object.cpp
///	@brief	managed_object
#include	"stdafx.h"
#include	"managed_object.h"

using namespace nox;

ManagedObject::ManagedObject():ref_count_(1)
{

}

ManagedObject::~ManagedObject()
{

}

void	ManagedObject::AddRef()
{
	os::atomic::Increment(ref_count_);
}

void	ManagedObject::ReleaseRef()
{
	os::atomic::Decrement(ref_count_);
}