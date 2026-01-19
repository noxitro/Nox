//	Copyright (c) 2026 NOX ENGINE All rights reserved.

///	@file	socket_stream_reader.cpp
///	@brief	socket_stream_reader
#include	"stdafx.h"
#include	"socket_stream_reader.h"

#include	"editor_ipc_server.h"

nox::uint64 nox::dev::editor_ipc::SocketStreamReader::ReadLength()
{
	// LEB128（unsigned）をデコードして uint64 を返す。
// 注意: 呼び出し側は事前に十分なバイトがバッファにあることを保証するか、
// 	   // バッファに少なくとも1バイトあることを確認
	NOX_ASSERT(GetReceivedSize() > 0, u"SocketStreamReaderの受信バッファが不足しています (ReadLength)");
//       ここでアサートが発生することを許容すること。
	nox::uint64 value = 0;
	nox::uint32 shift = 0;

	// LEB128 for 64bit は最大 10 バイト
	for (nox::int32 i = 0; i < 10; ++i)
	{
		// 現在の読み出し位置からバイトを取得（リングバッファのラップを考慮）
		const nox::uint32 idx = read_pos_ & (k_buffer_size - 1);
		const nox::uint8 byte = buffer_[idx];

		value |= static_cast<nox::uint64>(byte & 0x7Fu) << shift;

		// 読み出し位置を進める（モジュロ）
		read_pos_ = (read_pos_ + 1) & (k_buffer_size - 1);

		// MSB が 0 なら終端
		if ((byte & 0x80u) == 0)
		{
			return value;
		}

		shift += 7;
	}

	// ここに到達するのはフォーマット異常（過剰なバイト）
	NOX_ASSERT(false, u"LEB128 デコードエラー: 長すぎるエンコーディング");
	return 0;
}

void nox::dev::editor_ipc::SocketStreamReader::AddReceiveBuffer(std::span<const nox::uint8> buffer)
{
	//	readとAddReceiveBufferはマルチスレッドで呼ばれるので、書き込み順に気を付ける
	//	排他制御はしない方針

	const nox::uint32 write_size = static_cast<nox::uint32>(buffer.size());

	//	書き込み可能サイズ
	const nox::uint32 free_size = k_buffer_size - GetReceivedSize();

	//	バッファオーバーチェック
	NOX_ASSERT(write_size <= free_size, u"SocketStreamReaderの受信バッファがオーバーフローしました");

	//	先に書き込み、そのあとにrecv_pos_を更新する
	const nox::uint32 received_pos = (recv_pos_ + write_size) & (k_buffer_size - 1);

	if (recv_pos_ + write_size <= k_buffer_size)
	{
		//	一回のコピーで済む場合
		nox::memory::Copy(buffer_.data() + recv_pos_, buffer.data(), write_size);
	}
	else
	{
		//	分割してコピーする場合
		const nox::uint32 first_copy_size = k_buffer_size - recv_pos_;
		nox::memory::Copy(buffer_.data() + recv_pos_, buffer.data(), first_copy_size);
		const nox::uint32 second_copy_size = write_size - first_copy_size;
		nox::memory::Copy(buffer_.data(), buffer.data() + first_copy_size, second_copy_size);
	}

	recv_pos_ = received_pos;
}

void	nox::dev::editor_ipc::SocketStreamReader::Read(std::span<nox::uint8> dest)
{
	const nox::uint32 need = static_cast<nox::uint32>(dest.size());
	//	バッファチェック
	NOX_ASSERT(need <= GetReceivedSize(), u"SocketStreamReaderの受信バッファが不足しています");

	const nox::uint32 recv_pos = recv_pos_;

	if (read_pos_ + need <= k_buffer_size)
	{
		//	一回のコピーで済む場合
		nox::memory::Copy(dest.data(), buffer_.data() + read_pos_, need);
	}
	else
	{
		//	分割してコピーする場合
		const nox::uint32 first_copy_size = k_buffer_size - read_pos_;
		nox::memory::Copy(dest.data(), buffer_.data() + read_pos_, first_copy_size);
		const nox::uint32 second_copy_size = need - first_copy_size;
		nox::memory::Copy(dest.data() + first_copy_size, buffer_.data(), second_copy_size);
	}

	read_pos_ = (read_pos_ + need) & (k_buffer_size - 1);
}

std::u8string_view nox::dev::editor_ipc::SocketStreamReader::ReadString(std::span<nox::char8> dest)
{
	const nox::uint64 length = ReadLength();

	NOX_ASSERT(length <= static_cast<nox::uint64>(dest.size()), u"SocketStreamReader::ReadString: バッファサイズオーバー");

	this->Read(std::span<nox::uint8>(reinterpret_cast<nox::uint8*>(dest.data()), static_cast<nox::uint32>(length)));
	return std::u8string_view(dest.data(), static_cast<size_t>(length));
}

nox::U8String nox::dev::editor_ipc::SocketStreamReader::ReadString()
{
	const nox::uint64 length = ReadLength();
	nox::U8String result(static_cast<size_t>(length), u'0');
	this->Read(std::span<nox::uint8>(reinterpret_cast<nox::uint8*>(result.data()), static_cast<nox::uint32>(length)));
	return result;
}