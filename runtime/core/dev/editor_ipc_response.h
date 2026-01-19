//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	editor_ipc_response.h
///	@brief	editor_ipc_response
#pragma once
#if NOX_DEVELOP
#include	"editor_ipc_entity.h"

namespace nox::dev::editor_ipc
{
	class Response : public nox::dev::editor_ipc::EditorIpcEntity
	{
		NOX_DECLARE_OBJECT(nox::dev::editor_ipc::Response, nox::dev::editor_ipc::EditorIpcEntity);
	public:
		inline constexpr Response() noexcept {}
		inline constexpr ~Response() noexcept override {}
	};
}
#endif // NOX_DEVELOP