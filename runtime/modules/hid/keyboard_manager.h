//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	keyboard_manager.h
///	@brief	keyboard_manager
#pragma once
#include	"../../core/system.h"
#include	"../../kernel/os/os.h"
#include	"keyboard.h"

#include	<array>
#include	<atomic>
#include	<span>

namespace nox::hid
{
	/// @brief ウィンドウスレッドのキーイベントをSystem Updateへ渡すシステム
	class KeyboardManager : public nox::SystemBase
	{
		NOX_DECLARE_OBJECT(nox::hid::KeyboardManager, nox::SystemBase);
	public:
		KeyboardManager()noexcept;
		~KeyboardManager()override;

		[[nodiscard]] static nox::hid::KeyboardManager* GetInstance()noexcept;
		/// @brief Updateフェーズ後にキー状態を読み取ります
		/// @details 参照するSystemはk_phase_updateに依存させます
		[[nodiscard]] const nox::hid::Keyboard& GetKeyboard()const noexcept;

	private:
		struct QueuedInputEvent
		{
			nox::hid::KeyCode key_code;
			bool is_down;
			bool release_all;
		};

		static constexpr nox::uint32 k_event_queue_capacity = 1024u;
		static constexpr nox::uint32 k_event_queue_mask = k_event_queue_capacity - 1u;
		static_assert((k_event_queue_capacity & (k_event_queue_capacity - 1u)) == 0u);

		static void OnRawKeyboardInput(
			const nox::os::RawKeyboardInputEvent& event,
			void* user_data)noexcept;
		static nox::hid::KeyCode MapRawKey(const nox::os::RawKeyboardInputEvent& event)noexcept;

		void Update(nox::World& world);
		std::span<const nox::SystemBase::PhaseRegister> GetPhaseRegisterList()const noexcept override;
		bool TryPush(const QueuedInputEvent& event)noexcept;
		bool TryPop(QueuedInputEvent& event)noexcept;
		void MarkQueueOverflow()noexcept;

	public:
		/// @brief 同じUpdateフェーズでキー状態を使うSystemは、このフェーズへの依存を登録します
		static constexpr SystemPhaseUpdate k_phase_update
		{
			&KeyboardManager::Update,
			u8"KeyboardManager::Update"
		};

	private:
		static nox::hid::KeyboardManager* instance_;

		nox::hid::Keyboard keyboard_;
		std::array<QueuedInputEvent, k_event_queue_capacity> event_queue_{};
		alignas(64) std::atomic<nox::uint32> write_index_{ 0u };
		alignas(64) std::atomic<nox::uint32> read_index_{ 0u };
		std::atomic<bool> queue_overflowed_{ false };
		std::atomic<bool> producer_resync_requested_{ false };
		std::array<bool, 256u> producer_key_state_{};
	};
}
