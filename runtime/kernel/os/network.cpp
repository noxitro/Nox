//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	network.cpp
///	@brief	network
#include	"stdafx.h"
#include	"network.h"

#include    "windows.h"
#include    "../assertion.h"
#include    "../string_format.h"
using namespace nox;

namespace
{
    
}

void	os::network::Initialize()
{
#if NOX_WIN64

    ::WSADATA wsa_data;
    const int32 error_code = ::WSAStartup(MAKEWORD(2, 2), &wsa_data);
 //   util::Format(u"error code:{0}", error_code);
  //  NOX_ASSERT(error_code != 0, util::Format(U"error code:{0}", error_code));

 //   ::GetHostNameW
#endif // NOXWIN64

}