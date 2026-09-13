//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	socket_stream_reader.h
///	@brief	socket_stream_reader
#pragma once

namespace nox
{
	class Object;
}

namespace nox::dev::editor_remote
{
	class EditorRemoteServer;

	class SocketStreamReader
	{
	private:
		/// @brief		バッファサイズ
		/// @details	リングバッファとして使用するバッファのサイズ（2^n必須）
		static constexpr nox::uint32 k_buffer_size = 65536;
	public:
		inline constexpr explicit SocketStreamReader(nox::dev::editor_remote::EditorRemoteServer& server)noexcept :
			server_(server),
			buffer_{ 0 },
			recv_pos_(0),
			read_pos_(0)
		{
		}

		inline constexpr SocketStreamReader(const SocketStreamReader&) = delete;
		inline constexpr SocketStreamReader(SocketStreamReader&&) noexcept = delete;
		inline constexpr ~SocketStreamReader() noexcept = default;

		void AddReceiveBuffer(std::span<const nox::uint8> buffer);
		void ReadBytes(std::span<nox::uint8> dest);

		void Read(std::span<nox::uint8> dest);
		void Read(std::span<nox::char8> dest);
		inline std::u8string_view ReadString(const std::span<nox::char8> dest)
		{
			nox::uint64 length = this->ReadLength();
			NOX_ASSERT(length <= static_cast<nox::uint64>(dest.size()), u"SocketStreamReader::ReadString: バッファサイズオーバー length:{0}, buffer_size:{1}", length, dest.size());
			this->ReadBytes(std::span<nox::uint8>(reinterpret_cast<nox::uint8*>(dest.data()), static_cast<nox::uint32>(length)));
			return std::u8string_view(dest.data(), static_cast<size_t>(length));
		}

		template<typename T> requires(std::is_arithmetic_v<T>)
		inline void Read(T& out)
		{
			this->ReadBytes(std::span<nox::uint8>(reinterpret_cast<nox::uint8*>(&out), sizeof(T)));
		}

		template<nox::concepts::Enum T>
		inline void Read(T& value)
		{
			this->ReadBytes(std::span<nox::uint8>(reinterpret_cast<nox::uint8*>(&value), sizeof(T)));
		}

		void Read(nox::IntrusivePtr<nox::Object>& value);

		template<std::size_t Length>
		inline void Read(nox::BasicFixedString<nox::char8, Length>& value)
		{
			std::array<nox::char8, Length> buffer{};
			value.Assign(this->ReadString(buffer));
		}

		nox::uint64 ReadLength();
		
		nox::StlU8String ReadString();

		/// @brief 受信済みサイズを取得
		[[nodiscard]] inline constexpr nox::uint32 GetReceivedSize()const noexcept
		{
			return (recv_pos_ - read_pos_) & (k_buffer_size - 1);
		}

		/// @brief 次のパケットを読み取ることができるかどうかを判定します。
		/// @return 次のパケットを読み取ることができる場合は true、そうでない場合は false。
		[[nodiscard]] bool CanReadBody()const noexcept;

		/// @brief ヘッダ部分を空読み
		void SkipHeader();
	private:

	private:
		nox::dev::editor_remote::EditorRemoteServer& server_;
		/// @brief 受信バッファ
		std::array<nox::uint8, k_buffer_size> buffer_;
		/// @brief 受信済み位置
		nox::uint32 recv_pos_;
		/// @brief 読み取り位置
		nox::uint32 read_pos_;
	};
}
