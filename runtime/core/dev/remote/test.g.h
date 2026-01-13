//	Copyright (C) 2026 NOX ENGINE All rights reserved.

///	@file	test.h
///	@brief	test
#pragma once
#include	"../editor_ipc_query.h"
#include	"../editor_ipc_response.h"

#if NOX_DEVELOP

namespace nox::dev::editor_ipc
{
	class Query;
	class Response;

	class ConvertQuery : public nox::dev::editor_ipc::Query
	{
		NOX_DECLARE_OBJECT(nox::dev::editor_ipc::ConvertQuery, nox::dev::editor_ipc::Query);
	public:
		ConvertQuery() {}
		void OnSerialize(SocketStreamWriter&)override;
		void OnDeserialize(SocketStreamReader&)override;
		inline constexpr auto& GetPath()const noexcept { return path_; }

		inline constexpr Response* Execute()const override { return nullptr; }
	private:
		std::array<nox::char8, 256> path_;
	};

	class ConvertResponse : public nox::dev::editor_ipc::Response
	{
		NOX_DECLARE_OBJECT(nox::dev::editor_ipc::ConvertResponse, nox::dev::editor_ipc::Response);
	public:
		void OnSerialize(SocketStreamWriter&)override;
		void OnDeserialize(SocketStreamReader&)override;
		inline constexpr const auto& GetResultPath()const noexcept { return path_; }

		inline constexpr void Execute()const override
		{
			//	resource managerに通知
		}
	private:
		std::array<nox::char8, 256> path_;
	};
}
#endif // NOX_DEVELOP