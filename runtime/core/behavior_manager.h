//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	behavior_manager.h
///	@brief	behavior_manager
#pragma once
#include	"object.h"

namespace nox
{
	class BehaviorManager : public nox::Object
	{
		NOX_DECLARE_OBJECT(nox::BehaviorManager, nox::Object);
	private:
	public:

		void	Initialize();
		void	Update();
		void	LateUpdate();
		void	Finalize();
	private:
	};
}