//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	window.cpp
///	@brief	window
#include	"pch.h"
#include	"window.h"

#include	<filesystem>
#if NOX_WINDOWS
#include	"windows.h"
#endif // NOX_WINDOWS

#include	"../basic_definition.h"
#include	"assertion.h"
#include	"os.h"

namespace nox::os
{
	struct NativeCreateArgs
	{
		nox::os::Window& self;
		WindowSetupDesc& desc;
	};
}

::LRESULT CALLBACK nox::os::Window::CallbackWindow(const ::HWND hWnd, const ::UINT message, const ::WPARAM wParam, ::LPARAM lParam)
{
	if (message == WM_NCCREATE)
	{
		const ::CREATESTRUCTW* create = reinterpret_cast<const ::CREATESTRUCTW*>(lParam);
		auto*const window = reinterpret_cast<nox::os::Window*>(create->lpCreateParams);
		::SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<::LONG_PTR>(window));
	}

	nox::os::Window* const self = reinterpret_cast<nox::os::Window*>(::GetWindowLongPtrW(hWnd, GWLP_USERDATA));
	if (self != nullptr && self->callback_ != nullptr)
	{
		nox::os::WindowCallbackArgs args{};
		bool invoke_args = true;
		switch (message)
		{
			case WM_MOVE:
			{
				const nox::int32 x = static_cast<nox::int32>(LOWORD(lParam));
				const nox::int32 y = static_cast<nox::int32>(HIWORD(lParam));
				self->pos_ = nox::Int2{ x, y };
				args.move.x = x;
				args.move.y = y;
			}
			break;

			case WM_SIZE:
			{
				const nox::uint32 width = static_cast<nox::uint32>(LOWORD(lParam));
				const nox::uint32 height = static_cast<nox::uint32>(HIWORD(lParam));
				self->size_ = nox::Int2{ static_cast<nox::int32>(width), static_cast<nox::int32>(height) };

				args.resize.width = width;
				args.resize.height = height;
			}
			break;

			default:
				invoke_args = false;
				break;
		}

		if (invoke_args)
		{
			self->callback_(args);
		}
	}

	switch (message)
	{
	case WM_CLOSE:
		::DestroyWindow(hWnd);
		return 0;

	case WM_DESTROY:
		if (self != nullptr && self->window_handle_ == hWnd)
		{
			self->window_handle_ = nullptr;
			self->is_visible_ = false;
		}
		::PostQuitMessage(0);
		return 0;
	}

	return ::DefWindowProcW(hWnd, message, wParam, lParam);
}

nox::os::Window::Window()noexcept:
	callback_(nullptr),
	window_handle_(nullptr),
	instance_handle_(nullptr),
	is_visible_(false)
{

}

nox::os::Window::~Window()
{
	Dispose();
}

void nox::os::Window::Create(const WindowSetupDesc& desc)
{
	const nox::os::NativeCreateArgs args{ *this, const_cast<WindowSetupDesc&>(desc) };
	nox::os::detail::DispatchCreateNativeWindow(CreateNative, &args);
}

std::array<nox::char16, nox::os::Window::k_max_title_length> nox::os::Window::GetWindowTitle()const noexcept
{
	std::array<nox::char16, k_max_title_length> title_buffer{0};
	this->GetWindowTitle(std::span<nox::char16>(title_buffer.data(), title_buffer.size()));
	return title_buffer;
}

void	nox::os::Window::SetPos(const nox::Int2& pos)
{
#if NOX_WINDOWS
	::SetWindowPos(window_handle_, nullptr, pos.x, pos.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
#endif // NOX_WINSOWS
}

void nox::os::Window::SetSize(const nox::Int2& size)
{
#if NOX_WINDOWS
	::SetWindowPos(window_handle_, nullptr, 0, 0, size.x, size.y, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
#endif
}

std::u16string_view nox::os::Window::GetWindowTitle(std::span<nox::char16> dest)const noexcept
{
#if NOX_WINDOWS
	::GetWindowTextW(window_handle_, nox::util::CharCast<nox::wchar16>(dest.data()), dest.size());
#else
	static_assert(false, "Not implemented");
#endif // NOX_WINDOWS
	return std::u16string_view(dest);
}

void	nox::os::Window::CreateNative(const void* args_ptr)
{
	NOX_ASSERT(args_ptr != nullptr, u"self is null");
	const auto* args = static_cast<const nox::os::NativeCreateArgs*>(args_ptr);
	nox::os::Window& self = args->self;
	WindowSetupDesc& desc = args->desc;

#if NOX_WINDOWS
	self.instance_handle_ = ::GetModuleHandleW(nullptr);

	const ::WNDCLASSEX window_class
	{
		.cbSize = sizeof(::WNDCLASSEX),
		.style = (CS_HREDRAW | CS_VREDRAW),
		.lpfnWndProc = nox::os::Window::CallbackWindow,
		.hInstance = self.instance_handle_,
		.hIcon = ::LoadIconW(self.instance_handle_, MAKEINTRESOURCEW(100)),
		.hCursor = nullptr,
		.hbrBackground = static_cast<HBRUSH>(::GetStockObject(DKGRAY_BRUSH)),
		.lpszClassName = L"nox"
	};

	if (::RegisterClassExW(&window_class) == 0)
	{
		auto error_code = ::GetLastError();
		NOX_ASSERT(false, u"ウィンドウクラスの登録に失敗しました");
		return;
	}

	const ::DWORD style = WS_OVERLAPPEDWINDOW;
	const ::DWORD style_ex = 0;

	::RECT rect{ 0, 0, 1920, 1080 };
	::AdjustWindowRectEx(&rect, style, FALSE, style_ex);

	const int winWidth = rect.right - rect.left;
	const int winHeight = rect.bottom - rect.top;

	::HINSTANCE h_inst = ::GetModuleHandleW(nullptr);

	self.window_handle_ = ::CreateWindowExW(
		style_ex,
		L"nox",                       // クラス名（上で登録したもの）
		reinterpret_cast<const nox::wchar16*>(desc.title_ptr), // タイトル（UTF-16 ならそのまま）
		style,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		winWidth,
		winHeight,
		nullptr,	// 親ウィンドウ
		nullptr,	// メニュー
		h_inst,		// インスタンスハンドル
		&self		// ユーザーデータ
	);

	if (self.window_handle_ == nullptr)
	{
		NOX_ASSERT(false, u8"ウィンドウの生成に失敗しました");
		return;
	}
#endif // NOX_WINDOWS

}

void nox::os::Window::Show()
{
	NOX_ASSERT(window_handle_ != nullptr, u8"ウィンドウハンドルが不正です");
	::ShowWindow(window_handle_, SW_SHOW);
	::UpdateWindow(window_handle_);
}

void nox::os::Window::Dispose()
{
	if (window_handle_ != nullptr)
	{
		::DestroyWindow(window_handle_);
		window_handle_ = nullptr;
	}
}