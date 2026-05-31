//	Copyright (C) 2026 NOX ENGINE All rights reserved.

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