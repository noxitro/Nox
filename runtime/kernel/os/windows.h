// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

///	@file	x64.h
///	@brief	x64
#pragma once
#include	"../basic_definition.h"

#if NOX_WIN64

//	型チェックを厳密に行う
#define STRICT

//	min maxマクロを定義させない
#define NOMINMAX

#define NODRAWTEXT
//#define NOGDI
#define NOBITMAP
#define NOMCX
#define NOSERVICE
#define NOHELP

// ヘッダーからあまり使われない関数を省く
#define WIN32_LEAN_AND_MEAN
#pragma warning(push, 0)
#pragma warning(disable:4820)
#pragma warning(disable:26812)
//	windows.hはwinsock2.hが先にインクルードされていないと、
//	winsock.hを自動的にインクルードしてしまう。
#include	<WinSock2.h>
#include	<WS2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

#include	<Windows.h>
#pragma	warning(pop)

//	ここでundefするので、先に必要な関数を定義
namespace nox::os::file_descriptor
{
	inline	void	Zero(::fd_set& fd)
	{
		FD_ZERO(&fd);
	}

	inline	void	Set(::SOCKET socket, ::fd_set& fd)
	{
		FD_SET(socket, &fd);
	}

	inline	bool	IsSet(::SOCKET socket, ::fd_set& fd)
	{
		return FD_ISSET(socket, &fd);
	}

	inline	void	Clear(::SOCKET socket, ::fd_set& fd)
	{
		FD_CLR(socket, &fd);
	}
}

#undef	near
#undef	far

//#undef STRICT
//#undef NOMINMAX
#undef NODRAWTEXT
//#undef NOGDI
#undef NOBITMAP
#undef NOMCX
#undef NOSERVICE
#undef NOHELP
//#undef WIN32_LEAN_AND_MEAN

#endif // NITRO_WIN64