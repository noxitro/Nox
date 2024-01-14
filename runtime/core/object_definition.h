//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	object_definition.h
///	@brief	object_definition
#pragma once

namespace nox
{
	class ManagedObject;
}

///@brief	基底オブジェクトの定義
#define	NOX_DECLARE_OBJECT_ROOT(ClassType) \
	NOX_DECLARE_REFLECTION_OBJECT(ClassType)
//	end define

#define	NOX_DECLARE_OBJECT(ClassType, BaseType) \
	NOX_DECLARE_REFLECTION_OBJECT(ClassType); \
	private:\
		NOX_ATTR(::nox::reflection::attr::IgnoreReflection())\
		static inline consteval void	StaticAssertNoxDeclareObject()noexcept\
		{\
			static_assert(!std::is_same_v<BaseType, ClassType>, "base type failed");\
			static_assert(std::is_base_of_v<BaseType, ClassType>, "base type failed");\
			static_assert(!std::is_base_of_v<::nox::ManagedObject, ClassType>, "base type failed");\
		}\
	public:\
		using Base = BaseType
//	end define

#define	NOX_DECLARE_MANAGED_OBJECT(ClassType, BaseType)\
	private:\
		NOX_ATTR(::nox::reflection::attr::IgnoreReflection())\
		static	inline	consteval	void	StaticAssertNoxDeclareManagedObject()noexcept\
		{\
			static_assert(!std::is_same_v<BaseType, ClassType>, "base type failed");\
			static_assert(std::is_base_of_v<class ::nox::ManagedObject, ClassType>, "managed object failed");\
		}\
		NOX_DECLARE_REFLECTION_OBJECT(ClassType); \
	public:\
		using Base = BaseType
//	end define
