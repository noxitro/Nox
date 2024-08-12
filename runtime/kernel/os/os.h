//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	os.h
///	@brief	os
#pragma once
#include	"../advanced_type.h"



namespace nox
{
	class String;

	
	
	namespace os
	{
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

		/// @brief 終了処理
		void	Finalize();

		/// @brief 指定のコマンドライン引数を取得します
		/// @param index 
		/// @return 
		std::span<const char16* const> GetCommandLineArgList()noexcept;

		inline std::u16string_view GetCommandLineArg(uint32 index)
		{
			return GetCommandLineArgList()[index];
		}

		U16String	GetDirectoryUTF8();

		ProcessMemoryInfo GetCurrentProcessMemoryInfo();
	}
}