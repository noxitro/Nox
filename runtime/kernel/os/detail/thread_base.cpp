//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	thread_base.cpp
///	@brief	thread_base
#include	"pch.h"
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