//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	reflection_system.cpp
///	@brief	reflection_system
#include	"stdafx.h"
#include	"reflection_system.h"

#include	"manager.h"

using namespace nox;

void reflection::Initialize()
{
	reflection::Reflection::CreateInstance();
}

void reflection::Finalize()
{
	reflection::Reflection::DeleteInstance();
}