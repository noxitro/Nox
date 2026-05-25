//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	component.h
///	@brief	component
#pragma once

#include	"object.h"

namespace nox
{
	/// @brief componentタグ
	struct IComponentData
	{

	};

	template<class T>
	inline consteval bool IsComponentDataType()noexcept
	{
		return 
			std::is_base_of_v<nox::IComponentData, T> &&
			std::is_abstract_v<T> == false &&
			std::is_trivially_copyable_v<T> &&
			std::is_default_constructible_v<T>
			;
	}

	class EntityNode;
	class Component : public nox::Object, nox::IComponentData
	{
		NOX_DECLARE_OBJECT(Component, nox::Object);
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

	template<class T>
	inline consteval bool IsComponentType()noexcept
	{
		return
			std::is_base_of_v<nox::Component, T> &&
			std::is_trivially_copyable_v<T> == false
			;
	}
}