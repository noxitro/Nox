// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

///	@file	function.cpp
///	@brief	function
#include	"pch.h"
#include	"function_info.h"

#include	"class_info.h"
#include	"database.h"
#include	"reflection_object.h"

const class nox::reflection::ReflectionObject* nox::reflection::FunctionInfo::GetAttribute(const nox::reflection::Type& type)const noexcept
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