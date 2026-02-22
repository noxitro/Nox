//	Copyright (c) 2026 NOX ENGINE All rights reserved.

///	@file	socket_stream_writer.cpp
///	@brief	socket_stream_writer
#include	"stdafx.h"
#include	"socket_stream_writer.h"

#include	"editor_remote_server.h"

namespace nox::util
{
	inline constexpr void EncodeLEB128(std::span<nox::uint8> dest, std::size_t value)noexcept
	{
		std::size_t index = 0;
		do
		{
			nox::uint8 byte = static_cast<nox::uint8>(value & 0x7Fu);
			value >>= 7;
			if (value != 0)
			{
				byte |= 0x80u; // 続きありフラグ
			}
			dest[index++] = byte;
		} while (value != 0);
	}

	inline constexpr std::size_t DecodeLEB128(const std::span<nox::uint8> data)noexcept
	{
		std::size_t index = 0;
		std::size_t shift = 0;
		std::size_t outValue = 0;
		while (true)
		{
			nox::uint8 byte = data[index++];
			outValue |= static_cast<std::size_t>(byte & 0x7Fu) << shift;
			if ((byte & 0x80u) == 0)
			{
				break; // 続きなし
			}
			shift += 7;
		}
		return index; // 読み取ったバイト数を返す
	}
}


void nox::dev::editor_remote::SocketStreamWriter::WriteLength(nox::uint64 length)
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
		// ここでは Writer が提供するバイナリ書き込み関数を利用する想定:
		// void Write(const void* data, size_t size);
		WriteBytes(std::span(&byte, 1));
	} while (length != 0);
}

void nox::dev::editor_remote::SocketStreamWriter::WriteBytes(std::span<const nox::uint8> data)
{
	//	書き込みバッファはリングバッファではないので、そのまま書き込む
	std::size_t offset = 0;
	const std::size_t total = data.size();

	while (offset < total)
	{
		// バッファの残容量に収まる分だけコピー
		const std::size_t space = static_cast<std::size_t>(k_buffer_size - position_);
		const std::size_t to_copy = math::Min(space, total - offset);
		nox::memory::Copy(buffer_.data() + position_, data.data() + offset, static_cast<nox::uint32>(to_copy));
		position_ += static_cast<nox::uint32>(to_copy);
		offset += to_copy;

		// バッファが満杯なら Flush
		if (position_ == k_buffer_size)
		{
			Flush();
		}
	}
}

void nox::dev::editor_remote::SocketStreamWriter::Flush()
{
	if (position_ <= 0)
	{
		return;
	}
	server_.SendBuffer(std::span(buffer_.data(), position_));
	Clear();
}

void nox::dev::editor_remote::SocketStreamWriter::Clear()
{
	position_ = 0;
}

void nox::dev::editor_remote::SocketStreamWriter::Write(const nox::reflection::ReflectionObject*const value)
{
	if (value == nullptr)
	{
		Write(0);
		return;
	}

	//	型情報の取得
	const nox::reflection::ClassInfo& class_info = nox::util::Deref(nox::reflection::FindClassInfo(value->GetType()));
	
	//	メンバ変数
	for (const nox::reflection::VariableInfo& variable_info : class_info.GetVariableList())
	{
		const nox::reflection::Type& type = variable_info.GetType();
		//switch(type.)
	}

	//	メンバ関数(プロパティ)
}