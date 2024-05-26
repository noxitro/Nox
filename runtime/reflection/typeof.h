//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	typeof.h
///	@brief	typeof
#pragma once
#include	"reflection_object.h"
#include	"manager.h"

namespace nox::reflection
{
	template<concepts::ClassUnion T>
	inline const class ClassInfo* TypeofClass()
	{
		return Reflection::Instance().FindClassInfo<T>();
	}

	inline const class ClassInfo* TypeofClass(const ReflectionObject& object)
	{
		return Reflection::Instance().FindClassInfo(object.GetUniqueTypeID());
	}

	template<concepts::Enum T>
	inline const class EnumInfo* TypeofEnum()
	{
		return Reflection::Instance().FindEnumInfo<T>();
	}


}