//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	os.h
///	@brief	os
#pragma once
#include	"../advanced_type.h"

#include	"../nox_string.h"
#include	"../nox_string_view.h"

namespace nox::os
{
	enum class RawKeyboardInputType : nox::uint8
	{
		KeyDown,
		KeyUp,
		FocusLost
	};

	/// @brief キーボード入力 1 件
	/// @details make_code は Raw Input と同じ表現 (Set 1 の make code。E0 / E1 接頭辞はフラグで持つ)。
	///			従来のキーメッセージから作る場合もこの表現へ揃える。
	struct RawKeyboardInputEvent
	{
		RawKeyboardInputType type;
		nox::uint16 make_code;
		nox::uint16 virtual_key;
		bool is_extended;
		bool is_extended1;
	};

	using RawKeyboardInputCallback = void(*)(const RawKeyboardInputEvent&, void*)noexcept;

	/// @brief コールバックはウィンドウメッセージを処理するスレッド上で実行されます
	/// @details メッセージポンプ開始前に登録し、ポンプ終了後に解除します
	void SetRawKeyboardInputCallback(RawKeyboardInputCallback callback, void* user_data)noexcept;

	namespace detail
	{
		void DispatchRawKeyboardInput(const RawKeyboardInputEvent& event)noexcept;

		/// @brief Raw Input へキーボードを登録できたか
		/// @details 登録できていれば、従来のキーメッセージはコールバックへ送らない
		[[nodiscard]] bool IsRawKeyboardInputRegistered()noexcept;

		/// @brief 受け取った Raw Input キーボード入力をデバッグ出力へ書き出すか
		/// @details 開発ビルドで起動引数に --log-raw-keyboard を付けたときだけ true
		[[nodiscard]] bool IsRawKeyboardInputLogEnabled()noexcept;

		/// @brief RAWKEYBOARD::Flags のビット。RI_KEY_BREAK / RI_KEY_E0 / RI_KEY_E1 と同じ値
		inline constexpr nox::uint16 kRawKeyBreak = 0x01u;
		inline constexpr nox::uint16 kRawKeyE0 = 0x02u;
		inline constexpr nox::uint16 kRawKeyE1 = 0x04u;

		/// @brief Raw Input のキーボード入力を変換するときに、次の入力へ持ち越す状態
		struct RawKeyboardTranslateState
		{
			/// @brief 直前の入力に E1 が付いていたか (Pause の後半を見分ける)
			bool is_e1_pending = false;
		};

		/// @brief RAWKEYBOARD の値から入力を作る
		/// @details Pause は E1 1D と 45 の 2 件に分かれて届く。後半の 45 は NumLock と同じ値なので送らない
		/// @return 送る入力があれば true
		[[nodiscard]] bool TranslateRawKeyboardInput(
			nox::uint16 make_code,
			nox::uint16 flags,
			nox::uint16 virtual_key,
			RawKeyboardTranslateState& state,
			RawKeyboardInputEvent& out)noexcept;

		/// @brief 従来のキーメッセージ (WM_KEYDOWN など) から入力を作る
		/// @details Pause と NumLock はどちらも 0x45 で届くので、Raw Input の表現へ揃える
		/// @param key_data lParam の下位 32 ビット
		[[nodiscard]] RawKeyboardInputEvent TranslateLegacyKeyMessage(
			bool is_down,
			nox::uint16 virtual_key,
			nox::uint32 key_data)noexcept;
	}

	struct ProcessMemoryInfo
	{
		uint32 cb;
		uint32 page_fault_count;
		size_t peak_working_set_size;
		size_t working_set_size;
		size_t quota_peak_paged_pool_usage;
		size_t quota_paged_pool_usage;
		size_t quota_peak_non_paged_pool_usage;
		size_t quota_non_paged_pool_usage;
		size_t pagefile_usage;
		size_t peak_pagefile_usage;
		size_t private_usage;
		size_t private_working_set_size;
		uint64 shared_commit_usage;
	};

	/// @brief 初期化
	/// @param args 引数
	void	Initialize(const std::span<const char16* const> args);

	/// @brief 
	/// @return アプリケーション終了
	bool	Update();

	/// @brief 終了処理
	void	Finalize();

	/// @brief 指定のコマンドライン引数を取得します
	/// @param index 
	/// @return 
	std::span<const char16* const> GetCommandLineArgList()noexcept;

	inline std::u16string_view GetCommandLineArg(nox::uint32 index)
	{
		return nox::os::GetCommandLineArgList()[index];
	}

	///	@brief		指定した引数列から、キーに対応する値を取り出す。
	///	@details	キーの直後が区切り文字 ('=' または ':') か終端のときだけ一致とみなす。
	///				「キーで始まる」だけの判定にすると --foo が --foobar にも一致してしまうため。
	///
	///				返る値に区切り文字は含めない。--foo=bar なら "bar"。
	///				--foo のように値が無い場合は空文字列を返す (キーは在ったので nullopt にはしない)。
	///				キーが見つからなければ nullopt。
	///
	///				プロセスの実引数に依存しないので、この形はテストから直接叩ける。
	///	@param		command_line_args	走査する引数列。nullptr 要素は読み飛ばす
	///	@param		key					探すキー。区切り文字は含めない
	[[nodiscard]] std::optional<std::u16string_view> TryGetCommandLineArgValue(
		std::span<const nox::char16* const> command_line_args,
		std::u16string_view key)noexcept;

	///	@brief		指定した引数列にキーが存在するか。判定規則は TryGetCommandLineArgValue と同じ。
	[[nodiscard]] bool ContainsCommandLineArgKey(
		std::span<const nox::char16* const> command_line_args,
		std::u16string_view arg)noexcept;

	/// @brief		コマンドライン引数本体、または=で区切られたコマンドライン引数のキーが存在するか
	/// @details	例：コマンドライン引数が「--foo=bar」の場合、ContainsCommandLineArgKey(u"--foo")はtrueを返す
	bool ContainsCommandLineArgKey(std::u16string_view arg)noexcept;

	/// @brief		=で区切られたコマンドライン引数の値を取得します
	/// @details	区切り文字は値に含めない。判定規則は TryGetCommandLineArgValue を参照。
	/// @param key
	std::optional<std::u16string_view> GetCommandLineArgValue(std::u16string_view key)noexcept;

	/// @brief		ソリューションディレクトリを取得します
	/// @details	デバッグ用途
	/// @return 
	inline std::u16string_view GetSolutionDir()noexcept
	{
		return nox::os::GetCommandLineArgValue(u"--solution-dir").value_or(u"");
	}

	StlU16String	GetDirectoryUTF8();

	nox::U16String	GetDirectory();
	std::u16string_view	GetDirectory(std::span<nox::char16> dest_buffer);

	ProcessMemoryInfo GetCurrentProcessMemoryInfo();

	void Sleep(nox::uint32 milliseconds);

	namespace detail
	{
		void DispatchCreateNativeWindow(void(*func)(const void*), const void* arg);
	}
}