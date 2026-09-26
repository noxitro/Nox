//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	memory_profile.h
///	@brief	メモリプロファイラ
#pragma once
#include	"memory_definition.h"
#include	"../stack_trace_definition.h"
#include	"basic_definition.h"

namespace nox::memory::profile
{
	/// @brief プロファイラの管理データ
	struct ProfileData
	{
		/// @brief プロファイラの管理ハンドル
		nox::uint16 handle;

		/// @brief コールスタックのアドレス
		std::array<std::size_t, nox::stack_walker::DEFAULT_STACK_DEPTH> call_stack_address_table;

		inline constexpr ProfileData()noexcept:
			handle(0),
			call_stack_address_table{}
		{
		}
	};

	/// @brief プロファイラを有効化
	void EnableMemoryProfile();

	/// @brief プロファイラを無効化
	void DisableMemoryProfile();
	
	/// @brief プロファイラが有効かどうか
	bool EnabledMemoryProfile();

	/// @brief プロファイラへ登録
	/// @param heap_info 
	/// @return 
	nox::uint16	Register(const nox::memory::HeapInfo& heap_info);

	/// @brief プロファイラから登録解除
	/// @param heap_info 
	void Unregister(const nox::memory::HeapInfo& heap_info);

	const ProfileData& FindProfileData(nox::not_null<const void*> addr);
	const ProfileData& FindProfileData(nox::uint16 profiler_handle);
}