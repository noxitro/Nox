//	Copyright (C) 2026 NOX ENGINE All rights reserved.

///	@file	test.h
///	@brief	test
#pragma once
#include	"../editor_remote_query.h"
#include	"../editor_remote_response.h"

#if NOX_DEVELOP

namespace nox::dev::editor_remote
{
	class ConvertQuery : public nox::dev::editor_remote::Query
	{
		NOX_DECLARE_OBJECT(nox::dev::editor_remote::ConvertQuery, nox::dev::editor_remote::Query);
	public:
		inline constexpr ConvertQuery() {}
		void OnSerialize(SocketStreamWriter&)override;
		void OnDeserialize(SocketStreamReader&)override;
		inline constexpr auto& GetPath()const noexcept { return path_; }
		inline void SetPath(std::u8string_view s) {
			path_ = s;
		}

		nox::PlacementObject<Response> Execute(std::span<nox::uint8> buffer)const override;
	private:
		nox::BasicFixedString<nox::char8, 256> path_;
	};

	class ConvertResponse : public nox::dev::editor_remote::Response
	{
		NOX_DECLARE_OBJECT(nox::dev::editor_remote::ConvertResponse, nox::dev::editor_remote::Response);
	public:
		inline constexpr void OnSerialize(SocketStreamWriter&)override {}
		inline constexpr void OnDeserialize(SocketStreamReader&)override {}
	
	private:
	};
}
#endif // NOX_DEVELOP