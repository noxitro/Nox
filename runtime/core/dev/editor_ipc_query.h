//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	editor_ipc_query.h
///	@brief	editor_ipc_query
#pragma once
#if NOX_DEVELOP
#include	"editor_ipc_entity.h"

namespace nox::dev::editor_ipc
{
	class Response;
	class Query : public nox::dev::editor_ipc::EditorIpcEntity
	{
		NOX_DECLARE_OBJECT(nox::dev::editor_ipc::Query, nox::dev::editor_ipc::EditorIpcEntity);
	public:
		inline constexpr Query() {}
		inline constexpr ~Query() noexcept override {}

		/// @brief Queryを受信した時に実行される
		/// @param buffer Responseを作成するためのバッファ　配置newを使用してResponseを作成すること
		/// @return 
		inline constexpr virtual nox::PlacementObject<Response> Execute(std::span<nox::uint8> buffer)const { return nullptr; }
	};
}
#endif // NOX_DEVELOP