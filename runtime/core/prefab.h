//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	prefab.h
///	@brief	prefab
#pragma once
#include	"asset.h"
#include	"asset_attribute.h"

namespace nox
{
	class NOX_ATTR(nox::attr::Asset(u8"pfb", 1))
		PrefabAsset final : public nox::Asset
	{
		NOX_DECLARE_OBJECT(PrefabAsset, nox::Asset);
	public:
		void Instantiate() {}
	};
}