//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	editor_ipc_client.h
///	@brief	editor_ipc_client
#pragma once

namespace nox::dev::editor_ipc
{
	class EditorIpcClient;
	class EditorIpcClientStream
	{
	public:
		inline constexpr EditorIpcClientStream(EditorIpcClient& client) noexcept
			: client_(client)
		{
		}

	protected:
		EditorIpcClient& client_;
	};

	class EditorIpcClientWriter : public EditorIpcClientStream
	{
	public:
		inline constexpr EditorIpcClientWriter(EditorIpcClient& client) noexcept
			: EditorIpcClientStream(client)
		{
		}

	private:
		
	};

	class EditorIpcClientReader : public EditorIpcClientStream
	{
	public:
		inline constexpr EditorIpcClientReader(EditorIpcClient& client) noexcept
			: EditorIpcClientStream(client)
		{
		}
	};

	class EditorIpcClient
	{
	public:

	private:

	};
}