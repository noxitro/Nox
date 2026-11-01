// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

///	@file	assertion.cpp
///	@brief	assertion
#include	"assertion.h"
#include	"pch.h"

#include	<cassert>
#include	<cstdlib>
#include	<stacktrace>

#include	"memory/stl_allocate_adapter.h"
#include	"preprocessor/util.h"
#include	"unicode_converter.h"

#include	"algorithm.h"
#include	"stack_trace.h"
#include	"string_format.h"
#include	"mutex.h"
#include	"scoped_lock.h"
#include	"os_definition.h"

namespace nox::detail
{
	namespace
	{
		/// @brief		アサート表示を直列化するロック
		/// @details	NOX_ASSERTはアロケータ内部を含めプロセスのどの時点でも発火し得るため、
		///				動的初期化が必要なnox::os::Mutexでは静的初期化中のアサートが
		///				アサート機構そのものの中でアクセス違反になっていた。
		///				MEMO:	ロック区間は::_wassertの呼び出しだけで、
		///						そこからnox側のコード(NOX_ASSERT/ログ/スタック採取)へは
		///						戻ってこないため、非再帰ロックで問題ない。
		///						スタック採取とメッセージ整形はロック取得より手前で終えている。
		constinit nox::Mutex kMutex;

		inline constexpr bool is_high_surrogate(const nox::char16 c) { return (c >= 0xD800) && (c < 0xDC00); }

		inline constexpr bool is_low_surrogate(const nox::char16 c) { return (c >= 0xDC00) && (c < 0xE000); }
	}
}

void	nox::detail::Assert(const nox::assertion::ErrorId id, std::u8string_view message, const std::source_location& source_location)
{
	nox::stack_walker::StackWalkerSlim stack_walker;
	stack_walker.Collect(1);

	//	Formatの引数変換バッファ(512文字)へ収まらないので、ここで変換しておく
	std::array<nox::char16, 64> native_id = { 0 };
	const std::u16string_view id_view = nox::unicode::ConvertU16String(id.value, native_id);

	std::array<nox::char16, 2048> native_message = { 0 };
	const std::u16string_view message_view = nox::unicode::ConvertU16String(message, native_message);

	std::array<nox::char16, nox::k_max_path_length> file_path = { 0 };
	const std::string_view file_name = std::string_view{ source_location.file_name() }.substr(0, file_path.size() - 1);
	const std::u16string_view file_path_view = nox::unicode::ConvertU16String(file_name, file_path);

	std::array<nox::char16, 4096> assert_message = { 0 };
	nox::util::Format(assert_message, u"{0}\n{1}\n{2}\nLine:{3}, Column:{4}",
		id_view,
		message_view,
		file_path_view,
		source_location.line(),
		source_location.column()
	);

	NOX_LOCAL_SCOPE(nox::ScopedLock{ nox::detail::kMutex });

#if NOX_WINDOWS
	::_wassert(
		nox::util::CharCast<wchar16>(assert_message.data()),
		nox::util::CharCast<wchar16>(file_path.data()),
		source_location.line()
	);
#else
	std::abort();
#endif // NOX_WINDOWS
}
