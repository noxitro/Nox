//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	atomic.h
///	@brief	atomic
#pragma once
#include	"../basic_definition.h"

#if NOX_WIN64
#include	"detail/atomic_win64.h"
#else
static_assert(false);
#endif // NOX_WIN64
