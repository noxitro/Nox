//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	window.h
///	@brief	window
#pragma once
#include	"../math/math.h"

namespace nox::os
{
	/// @brief ウィンドウ
	class Window
	{
	public:
		/// @brief インスタンスを生成
		/// @return 
		static	Window* Create();

		virtual void Show() = 0;
		virtual void Destroy() = 0;

		virtual void* GetNativeHandle()const = 0;
//		virtual void SetWindowTitle(const StringView& s) = 0;
		virtual void GetWindowTitle()const noexcept = 0;

		virtual void SetPos(const UInt2& pos) = 0;
		virtual UInt2 GetPos()const noexcept = 0;
	protected:
		Window()noexcept;
		~Window();

		virtual void	Init() = 0;

	protected:
	};
}