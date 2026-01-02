//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	thread_base.cpp
///	@brief	thread_base
#include	"stdafx.h"
#include	"thread_base.h"

#include	"../os.h"

void	nox::os::ThreadBase::SetThreadName(std::u16string_view name)
{
	thread_name_ = { u'\0' };

	std::ranges::copy(name, thread_name_.data());
}

void	nox::os::ThreadBase::Sleep(nox::uint32 milliseccond)
{
	nox::os::Sleep(milliseccond);
}