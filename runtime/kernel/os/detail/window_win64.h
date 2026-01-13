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
		friend class WindowWin64Impl;
	public:
		WindowWin64(const WindowSetupDesc& desc)noexcept;
		~WindowWin64()override;

		inline virtual void* GetNativeHandle()const override { return window_handle_; }
		void	SetPos(const UInt2& pos)override;
		UInt2 GetPos()const noexcept override;
		void Show()override;
		void Destroy()override;
		std::u16string_view GetWindowTitle(std::span<nox::char16> dest)const noexcept override;
	private:
		void	Init()override;

	private:
		::HWND window_handle_;
		::HINSTANCE instance_handle_;
	};
}

#endif // NOX_WIN64