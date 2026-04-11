///	@file	intrusive_ptr.cpp
///	@brief	intrusive_ptr
#include	"pch.h"
#include	"intrusive_ptr.h"

#include	"assertion.h"
void nox::detail::IntrusivePtrAbort()
{
	NOX_ASSERT(false, u"ここには来ないはず");
}