//	Copyright (c) 2026 NOX ENGINE All rights reserved.

///	@file	stream_reader.cpp
///	@brief	stream_reader
#include	"pch.h"
#include	"stream_reader.h"

#include	"../os/file.h"
#include	"../assertion.h"

nox::io::FileStreamReader::FileStreamReader(std::u8string_view path) :
	ifs_(reinterpret_cast<const char*>(path.data()), std::ios::binary)
{
	NOX_ASSERT(ifs_.is_open(), u"failed to open file: {0}", path);
}

void nox::io::FileStreamReader::Read(std::span<std::byte> dest)
{
	ifs_.read(reinterpret_cast<nox::char8*>(dest.data()), static_cast<std::streamsize>(dest.size()));
}

void nox::io::SpanStreamReader::Read(std::span<std::byte> dest)
{
	NOX_ASSERT(position_ <= buffer_.size(), u"position_ is out of buffer range");
	NOX_ASSERT(dest.size() <= buffer_.size() - position_, u"dest size is too large for remaining buffer");
	std::copy_n(buffer_.begin() + position_, dest.size(), dest.begin());
	position_ += static_cast<nox::uint32>(dest.size());
}

nox::io::FileSpanStreamReader::FileSpanStreamReader(const std::u8string_view path)
{
	NOX_ASSERT(file_.Open(path, u8"rb"), u"failed to open file: {0}", path);
}

void nox::io::FileSpanStreamReader::Read(std::span<std::byte> dest)
{
	NOX_ASSERT(file_.IsOpen(), u"file is not open");
	auto read_span = file_.Read(dest);
	NOX_ASSERT(read_span.size() == dest.size(), u"failed to read the requested number of bytes");
}
