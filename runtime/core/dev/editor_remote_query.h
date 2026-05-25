//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	editor_remote_query.h
///	@brief	editor_remote_query
#pragma once
#if NOX_DEVELOP
#include	"editor_remote_entity.h"

namespace nox
{
	class World;
}

namespace nox::dev::editor_remote
{
	/// @brief 受信専用Queryタグ
	/// @details SendQuery不可
	struct IRecvOnlyQueryTag {};

	class Response;
	class Query : public nox::dev::editor_remote::EditorRemoteEntity
	{
		NOX_DECLARE_OBJECT(nox::dev::editor_remote::Query, nox::dev::editor_remote::EditorRemoteEntity);
	public:
		inline constexpr Query()noexcept {}
		inline constexpr ~Query() override {}

		/// @brief Queryを受信した時に実行される
		/// @param world ワールドインスタンス
		/// @param buffer Responseを作成するためのバッファ　配置newを使用してResponseを作成すること
		/// @return 
		inline constexpr virtual nox::PlacementObject<Response> Execute(nox::World&, [[maybe_unused]] std::span<nox::uint8> buffer)const { return nullptr; }
	};
}
#endif // NOX_DEVELOP