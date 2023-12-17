//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	component.h
///	@brief	component
#pragma once

#include	"managed_object.h"

namespace nox
{
	class Component : public ManagedObject
	{
		NOX_DECLARE_MANAGED_OBJECT(Component, ManagedObject);
	public:
	protected:
		inline	constexpr	Component()noexcept :
			owner_(nullptr) {}

	private:
		class GameObject* owner_;
	};
}