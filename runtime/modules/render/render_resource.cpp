//	Copyright (c) 2026 NOX ENGINE All rights reserved.

///	@file	render_resource.cpp
///	@brief	render_resource
#include	"pch.h"
#include	"render_resource.h"

void nox::render::RenderResource::AddRef()
{
	nox::os::atomic::Increment(ref_count_);
}

void nox::render::RenderResource::Release()
{
	const nox::int32 ref_count = nox::os::atomic::Decrement(ref_count_);
	if (ref_count == 0)
	{
		delete this;
	}
	else if (ref_count < 0)
	{
		NOX_ASSERT(false, u"RenderResource::Release() ref_count_ < 0");
	}
}