//	Copyright (c) 2026 NOX ENGINE All rights reserved.

///	@file	path.cpp
///	@brief	path
#include	"stdafx.h"
#include	"path.h"

std::span<nox::U8StringView> nox::io::Path::GetExtensions()const noexcept
{
	// thread_local ストレージに結果を保持（メンバ変数なし）
	static constexpr nox::uint8 k_max_extensions = 8;
	thread_local std::array<nox::U8StringView, k_max_extensions> tl_ext_views{};
	thread_local nox::uint8 tl_ext_count = 0;

	tl_ext_count = 0;

	const std::u8string_view full_path(buffer_.data());

	// 最後のパスセパレータ以降のファイル名部分を取得
	const auto sep_pos = full_path.find_last_of(u8"/\\");
	const std::u8string_view filename = (sep_pos == std::u8string_view::npos)
		? full_path
		: full_path.substr(sep_pos + 1);

	if (filename.empty())
	{
		return {};
	}

	// ".gitignore" のような先頭ドットはステムの一部なのでスキップ
	const std::size_t search_start = (filename[0] == u8'.') ? 1 : 0;

	std::size_t dot_pos = filename.find(u8'.', search_start);
	while (dot_pos != std::u8string_view::npos && tl_ext_count < k_max_extensions)
	{
		const std::size_t next_dot = filename.find(u8'.', dot_pos + 1);
		const std::size_t ext_len = (next_dot != std::u8string_view::npos)
			? next_dot - dot_pos
			: filename.size() - dot_pos;

		// filename.data() は buffer_.data() への内部ポインタなので安全なビュー
		tl_ext_views[tl_ext_count] = std::u8string_view(filename.data() + dot_pos, ext_len);
		++tl_ext_count;
		dot_pos = next_dot;
	}

	return std::span<nox::U8StringView>(tl_ext_views.data(), tl_ext_count);
}
