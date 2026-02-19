//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	editor_remote_client.h
///	@brief	editor_remote_client
#pragma once

namespace nox::dev::editor_remote
{
	class EditorRemoteClient;
	class EditorRemoteClientStream
	{
	public:
		inline constexpr EditorRemoteClientStream(EditorRemoteClient& client) noexcept
			: client_(client)
		{
		}

	protected:
		EditorRemoteClient& client_;
	};

	class EditorRemoteClientWriter : public EditorRemoteClientStream
	{
	public:
		inline constexpr EditorRemoteClientWriter(EditorRemoteClient& client) noexcept
			: EditorRemoteClientStream(client)
		{
		}

	private:
		
	};

	class EditorRemoteClientReader : public EditorRemoteClientStream
	{
	public:
		inline constexpr EditorRemoteClientReader(EditorRemoteClient& client) noexcept
			: EditorRemoteClientStream(client)
		{
		}
	};

	class EditorRemoteClient
	{
	public:

	private:

	};
}