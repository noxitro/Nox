//	Copyright (c) 2026 NOX ENGINE All rights reserved.

///	@file	binary_reader.cpp
///	@brief	binary_reader
#include	"pch.h"
#include	"binary_reader.h"

#include	"stream_reader.h"

void nox::io::BinaryReader::ReadBytes(std::span<std::byte> dest)const
{
	stream_.Read(dest);
}

nox::uint64 nox::io::BinaryReader::ReadLength()const
{
	nox::uint64 result = 0;
	nox::uint32 shift = 0;
	while (true)
	{
		std::byte byte = std::byte{0};
		ReadBytes(std::span(&byte, 1));
		result |= (static_cast<nox::uint64>(byte & std::byte{0x7Fu}) << shift);
		if ((byte & std::byte{0x80u}) == std::byte{0})
		{
			break;
		}
		shift += 7;
	}
	return result;
}
