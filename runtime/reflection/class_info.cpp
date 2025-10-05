///	@file	user_defined_compound_type_info.cpp
///	@brief	user_defined_compound_type_info
#include	"stdafx.h"
#include	"class_info.h"

#include	"function_info.h"
#include	"variable_info.h"

#include	"database.h"
bool	nox::reflection::ClassInfo::IsBaseOf(const nox::reflection::ClassInfo& derived)const noexcept
{
	return nox::reflection::IsBaseOf(*this, derived);
}

bool	nox::reflection::ClassInfo::IsSubclassOf(const nox::reflection::Type& base)const noexcept
{
	for (const nox::reflection::Type& type : GetBaseTypeList())
	{
		if (type == base)
		{
			return true;
		}
	}
	return false;
}

const nox::reflection::FunctionInfo* nox::reflection::ClassInfo::GetConstructor(std::span<const std::reference_wrapper<const nox::reflection::Type>> typeList)const noexcept
{
	const nox::uint8 numType = static_cast<nox::uint8>(typeList.size());

	for (const nox::reflection::FunctionInfo& function_info : GetFunctionList())
	{
		if (function_info.IsConstructor() == false)
		{
			continue;
		}
		
		const auto paramTypeList = function_info.GetFunctionParamList();

		//	引数の数が超えている場合はスキップ
		
		if (paramTypeList.size() < numType)
		{
			continue;
		}

		//	非デフォルト引数の数よりも、少ない場合はスキップ
		nox::uint8 nonDefaultParamNum = function_info.GetNonDefaultParamLength();
		if (numType < nonDefaultParamNum)
		{
			continue;
		}
		
		//	型チェック
		for (nox::uint8 i = 0; i < numType; ++i)
		{
			const nox::reflection::FunctionArgumentInfo& argInfo = paramTypeList[i];
			const nox::reflection::Type& type = typeList[i];

			if (type.IsConvertible(argInfo.GetUnderlyingType()) == false)
			{
				continue;
			}
		}
		
		return &function_info;
	}

	return nullptr;
}

const nox::reflection::FunctionInfo* nox::reflection::ClassInfo::GetCopyConstructor()const noexcept
{
	for (const nox::reflection::FunctionInfo& function_info : GetFunctionList())
	{
		if (function_info.IsCopyConstructor() == true)
		{
			return &function_info;
		}
	}

	return nullptr;
}

const nox::reflection::FunctionInfo* nox::reflection::ClassInfo::GetMoveConstructor()const noexcept
{
	for (const nox::reflection::FunctionInfo& function_info : GetFunctionList())
	{
		if (function_info.IsMoveConstructor() == true)
		{
			return &function_info;
		}
	}
	
	return nullptr;
}