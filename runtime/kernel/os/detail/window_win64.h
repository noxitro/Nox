//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	window_win64.h
///	@brief	window_win64
#pragma once
#include	"../../basic_definition.h"

#if NOX_WIN64
#include	"../window.h"
#include	"../windows.h"

namespace nox::os::detail
{
	class WindowWin64 : public Window
	{
	public:
		WindowWin64()noexcept;
		~WindowWin64();

		inline virtual void* GetNativeHandle()const override { return window_handle_; }
		void	SetPos(const UInt2& pos)override;
		UInt2 GetPos()const noexcept override;

	private:
		void	Init()override;

	private:
		::HWND window_handle_;
		::HINSTANCE instance_handle_;
	};
}

#endif // NOX_WIN64