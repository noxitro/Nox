//	Copyright (C) 2026 NOX ENGINE All rights reserved.

///	@file	socket_stream_reader.h
///	@brief	socket_stream_reader
#pragma once

namespace nox::dev::editor_ipc
{
	class EditorIpcServer;

	class SocketStreamReader
	{
	private:
		/// @brief		バッファサイズ
		/// @details	リングバッファとして使用するバッファのサイズ（2^n必須）
		static constexpr nox::uint32 k_buffer_size = nox::math::Pow(2, 11);
	public:
		inline constexpr explicit SocketStreamReader(nox::dev::editor_ipc::EditorIpcServer& server)noexcept :
			server_(server),
			buffer_{ 0 },
			recv_pos_(0),
			read_pos_(0)
		{
		}

		inline constexpr SocketStreamReader(const SocketStreamReader&) = delete;
		inline constexpr SocketStreamReader(SocketStreamReader&&) noexcept = delete;
		inline constexpr ~SocketStreamReader() noexcept = default;

		nox::uint64 ReadLength();
		void Read(std::span<nox::uint8> dest);

		void AddReceiveBuffer(std::span<const nox::uint8> buffer);

		template<typename T> requires(std::is_arithmetic_v<T>)
		inline T Read()
		{
			T value;
			this->Read(std::span<nox::uint8>(static_cast<nox::uint8*>(&value), sizeof(T)));
			return value;
		}

		std::u8string_view ReadString(std::span<nox::char8> dest);
		nox::U8String ReadString();

		/// @brief 受信済みサイズを取得
		[[nodiscard]] inline constexpr nox::uint32 GetReceivedSize()const noexcept
		{
			return (recv_pos_ - read_pos_) & (k_buffer_size - 1);
		}
	private:
		

	private:
		nox::dev::editor_ipc::EditorIpcServer& server_;
		/// @brief 受信バッファ
		std::array<nox::uint8, k_buffer_size> buffer_;
		/// @brief 受信済み位置
		nox::uint32 recv_pos_;
		/// @brief 読み取り位置
		nox::uint32 read_pos_;
	};
}