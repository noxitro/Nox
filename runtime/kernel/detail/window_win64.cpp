//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	window_win64.cpp
///	@brief	window_win64
#include	"pch.h"
#include	"window_win64.h"

#include	"../window.h"

#if NOX_WINDOWS
#include	"../win64_api.h"
#include	"../os.h"
#include	"../scoped_lock.h"
#include	"../assertion.h"

namespace nox::detail
{
	namespace
	{
		struct NativeCreateArgs
		{
			nox::Window& self;
			const nox::WindowSetupDesc& desc;
		};
	}
}

struct nox::Window::Detail
{
	Detail()noexcept = delete;
	static inline ::LRESULT CALLBACK CallbackWindow(const ::HWND hWnd, const ::UINT message, const ::WPARAM wParam, ::LPARAM lParam)
	{
		if (message == WM_NCCREATE)
		{
			const ::CREATESTRUCTW* create = reinterpret_cast<const ::CREATESTRUCTW*>(lParam);
			auto* const window = reinterpret_cast<nox::Window*>(create->lpCreateParams);
			::SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<::LONG_PTR>(window));
		}

		nox::Window* const self = reinterpret_cast<nox::Window*>(::GetWindowLongPtrW(hWnd, GWLP_USERDATA));

		//	フックへは全メッセージを渡す。既定の処理はこのあと通常どおり行う
		const nox::WindowMessage window_message
		{
			hWnd,
			static_cast<nox::uint32>(message),
			static_cast<nox::uint64>(wParam),
			static_cast<nox::int64>(lParam)
		};
		nox::Window::DispatchWindowMessageHooks(window_message);

		if (self != nullptr && self->callback_ != nullptr)
		{
			nox::WindowCallbackArgs args{};
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
			//	ハンドルはゲームスレッドからも読まれるので atomic に書き換える (window_handle_ を参照)。
			if (self != nullptr && self->window_handle_.load(std::memory_order_relaxed) == hWnd)
			{
				self->window_handle_.store(nullptr, std::memory_order_release);
				self->is_visible_ = false;
			}
			::PostQuitMessage(0);
			return 0;
		}

		return ::DefWindowProcW(hWnd, message, wParam, lParam);
	}
};



void nox::Window::Create(const nox::WindowSetupDesc& desc)
{
	const nox::detail::NativeCreateArgs args{ *this, const_cast<nox::WindowSetupDesc&>(desc) };
	nox::os::detail::DispatchCreateNativeWindow(CreateNative, &args);
}

void	nox::Window::CreateNative(const void* args_ptr)
{
	NOX_ASSERT(args_ptr != nullptr, u"self is null");
	const auto* args = static_cast<const nox::detail::NativeCreateArgs*>(args_ptr);
	nox::Window& self = args->self;
	const nox::WindowSetupDesc& desc = args->desc;

	self.instance_handle_ = ::GetModuleHandleW(nullptr);

	const ::WNDCLASSEX window_class
	{
		.cbSize = sizeof(::WNDCLASSEX),
		.style = (CS_HREDRAW | CS_VREDRAW),
		.lpfnWndProc = nox::Window::Detail::CallbackWindow,
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

	const ::HWND created_handle = ::CreateWindowExW(
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
	self.window_handle_.store(created_handle, std::memory_order_release);

	if (created_handle == nullptr)
	{
		NOX_ASSERT(false, u8"ウィンドウの生成に失敗しました");
		return;
	}
}

void	nox::Window::SetPos(const nox::Int2& pos)
{
	::SetWindowPos(window_handle_.load(std::memory_order_acquire), nullptr, pos.x, pos.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void nox::Window::SetSize(const nox::Int2& size)
{
	::SetWindowPos(window_handle_.load(std::memory_order_acquire), nullptr, 0, 0, size.x, size.y, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

std::u16string_view nox::Window::GetWindowTitle(std::span<nox::char16> dest)const noexcept
{
	::GetWindowTextW(window_handle_.load(std::memory_order_acquire), nox::util::CharCast<nox::wchar16>(dest.data()), dest.size());
	return std::u16string_view(dest.data(), dest.size());
}

void nox::Window::Show()
{
	//	ゲームスレッドから呼ばれ、表示した直後にユーザーが閉じうるので、ハンドルは 1 回だけ読む。
	const nox::WindowHandle handle = window_handle_.load(std::memory_order_acquire);
	NOX_ASSERT(handle != nullptr, u8"ウィンドウハンドルが不正です");
	::ShowWindow(handle, SW_SHOW);
	::UpdateWindow(handle);
}

void nox::Window::Dispose()
{
	const nox::WindowHandle handle = window_handle_.load(std::memory_order_acquire);
	if (handle != nullptr)
	{
		::DestroyWindow(handle);
		window_handle_.store(nullptr, std::memory_order_release);
	}
}

void nox::Window::RequestClose()noexcept
{
	//	ゲームスレッドから呼ばれる。読んだ直後にユーザーが閉じて破棄されていた場合、
	//	PostMessageW は無効なハンドルとして失敗するだけで、何も起きない。
	const nox::WindowHandle handle = window_handle_.load(std::memory_order_acquire);
	if (handle != nullptr)
	{
		//	WM_CLOSE → DestroyWindow → WM_DESTROY → PostQuitMessage の順に、閉じるボタンと同じ経路をたどる
		//	(CallbackWindow を参照)。DestroyWindow は作ったスレッドでしか効かないので、直接は呼ばない。
		::PostMessageW(handle, WM_CLOSE, 0, 0);
	}
}

#endif // NOX_WINDOWS