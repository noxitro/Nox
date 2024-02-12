//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	behavior.h
///	@brief	behavior
#pragma once
#include	"component.h"

namespace nox
{
	class Behavior : public Component
	{
		NOX_DECLARE_MANAGED_OBJECT(Behavior, Component);
	private:
		enum class FunctionType : uint8
		{
			Awake,
			Start,
			Update,
			LateUpdate,
			Destory,

		};
	public:


	private:
	};
}