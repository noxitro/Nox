///	@file	class_info.cpp
///	@brief	class_info
#include	"stdafx.h"
#include	"class_info.h"

#include	"function_info.h"
#include	"field_info.h"
using namespace nox;
using namespace nox::reflection;

const FieldInfo* ClassInfo::GetField(std::uint8_t typeID)const noexcept
{
	return util::SafeDeref(util::FindIf(field_ptr_table_, field_ptr_table_ + field_length_, [typeID](const FieldInfo* x)noexcept {return x->GetTypeID() == typeID; }));
}