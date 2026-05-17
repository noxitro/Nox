//	Copyright (C) 2026 NOX ENGINE All rights reserved.

///	@file	socket_stream_writer.h
///	@brief	socket_stream_writer
#pragma once

namespace nox
{
	class Object;
}

namespace nox::dev::editor_remote
{
	class EditorRemoteServer;

	class SocketStreamWriter 
	{
	private:
		static constexpr nox::uint32 k_buffer_size = 5096;
	public:
		explicit SocketStreamWriter(nox::dev::editor_remote::EditorRemoteServer& server) noexcept;

		inline constexpr SocketStreamWriter(const SocketStreamWriter&) = delete;
		inline constexpr SocketStreamWriter(SocketStreamWriter&&) noexcept = delete;
		inline constexpr ~SocketStreamWriter() noexcept = default;

		void Clear();
		void Flush();

		void WriteLength(nox::uint64 length);
		void WriteBytes(std::span<const nox::uint8> data);

		inline void Write(std::span<const nox::uint8> data)
		{
			this->WriteLength(static_cast<nox::uint64>(data.size()));
			this->WriteBytes(data);
		}

		template<typename T> requires(std::is_arithmetic_v<T>)
		inline void Write(T value)
		{
			this->WriteBytes(std::span<const nox::uint8>(reinterpret_cast<const nox::uint8*>(&value), sizeof(T)));
		}

		template<nox::concepts::Enum T>
		inline void Write(T value)
		{
			this->Write(nox::util::ToUnderlying(value));
		}

		inline void Write(std::u8string_view str)
		{
			this->WriteLength(static_cast<nox::uint64>(str.size()));
			this->WriteBytes(std::span(reinterpret_cast<const nox::uint8*>(str.data()), str.size()));
		}

		/// @brief bitblockで値を書き込む
		/// @tparam T 
		/// @param value 
		template<class T>
		inline void WriteValue(const T& value)
		{
			this->WriteBytes(std::span(static_cast<const nox::uint8*>(std::addressof(value)), sizeof(T)));
		}

		void Write(nox::IntrusivePtr<nox::Object>& value);
	private:
		nox::uint32 WriteLeb128ToEnd(nox::uint64 length);

	private:
		nox::dev::editor_remote::EditorRemoteServer& server_;
		nox::uint32 position_;
		std::array<nox::uint8, k_buffer_size> buffer_;
	};
}