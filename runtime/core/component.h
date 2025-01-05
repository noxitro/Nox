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
		
	public:
		inline class GameObject& GameObject()noexcept { return nox::util::Deref(owner_); }
		inline const class GameObject& GameObject()const noexcept { return nox::util::Deref(owner_); }

		void	SetOwner(class GameObject& owner)noexcept;
		virtual	void	Loaded() { return; }
		virtual void	UnLoaded() { return; }

		inline	void	SetComponentChain(Component*const chain)noexcept { chain_ = chain; }
		inline	Component* GetComponentChain()const noexcept { return chain_; }
	protected:
		inline	Component()noexcept :
			owner_(nullptr),
			chain_(nullptr)
		{}

	private:
		class GameObject* owner_;
		Component* chain_;
	};


}