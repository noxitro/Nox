///	@file		stdafx.h
///	@brief		stdafx
#include	"stdafx.h"

#if NOX_X64
#if NOX_DEBUG
#pragma comment(lib, "./build/runtime/x64/Debug/kernel.lib")
#pragma comment(lib, "./build/runtime/x64/Debug/reflection.lib")
#pragma comment(lib, "./build/runtime/x64/Debug/core.lib")

#elif NOX_RELEASE
#pragma comment(lib, "build/runtime/x64/Release/kernel.lib")
#pragma comment(lib, "build/runtime/x64/Release/reflection.lib")
#pragma comment(lib, "build/runtime/x64/Release/core.lib")

#elif NOX_MASTER
#pragma comment(lib, "build/runtime/x64/Master/kernel.lib")
#pragma comment(lib, "build/runtime/x64/Master/reflection.lib")
#pragma comment(lib, "build/runtime/x64/Master/core.lib")

#endif // NOX_DEBUG

#endif // NOX_X64