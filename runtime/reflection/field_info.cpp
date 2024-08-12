///	@file	field_info.cpp
///	@brief	field_info
#include	"stdafx.h"
#include	"field_info.h"

#include	"class_info.h"
#include	"manager.h"

using namespace nox;
using namespace nox::reflection;

const ClassInfo* VariableInfo::GetOwnerClass()const noexcept
{
	return Reflection::Instance().FindClassInfo(owner_class_type_.GetTypeID());
}