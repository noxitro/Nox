//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	render_command_type.h
///	@brief	render_command_type
#pragma once

namespace nox::render::commands
{
	enum class CommandType : nox::uint8
	{
		Unknown,
		CopyBuffer,
		CopyTexture,
		ClearRenderTarget,
		ClearDepthStencil,
		SetRenderTargets,
		SetViewport,
		SetScissorRect,
		Draw,
		DrawIndexed,
		Dispatch,
		BeginMarker,
		EndMarker,
		_Max
	};
}