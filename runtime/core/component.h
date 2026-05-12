//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	component.h
///	@brief	component
#pragma once

#include	"managed_object.h"

namespace nox
{
	class EntityNode;
	class Component : public nox::ManagedObject
	{
		NOX_DECLARE_MANAGED_OBJECT(Component, nox::ManagedObject);
	public:
		inline class nox::EntityNode& GetEntityNode()noexcept { return nox::util::Deref(owner_); }
		inline const class nox::EntityNode& GetEntityNode()const noexcept { return nox::util::Deref(owner_); }

		void	SetOwner(nox::EntityNode& owner)noexcept;
		virtual	void	Loaded() { return; }
		virtual void	UnLoaded() { return; }

		/// @brief 有効かどうか
		inline bool IsValid()const noexcept { return owner_ != nullptr; }
	protected:
		inline constexpr Component()noexcept :
			owner_(nullptr)
		{}

	private:
		nox::EntityNode* owner_;
	};
}