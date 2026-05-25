// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	log_service.cpp
/// @brief	log_service
#include "pch.h"

#include <limits>

#include "log_service.h"

#include "dev/remote/remote_log.g.h"
#include "dev/editor_remote_server.h"

namespace nox
{
	namespace
	{
		inline void SendLogToServer(
			nox::dev::editor_remote::EditorRemoteServer& server,
			const std::u8string_view message,
			const std::u8string_view callstack,
			const std::u8string_view channel,
			const nox::debug::LogLevel level)
		{
			nox::dev::editor_remote::SendLog send_log;
			send_log.SetLevel(static_cast<nox::dev::editor_remote::SendLog::LogLevel>(level));
			send_log.SetMsg(message);
			send_log.SetCallStack(callstack);
			send_log.SetChannel(channel);
			server.SendQuery(send_log);
		}
	}
}

nox::LogService::LogService():
	buffer_{ 0 },
	read_position_(0),
	server_(nullptr),
	write_position_(0)
{

}

void nox::LogService::AttachServer(nox::dev::editor_remote::EditorRemoteServer& server)
{
	{
		NOX_LOCAL_SCOPE(nox::os::ScopedWriteLock(rw_lock_));
		server_ = &server;
	}
	Flush();
}

void nox::LogService::DetachServer()
{
	NOX_LOCAL_SCOPE(nox::os::ScopedWriteLock(rw_lock_));
	server_ = nullptr;
}

void nox::LogService::AddLog(std::u8string_view message, std::u8string_view callstack, std::u8string_view channel, nox::debug::LogLevel level)
{
	NOX_LOCAL_SCOPE(nox::os::ScopedReadLock(rw_lock_));

	if (server_ != nullptr)
	{
		SendLogToServer(*server_, message, callstack, channel, level);
	}
	else
	{
		constexpr std::size_t k_max_text_length = std::numeric_limits<nox::uint16>::max();
		constexpr std::size_t k_max_channel_length = std::numeric_limits<nox::uint8>::max();
		NOX_ASSERT(message.size() <= k_max_text_length, u8"Log message is too long. size={0}", message.size());
		NOX_ASSERT(callstack.size() <= k_max_text_length, u8"Log callstack is too long. size={0}", callstack.size());
		NOX_ASSERT(channel.size() <= k_max_channel_length, u8"Log channel is too long. size={0}", channel.size());
		if (message.size() > k_max_text_length || callstack.size() > k_max_text_length || channel.size() > k_max_channel_length)
		{
			return;
		}

		const nox::uint32 total_size = static_cast<nox::uint32>(Data::k_header_size + message.size() + callstack.size() + channel.size());
		NOX_ASSERT(total_size <= k_buffer_size, u8"Log data is too long. size={0}", total_size);
		if (total_size > k_buffer_size)
		{
			return;
		}

		NOX_LOCAL_SCOPE(nox::os::ScopedLock(mutex_));

		const nox::uint32 write_position = write_position_.load();
		NOX_ASSERT(write_position <= k_buffer_size, u8"LogService buffer is broken.");
		if (write_position > k_buffer_size)
		{
			return;
		}

		const nox::uint32 capacity = k_buffer_size - write_position;
		NOX_ASSERT(total_size <= capacity, u8"LogService buffer is full. size={0}, capacity={1}", total_size, capacity);
		if (total_size > capacity)
		{
			return;
		}

		nox::uint8* cursor = buffer_.data() + write_position;
		const nox::uint16 message_length = static_cast<nox::uint16>(message.size());
		const nox::uint16 callstack_length = static_cast<nox::uint16>(callstack.size());
		const nox::uint8 channel_length = static_cast<nox::uint8>(channel.size());
		nox::memory::Copy(static_cast<void*>(cursor), &message_length, sizeof(message_length));
		cursor += sizeof(message_length);
		nox::memory::Copy(static_cast<void*>(cursor), &callstack_length, sizeof(callstack_length));
		cursor += sizeof(callstack_length);
		nox::memory::Copy(static_cast<void*>(cursor), &channel_length, sizeof(channel_length));
		cursor += sizeof(channel_length);
		nox::memory::Copy(static_cast<void*>(cursor), &level, sizeof(level));
		cursor += sizeof(level);

		if (message.empty() == false)
		{
			nox::memory::Copy(static_cast<void*>(cursor), message.data(), message.size());
			cursor += message.size();
		}

		if (callstack.empty() == false)
		{
			nox::memory::Copy(static_cast<void*>(cursor), callstack.data(), callstack.size());
			cursor += callstack.size();
		}

		if (channel.empty() == false)
		{
			nox::memory::Copy(static_cast<void*>(cursor), channel.data(), channel.size());
		}

		write_position_.store(write_position + total_size);
	}
}

void nox::LogService::Flush()
{
	NOX_LOCAL_SCOPE(nox::os::ScopedReadLock(rw_lock_));
	if (server_ == nullptr)
	{
		return;
	}

	NOX_LOCAL_SCOPE(nox::os::ScopedLock(mutex_));

	const nox::uint32 write_position = write_position_.load();
	nox::uint32 position = read_position_;

	while (position < write_position)
	{
		NOX_ASSERT(write_position - position >= Data::k_header_size, u8"LogService buffer is broken.");
		if (write_position - position < Data::k_header_size)
		{
			break;
		}

		const nox::uint8* cursor = buffer_.data() + position;
		Data data{};
		nox::memory::Copy(&data.message_length_, cursor, sizeof(data.message_length_));
		cursor += sizeof(data.message_length_);
		nox::memory::Copy(&data.callstack_length_, cursor, sizeof(data.callstack_length_));
		cursor += sizeof(data.callstack_length_);
		nox::memory::Copy(&data.channel_length_, cursor, sizeof(data.channel_length_));
		cursor += sizeof(data.channel_length_);
		nox::memory::Copy(&data.level_, cursor, sizeof(data.level_));
		cursor += sizeof(data.level_);

		NOX_ASSERT(data.GetSize() <= write_position - position, u8"LogService buffer is broken.");
		if (data.GetSize() > write_position - position)
		{
			break;
		}

		data.message_ = reinterpret_cast<const nox::char8*>(cursor);
		cursor += data.message_length_;
		data.callstack_ = reinterpret_cast<const nox::char8*>(cursor);
		cursor += data.callstack_length_;
		data.channel_ = reinterpret_cast<const nox::char8*>(cursor);

		Flush(data);
		position += data.GetSize();
	}

	read_position_ = 0;
	write_position_.store(0);
}

void nox::LogService::Flush(const Data& data)const
{
	NOX_ASSERT(server_ != nullptr, u8"EditorRemoteServer is not attached.");
	if (server_ == nullptr)
	{
		return;
	}

	SendLogToServer(*server_, data.GetMeg(), data.GetCallstack(), data.GetChannel(), data.level_);
}

void nox::LogService::LogHandler(const nox::debug::LogHandlerArgs& args)
{
	AddLog(args.message, args.callstack, args.channel, args.level);
}
