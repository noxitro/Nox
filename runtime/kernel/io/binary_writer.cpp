//	Copyright (c) 2026 NOX ENGINE All rights reserved.

///	@file	binary_writer.cpp
///	@brief	binary_writer
#include	"pch.h"
#include	"binary_writer.h"
#include	"stream_writer.h"

void	nox::io::BinaryWriter::WriteByte(const std::span<const nox::uint8> buffer)const
{
	stream_.Write(buffer);
}

void	nox::io::BinaryWriter::WriteLength(nox::uint64 length)const
{
	//	LEB128 (unsigned)
//	https://ja.wikipedia.org/wiki/LEB128
//	最下位 7 ビットずつを書き、続きがある場合は MSB を 1 にする。
//	length == 0 の場合でも 0x00 を一バイト書き出す。
//	uint64 の最大値でも最大 10 バイトで収まる。
	do
	{
		nox::uint8 byte = static_cast<nox::uint8>(length & 0x7Fu);
		length >>= 7;
		if (length != 0)
		{
			byte |= 0x80u; // 続きありフラグ
		}
		WriteByte(std::span(&byte, 1));
	} while (length != 0);
}