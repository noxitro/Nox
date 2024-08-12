//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	nox_string_view.cpp
///	@brief	nox_string_view
#include	"stdafx.h"
#include	"nox_string_view.h"
#include	"nox_string.h"

nox::StringView::StringView(const String& s)noexcept:
	view_(s.CStr())
{

}