//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	dev_components.h
///	@brief	dev_components
#pragma once
#include	"component.h"

#if NOX_DEVELOP
namespace nox::dev::components
{
	struct Name final : public nox::Component<Name>
	{

	};
}
#endif // NOX_DEVELOP
