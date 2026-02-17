//	Copyright (C) 2026 NOX ENGINE All rights reserved.

///	@file	stream_reader.h
///	@brief	stream_reader
#pragma once
#include	"stream.h"

namespace nox::io
{
	class StreamReader : public nox::io::Stream
	{
	public:
		inline constexpr StreamReader()noexcept {}
		inline constexpr virtual ~StreamReader()noexcept override {}

		virtual void Read(std::span<nox::uint8> dest) = 0;
	};
}