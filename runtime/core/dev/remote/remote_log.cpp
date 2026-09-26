//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

#include	"pch.h"
#if NOX_DEVELOP
#include	"remote_log.g.h"

nox::PlacementObject<nox::dev::editor_remote::Response> nox::dev::editor_remote::SendLog::Execute(nox::World&, std::span<nox::uint8> storage)const
{
	return nullptr;
}
#endif // NOX_DEVELOP
