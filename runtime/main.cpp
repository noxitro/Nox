
#include	"stdafx.h"

//	必須
#include	"kernel/kernel.h"
#include	"reflection/reflection.h"

#include	"core/core.h"

//	module

//	app
#include	"app/app.h"

//	third party
#if NOX_DEBUG
#pragma comment(lib, "./build/third_party/x64/Debug/fmtd.lib")
#else
#pragma comment(lib, "./build/third_party/x64/Release/fmt.lib")
#endif

#if NOX_WIN64
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

#endif // NOX_WIN64

#include	<shellapi.h>

int WINAPI WinMain(_In_::HINSTANCE hInstance, _In_opt_::HINSTANCE /*hPrevInstance*/, _In_::LPSTR /*lpCmdLine*/, _In_ int nCmdShow)
{
	using namespace nox;
	
	nox::int32 arg_num;
	const nox::char16*const* argv = reinterpret_cast<const nox::char16*const*>(::CommandLineToArgvW(::GetCommandLineW(), &arg_num));
	nox::EntryPoint({ argv, static_cast<size_t>(arg_num) });
	
	return 0;
}