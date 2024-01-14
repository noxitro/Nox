//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	gen.cpp
///	@brief	gen
#include	"stdafx.h"
#include	"gen.h"

using namespace nox;

void	nox::reflection::Initialize()
{
	nox::reflection::Reflection::CreateInstance();
}

void	nox::reflection::Finalize()
{
	nox::reflection::Reflection::DeleteInstance();
}