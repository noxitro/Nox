//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	window.h
///	@brief	window
#pragma once
#include	<atomic>
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

	/// @brief ウィンドウメッセージ
	struct WindowMessage
	{
		nox::os::WindowHandle window_handle;
		nox::uint32 message;
		nox::uint64 wparam;
		nox::int64 lparam;
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

		///	@brief		ウィンドウを閉じるよう要求する。ユーザーが閉じるボタンを押したのと同じ。
		///	@details	WM_CLOSE を投げるだけで、実際に閉じるのはウィンドウを作ったスレッドのメッセージ処理。
		///				PostMessage はスレッドをまたいでよいので、どのスレッドから呼んでもよい。
		void RequestClose()noexcept;

		///	@details	ゲームスレッド (Editor から届く GetMainSceneView の処理など) から呼ばれ、UI スレッドの WM_DESTROY と重なりうる。
		inline void* GetNativeHandle()const noexcept { return window_handle_.load(std::memory_order_acquire); }

		void SetPos(const nox::Int2& pos);
		inline const nox::Int2& GetPos()const noexcept { return pos_; }

		void SetSize(const nox::Int2& size);
		inline const nox::Int2& GetSize()const noexcept { return size_; }

		std::array<nox::char16, nox::os::Window::k_max_title_length> GetWindowTitle()const noexcept;

		bool RegisterMessageHook(void(&func)(const nox::os::WindowMessage& message, void* user_data), void* user_data);
		bool UnregisterMessageHook(void(&func)(const nox::os::WindowMessage& message, void* user_data), void* user_data);

		template<auto Func, class T>
			requires (
			std::is_invocable_r_v<void, decltype(Func),	const WindowMessage&, T&>
			&& !std::is_volatile_v<T>
			)
		inline bool RegisterMessageHook(T& user_data)
		{
			return RegisterMessageHook(
				+[](const nox::os::WindowMessage& message, void* user_data_ptr)
				{
					std::invoke(Func, message, *static_cast<T*>(user_data_ptr));
				},
				const_cast<void*>(
					static_cast<const void*>(std::addressof(user_data))));
		}
	private:
		static void	CreateNative(const void* self);
		std::u16string_view GetWindowTitle(std::span<nox::char16> dest)const noexcept;
		static inline ::LRESULT CALLBACK CallbackWindow(const ::HWND hWnd, const ::UINT message, const ::WPARAM wParam, ::LPARAM lParam);
	private:
		nox::Int2 pos_;
		nox::Int2 size_;
		bool is_visible_;

		///	@brief		ウィンドウハンドル。破棄されると nullptr になる。
		///	@details	書くのはウィンドウを作ったスレッド (UI スレッド) の CreateNative と WM_DESTROY、それに終了処理の Dispose。
		///				Dispose はゲームスレッドから呼ばれるが、World::Run の kill_ で UI スレッドの処理より後に順序付けられている。
		///				読むのはゲームスレッド (Show / RequestClose / GetNativeHandle など) も含む。
		///				ユーザーが閉じた瞬間にゲームスレッドが読むと非 atomic ではデータ競合になるので、atomic にしてある。
		///				読み書きは作成・破棄・終了要求のときだけで、フレームごとには触らない。
		std::atomic<nox::os::WindowHandle> window_handle_;
		nox::os::InstanceHandle instance_handle_;
		std::function<void(const WindowCallbackArgs&)> callback_;

	};
}