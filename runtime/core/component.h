//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

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
	struct IComponentTag : public nox::IComponentData
	{
		IComponentTag()noexcept = delete;
	};

}