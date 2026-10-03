//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	win64_socket.h
///	@brief	win64_socket
#pragma once
#include	"win64_api.h"

#if NOX_WIN64

//	WIN32_LEAN_AND_MEAN を定義した Windows.h は旧版の winsock.h を読み込まないので、
//	WinSock2.h を Windows.h の後に読んでも衝突しない。
//	ただし win64_api.h が near / far を #undef している。WinSock のヘッダーと FD_* マクロは
//	FAR (= far) を使うので、ここだけ定義し直し、ラッパーを定義し終えてから外す。
#define	near
#define	far

#pragma warning(push, 0)
#include	<WinSock2.h>
#include	<WS2tcpip.h>
#pragma warning(pop)

#pragma comment(lib, "Ws2_32.lib")

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

#undef near
#undef far
#endif