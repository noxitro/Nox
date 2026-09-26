//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	keyboard_manager.cpp
///	@brief	keyboard_manager
#include	"pch.h"
#include	"keyboard_manager.h"

#include	"../../kernel/os/window.h"

namespace
{
	static_assert(nox::hid::kRawKeyBreak == RI_KEY_BREAK);
	static_assert(nox::hid::kRawKeyE0 == RI_KEY_E0);
	static_assert(nox::hid::kRawKeyE1 == RI_KEY_E1);

	/// @brief Raw Input でキーボードを受け取る登録 (Generic Desktop / Keyboard)
	/// @details 従来のキーメッセージは WM_CHAR / IME のために止めない (RIDEV_NOLEGACY を付けない)。
	///			hwndTarget が null なので、キーボードフォーカスを持つウィンドウへ届く
	[[nodiscard]] ::RAWINPUTDEVICE MakeKeyboardDevice(const ::DWORD flags)noexcept
	{
		return ::RAWINPUTDEVICE
		{
			.usUsagePage = 0x01,
			.usUsage = 0x06,
			.dwFlags = flags,
			.hwndTarget = nullptr
		};
	}

#if NOX_DEVELOP
	/// @brief	受け取った RAWKEYBOARD の値をデバッグ出力へ書き出す
	/// @details	hid_test のテーブルへそのまま貼れるよう { MakeCode, Flags, VKey } の形で出す
	void LogRawKeyboardInput(const nox::uint16 make_code, const nox::uint16 flags, const nox::uint16 virtual_key)noexcept
	{
		wchar_t line[] = L"RawKeyboard { 0x0000, 0x0000, 0x0000 }\n";
		constexpr std::size_t kDigitOffsets[] = { 16u, 24u, 32u };
		const nox::uint16 values[] = { make_code, flags, virtual_key };
		for (std::size_t value_index = 0u; value_index < 3u; ++value_index)
		{
			for (std::size_t digit_index = 0u; digit_index < 4u; ++digit_index)
			{
				const nox::uint32 nibble = (static_cast<nox::uint32>(values[value_index]) >> (12u - (digit_index * 4u))) & 0xFu;
				line[kDigitOffsets[value_index] + digit_index] =
					static_cast<wchar_t>((nibble < 10u) ? (L'0' + nibble) : (L'A' + (nibble - 10u)));
			}
		}
		::OutputDebugStringW(line);
	}
#endif // NOX_DEVELOP
}

nox::hid::KeyboardManager* nox::hid::KeyboardManager::instance_ = nullptr;

nox::hid::KeyboardManager::KeyboardManager()noexcept
{
	if (instance_ != nullptr)
	{
		NOX_ASSERT(false, u8"KeyboardManagerは1つだけ生成できます");
		return;
	}

	instance_ = this;

	//	キー状態は、登録できれば WM_INPUT だけから作り、失敗したときだけ従来のキーメッセージで代替する
	const ::RAWINPUTDEVICE keyboard_device = MakeKeyboardDevice(0u);
	is_raw_input_registered_ =
		(::RegisterRawInputDevices(&keyboard_device, 1u, static_cast<::UINT>(sizeof(::RAWINPUTDEVICE))) != FALSE);
	if (is_raw_input_registered_ == false)
	{
		::OutputDebugStringW(L"RegisterRawInputDevices に失敗したため、従来のキーメッセージで代替します\n");
	}

#if NOX_DEVELOP
	is_raw_input_log_enabled_ = nox::os::ContainsCommandLineArgKey(u"--log-raw-keyboard");
#endif // NOX_DEVELOP

	[[maybe_unused]] const bool is_hook_added =
		nox::os::AddWindowMessageHook(&nox::hid::KeyboardManager::OnWindowMessage, this);
}

nox::hid::KeyboardManager::~KeyboardManager()
{
	if (instance_ != this)
	{
		return;
	}

	nox::os::RemoveWindowMessageHook(&nox::hid::KeyboardManager::OnWindowMessage, this);
	if (is_raw_input_registered_ == true)
	{
		const ::RAWINPUTDEVICE keyboard_device = MakeKeyboardDevice(RIDEV_REMOVE);
		::RegisterRawInputDevices(&keyboard_device, 1u, static_cast<::UINT>(sizeof(::RAWINPUTDEVICE)));
	}
	instance_ = nullptr;
}

nox::hid::KeyboardManager* nox::hid::KeyboardManager::GetInstance()noexcept
{
	return instance_;
}

const nox::hid::Keyboard& nox::hid::KeyboardManager::GetKeyboard()const noexcept
{
	return keyboard_;
}

void nox::hid::KeyboardManager::OnWindowMessage(const nox::os::WindowMessage& message, void* const user_data)noexcept
{
	auto* const manager = static_cast<nox::hid::KeyboardManager*>(user_data);
	if (manager == nullptr)
	{
		return;
	}

	switch (message.message)
	{
	case WM_INPUT:
		manager->ProduceRawInput(message.lparam);
		break;

	case WM_KILLFOCUS:
		manager->ProduceFocusLost();
		break;

	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYUP:
		//	Raw Input を登録できなかったときだけの代替。登録済みなら同じ変化が WM_INPUT でも届くうえ、
		//	OS が合成したメッセージ (AltGr の偽 LCtrl など) も混ざるので使わない。
		if (manager->is_raw_input_registered_ == false)
		{
			const bool is_down = (message.message == WM_KEYDOWN) || (message.message == WM_SYSKEYDOWN);
			manager->ProduceKeyInput(nox::hid::TranslateLegacyKeyMessage(
				is_down,
				static_cast<nox::uint16>(message.wparam),
				static_cast<nox::uint32>(message.lparam)));
		}
		break;

	default:
		break;
	}
}

void nox::hid::KeyboardManager::ProduceRawInput(const nox::int64 raw_input_handle)noexcept
{
	::RAWINPUT raw_input{};
	::UINT raw_input_buffer_size = static_cast<::UINT>(sizeof(raw_input));
	const ::UINT raw_input_size = ::GetRawInputData(
		reinterpret_cast<::HRAWINPUT>(raw_input_handle),
		RID_INPUT,
		&raw_input,
		&raw_input_buffer_size,
		static_cast<::UINT>(sizeof(::RAWINPUTHEADER)));

	//	キーボード以外の Raw Input (他のモジュールが登録したもの) もここへ来る。
	//	RAWINPUT に収まらない HID の入力は -1 で返るので、それも含めて黙って無視する
	constexpr ::UINT kMinKeyboardInputSize =
		static_cast<::UINT>(sizeof(::RAWINPUTHEADER) + sizeof(::RAWKEYBOARD));
	if (raw_input_size == static_cast<::UINT>(-1) ||
		raw_input.header.dwType != RIM_TYPEKEYBOARD ||
		raw_input_size < kMinKeyboardInputSize)
	{
		return;
	}

	const ::RAWKEYBOARD& keyboard = raw_input.data.keyboard;
#if NOX_DEVELOP
	if (is_raw_input_log_enabled_ == true)
	{
		LogRawKeyboardInput(keyboard.MakeCode, keyboard.Flags, keyboard.VKey);
	}
#endif // NOX_DEVELOP

	nox::hid::RawKeyboardInput input{};
	if (nox::hid::TranslateRawKeyboardInput(
		keyboard.MakeCode,
		keyboard.Flags,
		keyboard.VKey,
		producer_translate_state_,
		input) == true)
	{
		ProduceKeyInput(input);
	}
}

void nox::hid::KeyboardManager::ProduceKeyInput(const nox::hid::RawKeyboardInput& input)noexcept
{
	if (TryFlushResync() == false)
	{
		return;
	}

	const nox::hid::KeyCode key_code = nox::hid::MapKeyCode(input);
	if (key_code == nox::hid::KeyCode::Unknown)
	{
		return;
	}

	const std::size_t key_index = static_cast<nox::uint8>(key_code);
	if (producer_key_state_[key_index] == input.is_down)
	{
		return;
	}

	if (TryPush({ QueuedEventType::Key, key_code, input.is_down }) == false)
	{
		producer_resync_pending_ = true;
		return;
	}
	producer_key_state_[key_index] = input.is_down;
}

void nox::hid::KeyboardManager::ProduceFocusLost()noexcept
{
	//	Resync を積めなければ、積めた時点で全キーが離れるので同じ結果になる
	if (TryFlushResync() == false)
	{
		return;
	}

	if (TryPushReleaseAll(QueuedEventType::ReleaseAll) == false)
	{
		producer_resync_pending_ = true;
	}
}

bool nox::hid::KeyboardManager::TryFlushResync()noexcept
{
	//	取りこぼした後は、全キーを離す Resync をキューへ積めるまで他の入力を捨てる。
	//	キュー上の位置と producer_key_state_ のクリアが一致するので、Update 側と食い違わない。
	if (producer_resync_pending_ == false)
	{
		return true;
	}

	if (TryPushReleaseAll(QueuedEventType::Resync) == false)
	{
		return false;
	}
	producer_resync_pending_ = false;
	return true;
}

void nox::hid::KeyboardManager::Update([[maybe_unused]] nox::World& world)
{
	keyboard_.BeginFrame();

	QueuedInputEvent event{};
	while (TryPop(event) == true)
	{
		switch (event.type)
		{
		case QueuedEventType::Key:
			keyboard_.ProcessKeyEvent(event.key_code, event.is_down);
			break;

		case QueuedEventType::ReleaseAll:
			keyboard_.ReleaseAllKeys();
			break;

		case QueuedEventType::Resync:
			NOX_ASSERT(false, u8"キーボード入力イベントキューが溢れたため、押下状態をリセットしました");
			keyboard_.ReleaseAllKeys();
			break;
		}
	}
}

std::span<const nox::SystemBase::PhaseRegister> nox::hid::KeyboardManager::GetPhaseRegisterList()const noexcept
{
	static constexpr auto table = {
		PhaseRegister(nox::hid::KeyboardManager::kPhaseUpdate)
	};
	return table;
}

bool nox::hid::KeyboardManager::TryPush(const QueuedInputEvent& event)noexcept
{
	const nox::uint32 write_index = write_index_.load(std::memory_order_relaxed);
	const nox::uint32 next_write_index = (write_index + 1u) & kEventQueueMask;
	if (next_write_index == read_index_.load(std::memory_order_acquire))
	{
		return false;
	}

	event_queue_[write_index] = event;
	write_index_.store(next_write_index, std::memory_order_release);
	return true;
}

bool nox::hid::KeyboardManager::TryPop(QueuedInputEvent& event)noexcept
{
	const nox::uint32 read_index = read_index_.load(std::memory_order_relaxed);
	if (read_index == write_index_.load(std::memory_order_acquire))
	{
		return false;
	}

	event = event_queue_[read_index];
	read_index_.store((read_index + 1u) & kEventQueueMask, std::memory_order_release);
	return true;
}

bool nox::hid::KeyboardManager::TryPushReleaseAll(const QueuedEventType type)noexcept
{
	if (TryPush({ type, nox::hid::KeyCode::Unknown, false }) == false)
	{
		return false;
	}
	producer_key_state_.fill(false);
	return true;
}
