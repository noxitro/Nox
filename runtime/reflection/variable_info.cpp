///	@file	field_info.cpp
///	@brief	field_info
#include	"stdafx.h"
#include	"variable_info.h"

#include	"class_info.h"
#include	"database.h"
#include	"reflection_object.h"

const nox::reflection::ClassInfo* nox::reflection::VariableInfo::GetContainingUserDefinedCompoundTypeInfo()const noexcept
{
	if (containing_type_ == nox::reflection::GetInvalidType())
	{
		return nullptr;
	}

	return nox::reflection::FindClassInfo(containing_type_);
}

const nox::reflection::ReflectionObject* nox::reflection::VariableInfo::GetAttribute(const nox::reflection::Type& type)const noexcept
{
	if (attribute_list_ == nullptr || attribute_list_length_ == 0)
	{
		return nullptr;
	}

	for (std::uint8_t i = 0; i < attribute_list_length_; ++i)
	{
		const nox::reflection::ReflectionObject& attribute = attribute_list_[i];
		if (attribute.GetType() == type)
		{
			return &attribute;
		}
	}

	return nullptr;
}