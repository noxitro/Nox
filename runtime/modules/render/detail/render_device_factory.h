//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	render_device_factory.h
///	@brief	ビルド構成で選んだバックエンドの RenderDevice を作る
#pragma once

namespace nox::render
{
	class RenderDevice;
}

namespace nox::render::detail
{
	///	@brief	有効なバックエンドの RenderDevice を作る。所有権は呼び出し側に渡る (delete で破棄する)
	///	@return	有効なバックエンドが無い構成では nullptr
	nox::render::RenderDevice* CreateRenderDevice();
}
