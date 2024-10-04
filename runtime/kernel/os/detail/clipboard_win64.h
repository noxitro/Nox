//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	clipboard_win64.h
///	@brief	clipboard_win64
#pragma once
#include	"clipboard_base.h"
#include	"basic_definition.h"
//#include	"../../advanced_type.h"
#include	"../../nox_string.h"
#include	"../../nox_string_view.h"

#if NOX_WIN64
namespace nox::os::clipboard
{
	bool Clear();
	bool SetText(const nox::StringView text);
	bool SetText(const std::u8string_view text);

	std::optional<nox::String> GetText();
	std::optional<nox::StringView> GetText(std::span<nox::char32> dest_buffer);
}

namespace nox::os::detail
{
	class ClipboardWin64 : public ClipboardBase
	{
	public:
		/// @brief 空にする
		/// @return 正常に完了したか
		static bool Clear();


		static bool SetText(const nox::StringView text);
	};
}
#endif