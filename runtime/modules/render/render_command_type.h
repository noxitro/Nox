//	Copyright (C) 2026 NOX ENGINE All rights reserved.

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