///	@file	field_info.cpp
///	@brief	field_info
#include	"stdafx.h"
#include	"field_info.h"

#include	"class_info.h"
#include	"manager.h"

using namespace nox;
using namespace nox::reflection;

const ClassInfo* FieldInfo::GetOwnerClass(std::uint8_t typeID)const noexcept
{
	return util::Deref(Reflection::Instance()).FindClassInfo(typeID);
}