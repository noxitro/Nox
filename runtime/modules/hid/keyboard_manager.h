//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	keyboard_manager.h
///	@brief	keyboard_manager
#pragma once
#include	"../../core/system.h"
#include	"keyboard.h"
#include	"raw_keyboard.h"

#include	<array>
#include	<atomic>
#include	<span>

namespace nox::os
{
	struct WindowMessage;
}

namespace nox::hid
{
	/// @brief ウィンドウスレッドのキーイベントをSystem Updateへ渡すシステム
	/// @details Raw Input の登録とウィンドウメッセージの読み取りもここで行う。
	///			kernel には汎用のウィンドウメッセージフックだけを登録する
	class KeyboardManager : public nox::SystemBase
	{
		NOX_DECLARE_OBJECT(nox::hid::KeyboardManager, nox::SystemBase);
	public:
		KeyboardManager()noexcept;
		~KeyboardManager()override;

		[[nodiscard]] static nox::hid::KeyboardManager* GetInstance()noexcept;
		/// @brief Updateフェーズ後にキー状態を読み取ります
		/// @details 参照するSystemはkPhaseUpdateに依存させます
		[[nodiscard]] const nox::hid::Keyboard& GetKeyboard()const noexcept;

	private:
		enum class QueuedEventType : nox::uint8
		{
			/// @brief キーの押下・解放
			Key,
			/// @brief フォーカス喪失。全キーを離す
			ReleaseAll,
			/// @brief キュー溢れで取りこぼした後の再同期。全キーを離す
			Resync
		};

		struct QueuedInputEvent
		{
			QueuedEventType type;
			nox::hid::KeyCode key_code;
			bool is_down;
		};

		static constexpr nox::uint32 kEventQueueCapacity = 1024u;
		static constexpr nox::uint32 kEventQueueMask = kEventQueueCapacity - 1u;
		static_assert((kEventQueueCapacity & (kEventQueueCapacity - 1u)) == 0u);

		static void OnWindowMessage(const nox::os::WindowMessage& message, void* user_data)noexcept;

		//	以下の Produce* / TryFlushResync はウィンドウメッセージを処理するスレッドで呼ぶ
		void ProduceRawInput(nox::int64 raw_input_handle)noexcept;
		void ProduceKeyInput(const nox::hid::RawKeyboardInput& input)noexcept;
		void ProduceFocusLost()noexcept;
		bool TryFlushResync()noexcept;

		void Update(nox::World& world);
		std::span<const nox::SystemBase::PhaseRegister> GetPhaseRegisterList()const noexcept override;
		bool TryPush(const QueuedInputEvent& event)noexcept;
		bool TryPop(QueuedInputEvent& event)noexcept;
		bool TryPushReleaseAll(QueuedEventType type)noexcept;

	public:
		/// @brief 同じUpdateフェーズでキー状態を使うSystemは、このフェーズへの依存を登録します
		static constexpr SystemPhaseUpdate kPhaseUpdate
		{
			&KeyboardManager::Update,
			u8"KeyboardManager::Update"
		};

	private:
		static nox::hid::KeyboardManager* instance_;

		nox::hid::Keyboard keyboard_;
		std::array<QueuedInputEvent, kEventQueueCapacity> event_queue_{};

		//	write_index_ から read_index_ の手前までは、ウィンドウメッセージを処理するスレッドだけが触る
		alignas(64) std::atomic<nox::uint32> write_index_{ 0u };
		/// @brief キューへ積んだ時点のキー状態。auto-repeat など状態が変わらない入力を除くのに使う
		std::array<bool, nox::hid::Keyboard::kKeyCount> producer_key_state_{};
		nox::hid::RawKeyboardTranslateState producer_translate_state_{};
		/// @brief 取りこぼしがあり、Resync をまだキューへ積めていない
		bool producer_resync_pending_ = false;
		/// @brief Raw Input へ登録できたか。できなければ従来のキーメッセージで代替する
		bool is_raw_input_registered_ = false;
		/// @brief 受け取った RAWKEYBOARD をデバッグ出力へ書くか (開発ビルドで --log-raw-keyboard)
		bool is_raw_input_log_enabled_ = false;

		alignas(64) std::atomic<nox::uint32> read_index_{ 0u };
	};
}
