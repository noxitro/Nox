//	Copyright (c) 2025 NOX ENGINE All rights reserved.

///	@file	stream.cpp
///	@brief	stream
#include	"stdafx.h"
#include	"stream.h"

#include	"../assertion.h"
#include	"../unicode_converter.h"
#include	"../algorithm.h"

void nox::io::OutputStream::Write(const std::span<nox::uint8> buffer)
{
	NOX_ASSERT(false, u"Not Implemented");
}

void nox::io::OutputStream::WriteLength(nox::uint64 length)
{
	//	LEB128 (unsigned)
	//	https://ja.wikipedia.org/wiki/LEB128
	//	最下位 7 ビットずつを書き、続きがある場合は MSB を 1 にする。
	//	length == 0 の場合でも 0x00 を一バイト書き出す。
	do
	{
		nox::uint8 byte = static_cast<nox::uint8>(length & 0x7Fu);
		length >>= 7;
		if (length != 0)
		{
			byte |= 0x80u; // 続きありフラグ
		}
		Write(std::span(&byte, 1));
	} while (length != 0);
}