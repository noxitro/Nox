// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	delegate_handler.cpp
/// @brief	delegate_handler
#include "pch.h"
#include "delegate_handler.h"

#include "assertion.h"

nox::DelegateHandle::~DelegateHandle()
{
	NOX_ASSERT(owner == nullptr, u"DelegateHandle is not disposed");
}

void nox::DelegateHandle::Dispose()
{
	if (owner != nullptr)
	{
	//	owner->Remove(*this);
		owner = nullptr;
	}
}