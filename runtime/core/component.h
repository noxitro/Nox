//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	component.h
///	@brief	component
#pragma once

#include	"managed_object.h"

namespace nox
{
	class GameObject;
	class Component : public nox::ManagedObject
	{
		NOX_DECLARE_MANAGED_OBJECT(Component, nox::ManagedObject);

	public:
		
	public:
		inline class nox::GameObject& GameObject()noexcept { return nox::util::Deref(owner_); }
		inline const class nox::GameObject& GameObject()const noexcept { return nox::util::Deref(owner_); }

		void	SetOwner(class nox::GameObject& owner)noexcept;
		virtual	void	Loaded() { return; }
		virtual void	UnLoaded() { return; }

		/// @brief 有効かどうか
		inline bool IsValid()const noexcept { return owner_ != nullptr; }
	protected:
		inline	Component()noexcept :
			owner_(nullptr)
		{}

	private:
		class nox::GameObject* owner_;
	};
}