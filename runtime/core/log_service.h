// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	log_service.h
/// @brief	log_service
#pragma once

namespace nox
{
	namespace dev::editor_remote
	{
		class EditorRemoteServer;
	}

	/// @brief runtimeのログをserverに送信するクラス
	class LogService
	{
	private:
		static constexpr nox::uint32 k_buffer_size = nox::math::Pow(2, 13);
		struct Data
		{
			//	header部分

			nox::uint16 message_length_;
			nox::uint16 callstack_length_;
			nox::uint8 channel_length_;

			nox::debug::LogLevel level_;

			const nox::char8* message_;
			const nox::char8* callstack_;
			const nox::char8* channel_;

			constexpr static nox::uint32 k_header_size = sizeof(message_length_) + sizeof(callstack_length_) + sizeof(level_);

			inline constexpr nox::uint32 GetSize()const noexcept
			{
				return k_header_size + message_length_ + callstack_length_;
			}

			inline constexpr std::u8string_view GetMeg()const noexcept
			{
				return std::u8string_view(message_, message_length_);
			}

			inline constexpr std::u8string_view GetCallstack()const noexcept
			{
				return std::u8string_view(callstack_, callstack_length_);
			}

			inline constexpr std::u8string_view GetChannel()const noexcept
			{
				return std::u8string_view(channel_, channel_length_);
			}
		};
	public:
		LogService();

		void AttachServer(nox::dev::editor_remote::EditorRemoteServer& server);
		void DetachServer();

		void AddLog(std::u8string_view message, std::u8string_view callstack, std::u8string_view channel, nox::debug::LogLevel level);

		void LogHandler(const nox::debug::LogHandlerArgs&);
	private:
		void Flush();
		void Flush(const Data& data)const;
	private:
		std::array<nox::uint8, k_buffer_size> buffer_;
		std::uint32_t read_position_;
		nox::dev::editor_remote::EditorRemoteServer* server_;
		nox::os::Mutex mutex_;
		std::atomic<nox::uint32> write_position_;
		nox::os::ReadWriteLock rw_lock_;
	};
}
