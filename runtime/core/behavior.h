//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	behavior.h
///	@brief	behavior
#pragma once
#include	"component.h"

namespace nox
{
	class Behavior : public nox::Component
	{
		NOX_DECLARE_MANAGED_OBJECT(Behavior, Component);
	private:
		enum class FunctionType : uint8
		{
			NONE = 0,
			Awake = 1 << 0,
			Start = 1 << 1,
			Update = 1 << 2,
			LateUpdate = 1 << 3,
			Destroy = 1 << 4,

		};
	public:
		Behavior();
		~Behavior()override;

		virtual void	Awake() {}
		virtual void	Start() {}
		virtual void	Update() {}
		virtual void	LateUpdate() {}
		virtual void	Destroy() {}

	protected:
		void	Loaded()override final;
		void	UnLoaded()override final;

	private:
		FunctionType enabled_function_types_;
	};
}