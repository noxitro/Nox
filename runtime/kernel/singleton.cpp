//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	singleton.cpp
///	@brief	singleton
#include	"stdafx.h"
#include	"singleton.h"

#include	"assertion.h"
#include	"convert_string.h"
#include	"string_format.h"

void	nox::detail::CheckSingletonCreateInstance(void* instance_ptr, std::string_view type_name)noexcept(false)
{
	NOX_ASSERT(instance_ptr == nullptr, util::Format(U"インスタンスを生成済みです:{0}", type_name.data()));
}

void	nox::detail::CheckSingletonDeleteInstance(void* instance_ptr, std::string_view type_name)noexcept(false)
{
	NOX_ASSERT(instance_ptr != nullptr, util::Format(U"インスタンスを破棄済みです:{0}", type_name.data()));
}

