//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	os_utility.h
///	@brief	os_utility
#pragma once
#include	"os_definition.h"

#include	"../basic_definition.h"

#include	<string_view>

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