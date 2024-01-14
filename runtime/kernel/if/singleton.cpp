//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	singleton.cpp
///	@brief	singleton
#include	"stdafx.h"
#include	"singleton.h"

#include	"assertion.h"
#include	"convert_string.h"
#include	"string_format.h"

using namespace nox;

void	nox::detail::CheckSingletonCreateInstance(void* instance_ptr, std::string_view type_name)
{
	NOX_ASSERT(instance_ptr == nullptr, util::Format(u"インスタンスを生成済みです:{0}", type_name.data()));
}

void	nox::detail::CheckSingletonDeleteInstance(void* instance_ptr, std::string_view type_name)
{
	NOX_ASSERT(instance_ptr != nullptr, util::Format(u"インスタンスを破棄済みです:{0}", type_name.data()));
}

