//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	os_utility.h
///	@brief	os_utility
#pragma once
#include	<string_view>

#include	"os_definition.h"
#include	"../basic_definition.h"

namespace nox::os
{
	/**
	 * @brief
	 * @return
	*/
	SystemInfo	GetSystemInfo();

	/**
	 * @brief コア数を取得
	 * @return
	*/
	uint8 GetHardwareConcurrency();

	/// @brief 論理プロセッサ数を取得する
	/// @details GetHardwareConcurrencyはuint8で飽和するため、64論理プロセッサを超える環境も
	///          扱えるようにこちらを使う。取得に失敗した場合は1を返す(0は返さない)。
	uint32 GetLogicalProcessorCount()noexcept;

	namespace detail
	{
		[[nodiscard]] void* GetProcAddressImpl(void* const moduleHandle, const char* const procNamePtr);
	}

	/**
	 * @brief DLL読み込み
	 * @details	読み込みに失敗した場合アサートを投げます
	 * @param path
	 * @param handlePtr
	 * @param flags
	 * @return dllハンドル
	*/
	[[nodiscard]] void* LoadDLL(std::u16string_view path, void* handlePtr = nullptr, const uint32 flags = 0);

	/**
	 * @brief DLLの破棄
	 * @param module_handler_ptr 破棄するハンドル
	 * @return 破棄できたかどうか
	*/
	bool UnloadDLL(nox::not_null<void*> module_handler_ptr);

	template<class T>
	inline T GetProcAddress(not_null<void*> moduleHandle, std::string_view procNamePtr) 
	{
		return reinterpret_cast<T>(detail::GetProcAddressImpl(moduleHandle, procNamePtr.data()));
	}
}