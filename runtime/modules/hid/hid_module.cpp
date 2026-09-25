//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	hid_module.cpp
///	@brief	hid module
#include	"pch.h"
#include	"hid_module.h"
#include	"keyboard_manager.h"

nox::hid::Module::Module() = default;

void nox::hid::Module::CreateEngineSystems(nox::PmrVector<nox::SystemBase*>& out)const
{
	out.emplace_back(new nox::hid::KeyboardManager());
}
