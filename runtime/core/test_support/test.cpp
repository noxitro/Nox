//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	test.cpp
///	@brief	test
#include	"pch.h"
#include	"test.h"

#include	"test_reflection.h"

void nox::test::Test()
{
	nox::test::TestReflection();
	nox::test::TestDelegate();
	nox::test::TestEntityCommandBuffer();
	nox::test::TestEntityEcs();
	nox::test::TestJobSystem();
}