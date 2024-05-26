//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	thread_base.cpp
///	@brief	thread_base
#include	"stdafx.h"
#include	"thread_base.h"

using namespace nox;
using namespace nox::os;

void	ThreadBase::SetThreadName(std::u16string_view name)noexcept
{
	thread_name_ = { u'\0' };

	std::ranges::copy(name, thread_name_.data());
}