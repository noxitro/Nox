//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	test.cpp
///	@brief	test
#include	"pch.h"
#if	NOX_DEVELOP
#include	"test.g.h"
#include	"../socket_stream_writer.h"
#include	"../socket_stream_reader.h"

void nox::dev::editor_remote::ConvertQuery::OnSerialize(SocketStreamWriter& writer)
{
	writer.Write(path_);

	nox::Vector<nox::uint8> dummy_data;
	writer.WriteLength(static_cast<nox::uint64>(dummy_data.size()));
	for (const nox::uint8 b : dummy_data)
	{
		writer.Write(b);
	}
}

void nox::dev::editor_remote::ConvertQuery::OnDeserialize(SocketStreamReader& reader)
{
	reader.Read(path_);
}

nox::PlacementObject<nox::dev::editor_remote::Response> nox::dev::editor_remote::ConvertQuery::Execute(nox::World&, std::span<nox::uint8> buffer)const
{
	nox::dev::editor_remote::ConvertResponse*const response = nox::memory::ConstructAt< nox::dev::editor_remote::ConvertResponse>(buffer);


	return response;
}
#endif	//	NOX_DEVELOP
