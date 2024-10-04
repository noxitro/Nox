//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	reflection_system.cpp
///	@brief	reflection_system
#include	"stdafx.h"
#include	"reflection_system.h"

#include	"manager.h"

void nox::reflection::Initialize()
{
	reflection::Reflection::CreateInstance();
}

void nox::reflection::Finalize()
{
	reflection::Reflection::Instance().Finalize();
	reflection::Reflection::DeleteInstance();
}