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

nox::os::Window::Window()noexcept
{

}

nox::os::Window::~Window()
{

}


nox::os::Window* nox::os::Window::Create()
{
#if NOX_WIN64
	return nullptr;
	//	return new os::detail::WindowWin64();
#else
	static_assert(false);
	return nullptr;

#endif // NOX_WIN64

}