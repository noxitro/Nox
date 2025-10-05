//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	editor_ipc_query.h
///	@brief	editor_ipc_query
#pragma once
#include	"editor_ipc_entity.h"

#if NOX_DEVELOP
namespace nox::dev::editor_ipc
{
	class Query : public nox::dev::editor_ipc::EditorIpcEntity
	{
		NOX_DECLARE_OBJECT(nox::dev::editor_ipc::Query, nox::dev::editor_ipc::EditorIpcEntity);
	public:
		inline constexpr Query() {}
	};
}
#endif // NOX_DEVELOP