//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	behavior.cpp
///	@brief	behavior
#include	"pch.h"
#include	"behavior.h"

#include	"behavior_manager.h"

namespace nox
{

}

nox::Behavior::Behavior():
	enabled_function_types_(FunctionType::NONE)
{

}

nox::Behavior::~Behavior()
{
}

void	nox::Behavior::Loaded()
{
	nox::BehaviorManager::Instance().Register(*this);

	const nox::reflection::ClassInfo*const class_info = nox::reflection::FindClassInfo(GetType());
	if (class_info != nullptr)
	{
		constexpr const nox::FunctionPointerId& awake_id = nox::GetFunctionPointerId<&nox::Behavior::Awake>();
		constexpr const nox::FunctionPointerId& start_id = nox::GetFunctionPointerId<&nox::Behavior::Start>();
		constexpr const nox::FunctionPointerId& update_id = nox::GetFunctionPointerId<&nox::Behavior::Update>();
		constexpr const nox::FunctionPointerId& late_update_id = nox::GetFunctionPointerId<&nox::Behavior::LateUpdate>();
		constexpr const nox::FunctionPointerId& destroy_id = nox::GetFunctionPointerId<&nox::Behavior::Destroy>();

		for (const nox::reflection::FunctionInfo& function_info : class_info->GetFunctionList())
		{
			if (function_info.GetFunctionId() == awake_id)
			{
				enabled_function_types_ = nox::util::BitOr(enabled_function_types_, FunctionType::Awake);
			}
			else if (function_info.GetFunctionId() == start_id)
			{
				enabled_function_types_ = nox::util::BitOr(enabled_function_types_, FunctionType::Start);
			}
			else if (function_info.GetFunctionId() == update_id)
			{
				enabled_function_types_ = nox::util::BitOr(enabled_function_types_, FunctionType::Update);
			}
			else if (function_info.GetFunctionId() == late_update_id)
			{
				enabled_function_types_ = nox::util::BitOr(enabled_function_types_, FunctionType::LateUpdate);
			}
			else if (function_info.GetFunctionId() == destroy_id)
			{
				enabled_function_types_ = nox::util::BitOr(enabled_function_types_, FunctionType::Destroy);
			}
		}
	}
}

void	nox::Behavior::UnLoaded()
{

}