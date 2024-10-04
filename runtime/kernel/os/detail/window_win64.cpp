//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	window_win64.cpp
///	@brief	window_win64
#include	"stdafx.h"
#include	"window_win64.h"

#include	<filesystem>

namespace
{
	::LRESULT CALLBACK CallbackWindow(const ::HWND hWnd, const ::UINT message, const ::WPARAM wParam, ::LPARAM lParam)
	{
		return ::DefWindowProcW(hWnd, message, wParam, lParam);
	}
}

nox::os::detail::WindowWin64::WindowWin64()noexcept:
	window_handle_(nullptr),
	instance_handle_(nullptr)
{

}

nox::os::detail::WindowWin64::~WindowWin64()
{

}

void	nox::os::detail::WindowWin64::SetPos(const UInt2& pos)
{

}

nox::UInt2 nox::os::detail::WindowWin64::GetPos()const noexcept
{
	return UInt2();
}

void	nox::os::detail::WindowWin64::Init()
{
	instance_handle_ = ::GetModuleHandleW(nullptr);

	const ::WNDCLASSEX window_class
	{
		.cbSize = sizeof(::WNDCLASSEX),
		.style = (CS_HREDRAW | CS_VREDRAW),
		.lpfnWndProc = CallbackWindow,
		.hInstance = instance_handle_,
		.hIcon = ::LoadIconW(instance_handle_, MAKEINTRESOURCEW(100)),
		.hCursor = nullptr,
		.hbrBackground = static_cast<HBRUSH>(::GetStockObject(DKGRAY_BRUSH)),
		.lpszClassName = L"runtime"
	};

	std::array<char, 256> b = std::array<char, 256>{ "aseaa" };
}