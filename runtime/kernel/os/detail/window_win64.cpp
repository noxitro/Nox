//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	window_win64.cpp
///	@brief	window_win64
#include	"stdafx.h"
#include	"window_win64.h"

#include	<filesystem>
#include	"assertion.h"

namespace nox::os::detail
{
	struct WindowWin64Impl
	{
		static inline ::LRESULT CALLBACK CallbackWindow(const ::HWND hWnd, const ::UINT message, const ::WPARAM wParam, ::LPARAM lParam)
		{
			if (message == WM_NCCREATE)
			{
				const ::CREATESTRUCTW* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
				auto* window = reinterpret_cast<WindowWin64*>(create->lpCreateParams);
				::SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
			}

			nox::os::detail::WindowWin64*const self = reinterpret_cast<WindowWin64*>(::GetWindowLongPtrW(hWnd, GWLP_USERDATA));
			if (self != nullptr && self->callback_ != nullptr)
			{
				nox::os::WindowCallbackArgs args;
				self->callback_(args);
			}

			switch (message)
			{
			case WM_DESTROY:
				::PostQuitMessage(0);
				break;
			}

			return ::DefWindowProcW(hWnd, message, wParam, lParam);
		}
	};
}

nox::os::detail::WindowWin64::WindowWin64(const nox::os::WindowSetupDesc& desc)noexcept:
	nox::os::Window(desc),
	window_handle_(nullptr),
	instance_handle_(nullptr)
{
	instance_handle_ = ::GetModuleHandleW(nullptr);

	const ::WNDCLASSEX window_class
	{
		.cbSize = sizeof(::WNDCLASSEX),
		.style = (CS_HREDRAW | CS_VREDRAW),
		.lpfnWndProc = WindowWin64Impl::CallbackWindow,
		.hInstance = instance_handle_,
		.hIcon = ::LoadIconW(instance_handle_, MAKEINTRESOURCEW(100)),
		.hCursor = nullptr,
		.hbrBackground = static_cast<HBRUSH>(::GetStockObject(DKGRAY_BRUSH)),
		.lpszClassName = L"runtime"
	};

	if (::RegisterClassExW(&window_class) == 0)
	{
		auto error_code = ::GetLastError();
		NOX_ASSERT(false, u"ウィンドウクラスの登録に失敗しました");
		return;
	}

	const ::DWORD style = WS_OVERLAPPEDWINDOW;
	const ::DWORD style_ex = 0;

	::RECT rect{ 0, 0, desc.width, desc.height };
	::AdjustWindowRectEx(&rect, style, FALSE, style_ex);

	const int winWidth = rect.right - rect.left;
	const int winHeight = rect.bottom - rect.top;

	window_handle_ = ::CreateWindowExW(
		style_ex,
		L"runtime",                       // クラス名（上で登録したもの）
		nox::util::CharCast<nox::wchar16>(desc.title_ptr), // タイトル（UTF-16 ならそのまま）
		style,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		winWidth,
		winHeight,
		nullptr,                          // 親ウィンドウ
		nullptr,                          // メニュー
		instance_handle_,                 // インスタンスハンドル
		this                           // ユーザーデータ
	);

	if (window_handle_ == nullptr)
	{
		NOX_ASSERT(false, u"ウィンドウの生成に失敗しました");
		return;
	}
}

nox::os::detail::WindowWin64::~WindowWin64()
{

}

void	nox::os::detail::WindowWin64::SetPos(const UInt2& pos)
{
	::SetWindowPos(window_handle_, nullptr, pos.x, pos.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

nox::UInt2 nox::os::detail::WindowWin64::GetPos()const noexcept
{
	//	cacheしておく？
	::RECT rect;
	::GetWindowRect(window_handle_, &rect);
	return UInt2(static_cast<nox::uint32>(rect.left), static_cast<nox::uint32>(rect.top));
}

void	nox::os::detail::WindowWin64::Init()
{
	
}

void nox::os::detail::WindowWin64::Show()
{
	NOX_ASSERT(window_handle_ != nullptr, u"ウィンドウハンドルが不正です");
	::ShowWindow(window_handle_, SW_SHOW);
	::UpdateWindow(window_handle_);
}

void nox::os::detail::WindowWin64::Destroy()
{

}

std::u16string_view nox::os::detail::WindowWin64::GetWindowTitle(std::span<nox::char16> dest)const noexcept
{
	::GetWindowTextW(window_handle_, nox::util::CharCast<nox::wchar16>(dest.data()), dest.size());
	return std::u16string_view(dest);
}