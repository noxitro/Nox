//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	window.h
///	@brief	window
#pragma once
#include	<functional>
#include	"../math/math.h"

#include	"windows.h"

namespace nox::os
{
#if NOX_WINDOWS
	using WindowHandle = ::HWND;
	using InstanceHandle = ::HINSTANCE;
#else
	using WindowHandle = void*;
	using InstanceHandle = void*;
#endif // NOX_WINDOWS

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
		union
		{
			struct
			{
				nox::int32 x;
				nox::int32 y;
			} move;

			struct
			{
				nox::uint32 width;
				nox::uint32 height;
			} resize;
		};
	};

	struct WindowSetupDesc
	{
		nox::uint32 width;
		nox::uint32 height;
		nox::os::WindowStyle window_style;
		
		const nox::char16* title_ptr;

		std::function<void(const WindowCallbackArgs&)> callback = nullptr;

		//	
		bool resizable = true;
	};

	/// @brief ウィンドウ
	class Window
	{
	public:
		static constexpr nox::uint16 k_max_title_length = 256;

	public:
		Window()noexcept;
		~Window();

		inline constexpr Window(const Window&)noexcept = delete;
		inline constexpr Window(Window&&)noexcept = delete;

		void Create(const WindowSetupDesc& desc);

		void Show();
		void Dispose();

		inline void* GetNativeHandle()const { return window_handle_; }

		void SetPos(const nox::Int2& pos);
		inline const nox::Int2& GetPos()const noexcept { return pos_; }

		void SetSize(const nox::Int2& size);
		inline const nox::Int2& GetSize()const noexcept { return size_; }

		std::array<nox::char16, nox::os::Window::k_max_title_length> GetWindowTitle()const noexcept;
	private:
		static void	CreateNative(const void* self);
		std::u16string_view GetWindowTitle(std::span<nox::char16> dest)const noexcept;
		static inline ::LRESULT CALLBACK CallbackWindow(const ::HWND hWnd, const ::UINT message, const ::WPARAM wParam, ::LPARAM lParam);
	private:
		nox::Int2 pos_;
		nox::Int2 size_;
		bool is_visible_;

		nox::os::WindowHandle window_handle_;
		nox::os::InstanceHandle instance_handle_;
		std::function<void(const WindowCallbackArgs&)> callback_;

	};
}