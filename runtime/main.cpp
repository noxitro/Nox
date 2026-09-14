// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

#include	"pch.h"

//	必須
#include	"kernel/kernel.h"
#include	"reflection/reflection.h"

#include	"core/core.h"
//	module

//	app
#include	"app/app.h"

//	third party

//  fmt ライブラリ (vcpkg)。探索パスは nox_common.props の NoxVcpkgLibDir
//  (Debug は debug\lib、それ以外は lib) が通しているので、名前だけ書く。
//  以前はリポジトリ同梱の bin/x64/<構成>/ を直接指していたが、fmt を vcpkg へ
//  移した際にここだけ残り、同梱ファイルを消した時点でリンクできなくなった。
#if NOX_DEBUG
#pragma comment(lib, "fmtd.lib")
#else
#pragma comment(lib, "fmt.lib")
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

int WINAPI wWinMain(_In_ ::HINSTANCE /*hInstance*/, _In_opt_ ::HINSTANCE, _In_ LPWSTR /*lpCmdLine*/, _In_ int /*nCmdShow*/)
{
	nox::int32 arg_num;
	auto argv_raw = ::CommandLineToArgvW(::GetCommandLineW(), &arg_num);
	if (argv_raw == nullptr)
	{
		NOX_ASSERT(false, u8"コマンドライン引数の取得に失敗");
		return -1;
	}

	const nox::char16* const*const argv = reinterpret_cast<const nox::char16* const*>(argv_raw);
	nox::EntryPoint({ argv, static_cast<std::size_t>(arg_num) });
	::LocalFree(argv_raw);
	return 0;
}
