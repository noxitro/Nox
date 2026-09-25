//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	keyboard_manager.cpp
///	@brief	keyboard_manager
#include	"pch.h"
#include	"keyboard_manager.h"

nox::hid::KeyboardManager* nox::hid::KeyboardManager::instance_ = nullptr;

nox::hid::KeyboardManager::KeyboardManager()noexcept
{
	if (nox::hid::KeyboardManager::instance_ != nullptr)
	{
		NOX_ASSERT(false, u8"KeyboardManagerは1つだけ生成できます");
		return;
	}

	nox::hid::KeyboardManager::instance_ = this;
	nox::os::SetRawKeyboardInputCallback(&nox::hid::KeyboardManager::OnRawKeyboardInput, this);
}

nox::hid::KeyboardManager::~KeyboardManager()
{
	if (nox::hid::KeyboardManager::instance_ == this)
	{
		nox::os::SetRawKeyboardInputCallback(nullptr, nullptr);
		nox::hid::KeyboardManager::instance_ = nullptr;
	}
}

nox::hid::KeyboardManager* nox::hid::KeyboardManager::GetInstance()noexcept
{
	return nox::hid::KeyboardManager::instance_;
}

const nox::hid::Keyboard& nox::hid::KeyboardManager::GetKeyboard()const noexcept
{
	return nox::hid::KeyboardManager::keyboard_;
}

void nox::hid::KeyboardManager::OnRawKeyboardInput(
	const nox::os::RawKeyboardInputEvent& event,
	void* const user_data)noexcept
{
	auto* const manager = static_cast<nox::hid::KeyboardManager*>(user_data);
	if (manager == nullptr)
	{
		return;
	}

	if (manager->producer_resync_requested_.exchange(false, std::memory_order_acq_rel) == true)
	{
		manager->producer_key_state_.fill(false);
	}

	if (event.type == nox::os::RawKeyboardInputType::FocusLost)
	{
		manager->producer_key_state_.fill(false);
		if (manager->TryPush({ nox::hid::KeyCode::Unknown, false, true }) == false)
		{
			manager->MarkQueueOverflow();
		}
		return;
	}

	const nox::hid::KeyCode key_code = nox::hid::KeyboardManager::MapRawKey(event);
	if (key_code == nox::hid::KeyCode::Unknown)
	{
		return;
	}

	const std::size_t key_index = static_cast<nox::uint8>(key_code);
	const bool is_down = (event.type == nox::os::RawKeyboardInputType::KeyDown);
	if (manager->producer_key_state_[key_index] == is_down)
	{
		return;
	}

	manager->producer_key_state_[key_index] = is_down;
	if (manager->TryPush({ key_code, is_down, false }) == false)
	{
		manager->MarkQueueOverflow();
	}
}

nox::hid::KeyCode nox::hid::KeyboardManager::MapRawKey(
	const nox::os::RawKeyboardInputEvent& event)noexcept
{
	switch (event.virtual_key)
	{
	case 0x15u: return nox::hid::KeyCode::International2;
	case 0x19u: return nox::hid::KeyCode::Language2;
	case 0x1Cu: return nox::hid::KeyCode::International4;
	case 0x1Du: return nox::hid::KeyCode::International5;
	case 0xADu: return nox::hid::KeyCode::Mute;
	case 0xAEu: return nox::hid::KeyCode::VolumeDown;
	case 0xAFu: return nox::hid::KeyCode::VolumeUp;
	case 0xE2u: return nox::hid::KeyCode::NonUsBackslash;
	default: break;
	}

	if (event.virtual_key >= 0x70u && event.virtual_key <= 0x7Bu)
	{
		const nox::uint8 f_key_usage = static_cast<nox::uint8>(
			static_cast<nox::uint8>(nox::hid::KeyCode::F1) + (event.virtual_key - 0x70u));
		return static_cast<nox::hid::KeyCode>(f_key_usage);
	}
	if (event.virtual_key >= 0x7Cu && event.virtual_key <= 0x87u)
	{
		const nox::uint8 f_key_usage = static_cast<nox::uint8>(
			static_cast<nox::uint8>(nox::hid::KeyCode::F13) + (event.virtual_key - 0x7Cu));
		return static_cast<nox::hid::KeyCode>(f_key_usage);
	}

	if (event.is_extended1 == true)
	{
		return (event.make_code == 0x45u) ? nox::hid::KeyCode::Pause : nox::hid::KeyCode::Unknown;
	}

	if (event.is_extended == true)
	{
		switch (event.make_code)
		{
		case 0x1Cu: return nox::hid::KeyCode::KeypadEnter;
		case 0x1Du: return nox::hid::KeyCode::RightControl;
		case 0x35u: return nox::hid::KeyCode::KeypadDivide;
		case 0x37u: return nox::hid::KeyCode::PrintScreen;
		case 0x38u: return nox::hid::KeyCode::RightAlt;
		case 0x47u: return nox::hid::KeyCode::Home;
		case 0x48u: return nox::hid::KeyCode::UpArrow;
		case 0x49u: return nox::hid::KeyCode::PageUp;
		case 0x4Bu: return nox::hid::KeyCode::LeftArrow;
		case 0x4Du: return nox::hid::KeyCode::RightArrow;
		case 0x4Fu: return nox::hid::KeyCode::End;
		case 0x50u: return nox::hid::KeyCode::DownArrow;
		case 0x51u: return nox::hid::KeyCode::PageDown;
		case 0x52u: return nox::hid::KeyCode::Insert;
		case 0x53u: return nox::hid::KeyCode::Delete;
		case 0x5Bu: return nox::hid::KeyCode::LeftGui;
		case 0x5Cu: return nox::hid::KeyCode::RightGui;
		case 0x5Du: return nox::hid::KeyCode::Application;
		case 0x5Eu: return nox::hid::KeyCode::Power;
		default: return nox::hid::KeyCode::Unknown;
		}
	}

	switch (event.make_code)
	{
	case 0x01u: return nox::hid::KeyCode::Escape;
	case 0x02u: return nox::hid::KeyCode::Digit1;
	case 0x03u: return nox::hid::KeyCode::Digit2;
	case 0x04u: return nox::hid::KeyCode::Digit3;
	case 0x05u: return nox::hid::KeyCode::Digit4;
	case 0x06u: return nox::hid::KeyCode::Digit5;
	case 0x07u: return nox::hid::KeyCode::Digit6;
	case 0x08u: return nox::hid::KeyCode::Digit7;
	case 0x09u: return nox::hid::KeyCode::Digit8;
	case 0x0Au: return nox::hid::KeyCode::Digit9;
	case 0x0Bu: return nox::hid::KeyCode::Digit0;
	case 0x0Cu: return nox::hid::KeyCode::Minus;
	case 0x0Du: return nox::hid::KeyCode::Equal;
	case 0x0Eu: return nox::hid::KeyCode::Backspace;
	case 0x0Fu: return nox::hid::KeyCode::Tab;
	case 0x10u: return nox::hid::KeyCode::Q;
	case 0x11u: return nox::hid::KeyCode::W;
	case 0x12u: return nox::hid::KeyCode::E;
	case 0x13u: return nox::hid::KeyCode::R;
	case 0x14u: return nox::hid::KeyCode::T;
	case 0x15u: return nox::hid::KeyCode::Y;
	case 0x16u: return nox::hid::KeyCode::U;
	case 0x17u: return nox::hid::KeyCode::I;
	case 0x18u: return nox::hid::KeyCode::O;
	case 0x19u: return nox::hid::KeyCode::P;
	case 0x1Au: return nox::hid::KeyCode::LeftBracket;
	case 0x1Bu: return nox::hid::KeyCode::RightBracket;
	case 0x1Cu: return nox::hid::KeyCode::Enter;
	case 0x1Du: return nox::hid::KeyCode::LeftControl;
	case 0x1Eu: return nox::hid::KeyCode::A;
	case 0x1Fu: return nox::hid::KeyCode::S;
	case 0x20u: return nox::hid::KeyCode::D;
	case 0x21u: return nox::hid::KeyCode::F;
	case 0x22u: return nox::hid::KeyCode::G;
	case 0x23u: return nox::hid::KeyCode::H;
	case 0x24u: return nox::hid::KeyCode::J;
	case 0x25u: return nox::hid::KeyCode::K;
	case 0x26u: return nox::hid::KeyCode::L;
	case 0x27u: return nox::hid::KeyCode::Semicolon;
	case 0x28u: return nox::hid::KeyCode::Apostrophe;
	case 0x29u: return nox::hid::KeyCode::Grave;
	case 0x2Au: return nox::hid::KeyCode::LeftShift;
	case 0x2Bu: return nox::hid::KeyCode::Backslash;
	case 0x2Cu: return nox::hid::KeyCode::Z;
	case 0x2Du: return nox::hid::KeyCode::X;
	case 0x2Eu: return nox::hid::KeyCode::C;
	case 0x2Fu: return nox::hid::KeyCode::V;
	case 0x30u: return nox::hid::KeyCode::B;
	case 0x31u: return nox::hid::KeyCode::N;
	case 0x32u: return nox::hid::KeyCode::M;
	case 0x33u: return nox::hid::KeyCode::Comma;
	case 0x34u: return nox::hid::KeyCode::Period;
	case 0x35u: return nox::hid::KeyCode::Slash;
	case 0x36u: return nox::hid::KeyCode::RightShift;
	case 0x37u: return nox::hid::KeyCode::KeypadMultiply;
	case 0x38u: return nox::hid::KeyCode::LeftAlt;
	case 0x39u: return nox::hid::KeyCode::Space;
	case 0x3Au: return nox::hid::KeyCode::CapsLock;
	case 0x3Bu: return nox::hid::KeyCode::F1;
	case 0x3Cu: return nox::hid::KeyCode::F2;
	case 0x3Du: return nox::hid::KeyCode::F3;
	case 0x3Eu: return nox::hid::KeyCode::F4;
	case 0x3Fu: return nox::hid::KeyCode::F5;
	case 0x40u: return nox::hid::KeyCode::F6;
	case 0x41u: return nox::hid::KeyCode::F7;
	case 0x42u: return nox::hid::KeyCode::F8;
	case 0x43u: return nox::hid::KeyCode::F9;
	case 0x44u: return nox::hid::KeyCode::F10;
	case 0x45u: return nox::hid::KeyCode::NumLock;
	case 0x46u: return nox::hid::KeyCode::ScrollLock;
	case 0x47u: return nox::hid::KeyCode::Keypad7;
	case 0x48u: return nox::hid::KeyCode::Keypad8;
	case 0x49u: return nox::hid::KeyCode::Keypad9;
	case 0x4Au: return nox::hid::KeyCode::KeypadSubtract;
	case 0x4Bu: return nox::hid::KeyCode::Keypad4;
	case 0x4Cu: return nox::hid::KeyCode::Keypad5;
	case 0x4Du: return nox::hid::KeyCode::Keypad6;
	case 0x4Eu: return nox::hid::KeyCode::KeypadAdd;
	case 0x4Fu: return nox::hid::KeyCode::Keypad1;
	case 0x50u: return nox::hid::KeyCode::Keypad2;
	case 0x51u: return nox::hid::KeyCode::Keypad3;
	case 0x52u: return nox::hid::KeyCode::Keypad0;
	case 0x53u: return nox::hid::KeyCode::KeypadDecimal;
	case 0x56u: return nox::hid::KeyCode::NonUsHash;
	case 0x57u: return nox::hid::KeyCode::F11;
	case 0x58u: return nox::hid::KeyCode::F12;
	default: return nox::hid::KeyCode::Unknown;
	}
}

void nox::hid::KeyboardManager::Update([[maybe_unused]] nox::World& world)
{
	nox::hid::KeyboardManager::keyboard_.BeginFrame();

	nox::hid::KeyboardManager::QueuedInputEvent event{};
	while (nox::hid::KeyboardManager::TryPop(event) == true)
	{
		if (event.release_all == true)
		{
			nox::hid::KeyboardManager::keyboard_.ReleaseAllKeys();
		}
		else
		{
			nox::hid::KeyboardManager::keyboard_.ProcessKeyEvent(event.key_code, event.is_down);
		}
	}

	if (nox::hid::KeyboardManager::queue_overflowed_.exchange(false, std::memory_order_acq_rel) == true)
	{
		nox::hid::KeyboardManager::keyboard_.ReleaseAllKeys();
		while (nox::hid::KeyboardManager::TryPop(event) == true)
		{
		}
		nox::hid::KeyboardManager::producer_resync_requested_.store(true, std::memory_order_release);
		NOX_ASSERT(false, u8"キーボード入力イベントキューが溢れたため、押下状態をリセットしました");
	}
}

std::span<const nox::SystemBase::PhaseRegister> nox::hid::KeyboardManager::GetPhaseRegisterList()const noexcept
{
	static constexpr auto table = {
		PhaseRegister(nox::hid::KeyboardManager::k_phase_update)
	};
	return table;
}

bool nox::hid::KeyboardManager::TryPush(const nox::hid::KeyboardManager::QueuedInputEvent& event)noexcept
{
	if (nox::hid::KeyboardManager::queue_overflowed_.load(std::memory_order_acquire) == true)
	{
		return false;
	}

	const nox::uint32 write_index = nox::hid::KeyboardManager::write_index_.load(std::memory_order_relaxed);
	const nox::uint32 next_write_index = (write_index + 1u) & nox::hid::KeyboardManager::k_event_queue_mask;
	if (next_write_index == nox::hid::KeyboardManager::read_index_.load(std::memory_order_acquire))
	{
		return false;
	}

	nox::hid::KeyboardManager::event_queue_[write_index] = event;
	nox::hid::KeyboardManager::write_index_.store(next_write_index, std::memory_order_release);
	return true;
}

bool nox::hid::KeyboardManager::TryPop(nox::hid::KeyboardManager::QueuedInputEvent& event)noexcept
{
	const nox::uint32 read_index = nox::hid::KeyboardManager::read_index_.load(std::memory_order_relaxed);
	if (read_index == nox::hid::KeyboardManager::write_index_.load(std::memory_order_acquire))
	{
		return false;
	}

	event = nox::hid::KeyboardManager::event_queue_[read_index];
	nox::hid::KeyboardManager::read_index_.store(
		(read_index + 1u) & nox::hid::KeyboardManager::k_event_queue_mask,
		std::memory_order_release);
	return true;
}

void nox::hid::KeyboardManager::MarkQueueOverflow()noexcept
{
	nox::hid::KeyboardManager::queue_overflowed_.store(true, std::memory_order_release);
	nox::hid::KeyboardManager::producer_resync_requested_.store(true, std::memory_order_release);
}
