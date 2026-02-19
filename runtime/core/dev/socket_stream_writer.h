//	Copyright (C) 2026 NOX ENGINE All rights reserved.

///	@file	socket_stream_writer.h
///	@brief	socket_stream_writer
#pragma once

namespace nox::dev::editor_remote
{
	class EditorRemoteServer;

	class SocketStreamWriter 
	{
	private:
		static constexpr nox::uint32 k_buffer_size = 5096;
	public:
		inline constexpr explicit SocketStreamWriter(nox::dev::editor_remote::EditorRemoteServer& server) noexcept :
			server_(server),
			buffer_{ 0 },
			position_(0)
		{
		}

		inline constexpr SocketStreamWriter(const SocketStreamWriter&) = delete;
		inline constexpr SocketStreamWriter(SocketStreamWriter&&) noexcept = delete;
		inline constexpr ~SocketStreamWriter() noexcept = default;

		void Clear();
		void Flush();

		void WriteLength(nox::uint64 length);
		void Write(std::span<const nox::uint8> data);

		template<typename T> requires(std::is_arithmetic_v<T>)
		inline void Write(T value)
		{
			this->Write(std::span<const nox::uint8>(reinterpret_cast<const nox::uint8*>(&value), sizeof(T)));
		}

		inline void Write(std::u8string_view str)
		{
			this->WriteLength(static_cast<nox::uint64>(str.size()));
			this->Write(std::span(reinterpret_cast<const nox::uint8*>(str.data()), str.size()));
		}

		/// @brief bitblockで値を書き込む
		/// @tparam T 
		/// @param value 
		template<class T>
		inline void WriteValue(const T& value)
		{
			this->Write(std::span(static_cast<const nox::uint8*>(std::addressof(value)), sizeof(T)));
		}

		void WriteReflection(const void* obj, const nox::reflection::Type& type);
	private:
		nox::dev::editor_remote::EditorRemoteServer& server_;
		nox::uint32 position_;
		std::array<nox::uint8, k_buffer_size> buffer_;
	};
}