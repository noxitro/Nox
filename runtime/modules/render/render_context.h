//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	render_context.h
///	@brief	render_context
#pragma once
#include	"render_command_type.h"

namespace nox::render
{
	namespace commands
	{
		struct CommandBase;
	}

	class RenderContext
	{
	public:
		RenderContext();
		~RenderContext();

		nox::render::commands::CommandBase& AllocCommand(nox::render::commands::CommandType type, nox::uint32 size);

	private:
		nox::uint8* command_buffer_;
		nox::uint32 command_buffer_size_;

		nox::uint32 write_offset_;
	};
}