//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	socket_stream_writer.cpp
///	@brief	socket_stream_writer
#include	"pch.h"
#if NOX_DEVELOP
#include	"socket_stream_writer.h"

#include	"editor_remote_server.h"
#include	"socket_stream_utility.h"
#include	"../object.h"

namespace nox::dev::editor_remote
{
	namespace
	{
		constexpr nox::uint8 k_header_reserve_size = 10; // bodyのleb128を書き込む最大10バイト必要

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
}

nox::dev::editor_remote::SocketStreamWriter::SocketStreamWriter(nox::dev::editor_remote::EditorRemoteServer& server) noexcept :
	server_(server),
	buffer_{ 0 },
	position_(k_header_reserve_size)
{

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
	const nox::uint32 payload_size = position_ - k_header_reserve_size;
	if (payload_size == 0)	// uint32 は <= 0 にならない
	{
		return;
	}

	const nox::uint32 header_size = WriteLeb128ToEnd(payload_size);
	const nox::uint32 send_start = k_header_reserve_size - header_size;

	// send_start から送信（ヘッダ + ペイロード）
	server_.SendBuffer(std::span(buffer_.data() + send_start, header_size + payload_size));
	Clear();
}

void nox::dev::editor_remote::SocketStreamWriter::Clear()
{
	position_ = k_header_reserve_size;
}

void nox::dev::editor_remote::SocketStreamWriter::Write(nox::IntrusivePtr<nox::Object>& value)
{
	if (value == nullptr)
	{
		Write(0);
		return;
	}

	auto& value_ref = *value.Get();

	const nox::int64 instance_id = server_.FindRemoteInstanceId(value_ref);

	if (instance_id != 0)
	{
		Write(instance_id);
		return;
	}

 //	リモートインスタンスとして登録して、ID/FQN/プロパティバッファを書き込む
   server_.RegisterRemoteInstance(value_ref, instance_id);
	const nox::int64 registered_instance_id = server_.FindRemoteInstanceId(value_ref);
	NOX_ASSERT(registered_instance_id != 0, u"リモートインスタンスIDの登録に失敗しました");
	Write(registered_instance_id);

	//	型情報の取得
	const nox::reflection::ClassInfo& class_info = nox::util::Deref(nox::reflection::FindClassInfo(value->GetType()));
	
	//	FQNを書き込む
	Write(class_info.GetFullName());

	std::array<nox::uint8, 1024> property_buffer{};
	const std::span<nox::uint8> property_bytes = nox::dev::editor_remote::GetPropertiesBytes(property_buffer, value_ref);
	Write(static_cast<nox::int32>(property_bytes.size()));
	WriteBytes(property_bytes);
}

nox::uint32 nox::dev::editor_remote::SocketStreamWriter::WriteLeb128ToEnd(nox::uint64 length)
{
	// 一時バッファにLEB128エンコード
	std::array<nox::uint8, k_header_reserve_size> temp{};
	std::size_t count = 0;

	do
	{
		nox::uint8 byte = static_cast<nox::uint8>(length & 0x7Fu);
		length >>= 7;
		if (length != 0)
		{
			byte |= 0x80u;
		}
		temp[count++] = byte;
	} while (length != 0);

	// 予約領域の右詰め位置へコピー
	const nox::uint32 start = k_header_reserve_size - static_cast<nox::uint32>(count);
	nox::memory::Copy(buffer_.data() + start, temp.data(), static_cast<nox::uint32>(count));

	return static_cast<nox::uint32>(count);
}
#endif // NOX_DEVELOP
