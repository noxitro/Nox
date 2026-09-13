//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	render_context.cpp
///	@brief	render_context
#include	"pch.h"
#include	"render_context.h"

namespace nox::render
{
	constexpr nox::uint32 k_command_buffer_size = 256 * 256; // 1MB
}

nox::render::RenderContext::RenderContext() :
	command_buffer_(new nox::uint8[k_command_buffer_size]),
	command_buffer_size_(k_command_buffer_size),
	write_offset_(0U)
{
}

nox::render::RenderContext::~RenderContext()
{
	delete[] command_buffer_;
	command_buffer_ = nullptr;
}

nox::render::commands::CommandBase& nox::render::RenderContext::AllocCommand(nox::render::commands::CommandType type, nox::uint32 size)
{
	nox::render::commands::CommandBase* command = nullptr;
	return *command;
}