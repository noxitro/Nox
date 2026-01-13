//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	window.cpp
///	@brief	window
#include	"stdafx.h"
#include	"window.h"

#include	"../basic_definition.h"
#if NOX_WIN64
#include	"detail/window_win64.h"
#else
static_assert(false);
#endif // NOX_WIN64

nox::os::Window::Window(const WindowSetupDesc& desc)noexcept:
	callback_(desc.callback)
{

}

nox::os::Window::~Window()
{

}


nox::os::Window& nox::os::Window::Create(const WindowSetupDesc& desc)
{
#if NOX_WIN64
	return *new os::detail::WindowWin64(desc);
#else
	static_assert(false);
	return nullptr;
#endif // NOX_WIN64

}

std::array<nox::char16, nox::os::Window::k_max_title_length> nox::os::Window::GetWindowTitle()const noexcept
{
	std::array<nox::char16, k_max_title_length> title_buffer{0};
	this->GetWindowTitle(std::span<nox::char16>(title_buffer.data(), title_buffer.size()));
	return title_buffer;
}