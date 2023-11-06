///	@file	convert_string.cpp
///	@brief	convert_string
#include	"stdafx.h"
#include	"convert_string.h"

#include	"../memory/stl_allocate_adapter.h"
using namespace nox;

nox::WString	nox::util::ConvertWString(const c16* const from)
{
	return nox::WString(CharCast<const w16*>(from));
}