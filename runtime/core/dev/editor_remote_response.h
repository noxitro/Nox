//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	editor_remote_response.h
///	@brief	editor_remote_response
#pragma once
#if NOX_DEVELOP
#include	"editor_remote_entity.h"

namespace nox::dev::editor_remote
{
	class Response : public nox::dev::editor_remote::EditorRemoteEntity
	{
		NOX_DECLARE_OBJECT(nox::dev::editor_remote::Response, nox::dev::editor_remote::EditorRemoteEntity);
	public:
		inline constexpr Response() noexcept {}
		inline constexpr ~Response() noexcept override {}
	};
}
#endif // NOX_DEVELOP