//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	test_behavior.h
///	@brief	test_behavior
#pragma once
#include	"../behavior.h"

namespace nox::test
{
	class TestBehavior : public nox::Behavior
	{
		NOX_DECLARE_OBJECT(nox::test::TestBehavior, nox::Behavior);
	public:

		void Awake()override;
	};
}