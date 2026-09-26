//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	stream_writer.h
///	@brief	stream_writer
#pragma once
#include	"stream.h"

namespace nox::io
{
	class StreamWriter : public nox::io::Stream
	{
	public:
		inline constexpr StreamWriter()noexcept {}
		inline constexpr virtual ~StreamWriter()noexcept override{}
		virtual void Write(const std::span<const std::byte> buffer) = 0;

	private:
		
	};
}