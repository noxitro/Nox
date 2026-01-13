//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	window.h
///	@brief	window
#pragma once
#include	<functional>
#include	"../math/math.h"

namespace nox::os
{
	enum class WindowStyle : nox::uint8
	{
		/// @brief 通常ウィンドウ
		Normal = 0,
		/// @brief フルスクリーンウィンドウ
		FullScreen = 1,
		/// @brief 枠なしウィンドウ
		Boderless = 2,
	};

	struct WindowCallbackArgs
	{

	};

	struct WindowSetupDesc
	{
		nox::uint32 width;
		nox::uint16 height;
		nox::os::WindowStyle window_style;
		
		const nox::char16* title_ptr;

		std::function<void(const WindowCallbackArgs&)> callback = nullptr;

		//	
		bool resizable = true;
	};

	/// @brief ウィンドウ
	class Window
	{
	protected:
		static constexpr nox::uint16 k_max_title_length = 256;

	public:
		virtual ~Window();
		/// @brief インスタンスを生成
		/// @return 
		static	Window& Create(const WindowSetupDesc& desc);

		virtual void Show() = 0;
		virtual void Destroy() = 0;

		virtual void* GetNativeHandle()const = 0;
//		virtual void SetWindowTitle(const StringView& s) = 0;
		

		virtual void SetPos(const UInt2& pos) = 0;
		virtual UInt2 GetPos()const noexcept = 0;

		std::array<nox::char16, nox::os::Window::k_max_title_length> GetWindowTitle()const noexcept;
	protected:
		Window(const WindowSetupDesc& desc)noexcept;
		

		virtual void	Init() = 0;
		virtual std::u16string_view GetWindowTitle(std::span<nox::char16> dest)const noexcept = 0;
	protected:
		std::function<void(const WindowCallbackArgs&)> callback_;
	};
}