//	Copyright (C) 2025 NOX ENGINE All Rights Rserved.

///	@file	guid.cpp
///	@brief	guid
#include	"pch.h"
#include	"guid.h"

#if NOX_WINDOWS
#include <combaseapi.h>   // CoCreateGuid
#pragma comment(lib, "ole32.lib")
#endif

nox::Guid nox::Guid::NewGuid() noexcept
{
#if NOX_WINDOWS
	::GUID w{};
	if (SUCCEEDED(::CoCreateGuid(&w)))
	{
		Guid g;
		g.data1 = static_cast<nox::uint32>(w.Data1);
		g.data2 = static_cast<nox::uint16>(w.Data2);
		g.data3 = static_cast<nox::uint16>(w.Data3);
		for (nox::int32 i = 0; i < std::size(g.data4); ++i)
		{
			g.data4[i] = static_cast<nox::uint8>(w.Data4[i]);
		}
		return g;
	}
	else
	{
		return {};
	}

#else
	return {};
#endif
	//// フォールバック: RFC4122 version 4 (random)
	//Guid g;
	//std::random_device rd;
	//std::mt19937_64 gen((static_cast<uint64_t>(rd()) << 32) ^ rd());
	//std::uniform_int_distribution<uint32_t> dist32;
	//std::uniform_int_distribution<uint16_t> dist16;
	//std::uniform_int_distribution<uint8_t>  dist8;

	//g.data1 = dist32(gen);
	//g.data2 = dist16(gen);
	//g.data3 = dist16(gen);
	//// バージョン(4)とバリアント(10xx)を設定
	//g.data3 = static_cast<nox::uint16>((g.data3 & 0x0FFFu) | 0x4000u);
	//for (int i = 0; i < 8; ++i) { g.data4[i] = dist8(gen); }
	//g.data4[0] = static_cast<nox::uint8>((g.data4[0] & 0x3Fu) | 0x80u);

}