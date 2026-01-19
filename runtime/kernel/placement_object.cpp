//	Copyright (c) 2026 NOX ENGINE All rights reserved.

///	@file	placement_object.cpp
///	@brief	placement_object
#include	"stdafx.h"
#include	"placement_object.h"

#include	"assertion.h"

void	nox::detail::PlacementObjectAbort()noexcept
{
	NOX_ASSERT(false, u"ここには来ないはず");
}