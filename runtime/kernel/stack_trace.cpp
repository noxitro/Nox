//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	stack_trace.cpp
///	@brief	stack_trace
#include	"stdafx.h"
#include	"stack_trace.h"
#include	"basic_definition.h"
#include	"os/mutex.h"
#include	"os/os_utility.h"

#include	"string_format.h"
#include	"log_trace.h"
#include	"math/math_algorithm.h"
#if NOX_WINDOWS
#pragma warning(push, 0)
#pragma warning(disable:4514)
#pragma warning(disable:4820)
#include	<ImageHlp.h>
#pragma	warning(pop)
#pragma comment(lib, "imagehlp.lib")
#pragma comment(lib, "Dbghelp.lib")
#endif // NITRO_WIN64


using namespace nox;
using namespace nox::stack_walker;
using namespace nox::stack_walker::detail;

namespace
{
	//	グローバル変数

	/**
	 * @brief スタックトレースAPI
	*/
	using StackBackTraceFuncType = ::USHORT(WINAPI*)(::ULONG, ::ULONG, ::PVOID*, __out_opt::PULONG);

	StackBackTraceFuncType gRtiCaptureStackBackTrace = nullptr;

	/**
	 * @brief
	*/
	static inline void* mHandlePtr = nullptr;
	//

	/**
	 * @brief
	*/
	os::Mutex gResolveMutex;

	//if (n >= 63)
	//{
	//	n = 62;
	//}
	//return (int)RtlCaptureStackBackTraceProc(0, n, buffer, nullptr);

	//	関数

	inline bool	ResolveStack(Stack* const stackTbl, const uint8 stackNum)
	{
		::HANDLE const processHandle = ::GetCurrentProcess();
		if (processHandle == nullptr)
		{
			return false;
		}

		/* シンボル名最大サイズをセット */
		constexpr size_t MaxNameSize = 255;

		/* シンボル情報サイズを算出 */
		constexpr size_t SymbolInfoSize = sizeof(::SYMBOL_INFOW) + ((MaxNameSize + 1) * sizeof(char));

		//	シンボル情報のメモリ確保
		std::array<uint8, SymbolInfoSize> symbolBuffer;
		::SYMBOL_INFOW* const symbol = reinterpret_cast <::SYMBOL_INFOW*>(symbolBuffer.data());
		symbol->MaxNameLen = MaxNameSize;
		symbol->SizeOfStruct = sizeof(::SYMBOL_INFOW);

		////	Symbol情報
		//::IMAGEHLP_SYMBOL64* symbolInfo = nullptr;
		//u8 symbolInfoBuffer[MAX_PATH + sizeof(::IMAGEHLP_SYMBOL64)] = { 0 };

		//symbolInfo = reinterpret_cast<::IMAGEHLP_SYMBOL64*>(symbolInfoBuffer);
		//symbolInfo->SizeOfStruct = sizeof(::IMAGEHLP_SYMBOL64);
		//symbolInfo->MaxNameLength = MAX_PATH;

		//	::sys関係はスレッドセーフではないので、ロック
		os::ScopedLock scopedLock(gResolveMutex);

		//	シンボルハンドラの初期化
		::SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
		::SymInitialize(processHandle, nullptr, TRUE);


		//	情報を収集
		for (uint8 i = 0; i < stackNum; ++i)
		{
			Stack& stack = stackTbl[i];

			//	解決済みか
			if (stack.IsResolved() == true)
			{
				continue;
			}

			//	有効なアドレスか？
			if (stack.IsInvalidAddress() == true)
			{
				continue;
			}

			//	アドレス
			const ::DWORD64 nativeAddr = static_cast<::DWORD64>(stack.GetAddress());
			::DWORD64  dwDisplacement64 = 0;


			/* トレースアドレスからシンボル情報を取得 */
			if (::SymFromAddrW(processHandle, nativeAddr, &dwDisplacement64, symbol) == false)
			{
				continue;
			}

			{
				std::array<char32, MaxNameSize> u32_symbol_name;
				unicode::ConvertU32String(symbol->Name, u32_symbol_name);
				stack.SetSymbolName(u32_symbol_name.data());

			}

			//	ラインを取得
			::IMAGEHLP_LINEW64 line;
			line.SizeOfStruct = sizeof(::IMAGEHLP_LINEW64);

			::DWORD dwDisplacement = 0x10000000;
			//	::SymSetOptions()

			if (::SymGetLineFromAddrW64(processHandle, nativeAddr, &dwDisplacement, &line) == FALSE)
			{
				continue;
			}

			stack.SetLine(line.LineNumber);

			{
				std::array<char32, 1024> u32_file_name;
				unicode::ConvertU32String(line.FileName, u32_file_name);
				stack.SetFileName(u32_file_name.data());

			}

			//	モジュール情報
			::IMAGEHLP_MODULEW64 moduleInfo;
			moduleInfo.SizeOfStruct = sizeof(::IMAGEHLP_MODULEW64);

			if (::SymGetModuleInfoW64(processHandle, nativeAddr, &moduleInfo) == FALSE)
			{
				continue;
			}

			{
				std::array<char32, 1024> u32_module_name;
				unicode::ConvertU32String(moduleInfo.ModuleName, u32_module_name);
				stack.SetModuleName(u32_module_name.data());
			}

			//	解決済みにする
			stack.SetResolved(true);
		}

		return true;
	}
}

void	Stack::SetModuleName(StringView name)
{
	util::StrCopy(name.operator std::u32string_view(), std::span(module_name_.data(), module_name_.size()));
}

void	Stack::SetFileName(StringView name)
{
	util::StrCopy(name.operator std::u32string_view(), std::span(file_name_.data(), file_name_.size()));
}

void	Stack::SetSymbolName(StringView name)
{
	util::StrCopy(name.operator std::u32string_view(), std::span(symbol_name_.data(), symbol_name_.size()));
}

bool	WalkerBase::Collect(const uint8 startDepth)
{
	if (gRtiCaptureStackBackTrace == nullptr)
	{
		return false;
	}

	//	コールスタックバッファ
	std::array<void*, MAX_STACK_DEPTH> bufferAry = { nullptr };

	//	取得
	uint8 stackLength = static_cast<uint8>(gRtiCaptureStackBackTrace(startDepth, stack_length_, bufferAry.data(), nullptr));

	stackLength = math::Min(stackLength, stack_length_);

	for (uint8 i = 0; i < stackLength; ++i)
	{
		stack_table_[i].SetAddress(reinterpret_cast<std::size_t>(bufferAry.at(i)));
	}

	is_collected_ = true;
	return true;
}

bool	WalkerBase::Resolve()
{
	if (::ResolveStack(stack_table_, stack_length_) == false)
	{
		return false;
	}

	is_resolved_ = true;
	return true;
}

void	WalkerBase::Trace()const
{
	NOX_INFO_LINE(U"===CallStackTrace開始===");

	for (uint8 i = 0; i < stack_length_; ++i)
	{
		const Stack& stack = stack_table_[i];
		if (stack.IsResolved() == false)
		{
			//	失敗したスタックがあればそこで終了
			break;
		}

		//	[Symbol名]([ライン])
		NOX_INFO_LINE(util::Format(U"{0} ({1})", stack.GetSymbolName().data(), stack.GetLine()));

	}

	NOX_INFO_LINE(U"===CallStackTrace終了===");
}

nox::String	WalkerBase::GetStackTraceString()const
{
	nox::String buffer;

	for (uint8 i = 0; i < stack_length_; ++i)
	{
		const Stack& stack = stack_table_[i];
		if (stack.IsResolved() == false)
		{
			//	失敗したスタックがあればそこで終了
			break;
		}

		//	[Symbol名]([ライン])
		if (i > 0)
		{
			buffer += U'\n';
		}
		
		buffer += util::Format(U"{0} ({1})", stack.GetSymbolName().data(), stack.GetLine());
	}

	return buffer;
}

namespace
{
	template<concepts::Char CharType>
	inline	std::span<CharType>	GetStackTraceStringImpl(std::span<Stack*const> stack_table, std::span< CharType> dest_buffer)
	{
		return dest_buffer;
	}
}

std::span<char32>	WalkerBase::GetStackTraceString(std::span<char32> dest_buffer)const
{
	GetStackTraceStringImpl<char32>({ &stack_table_, stack_length_ }, dest_buffer);

	return dest_buffer;
}

std::span<char16>	WalkerBase::GetStackTraceU16String(std::span<char16> dest_buffer)const
{
	return dest_buffer;
}

void	stack_walker::Initialize()
{
	mHandlePtr = os::LoadDLL(u"kernel32.dll");
	gRtiCaptureStackBackTrace = os::GetProcAddress<StackBackTraceFuncType>(mHandlePtr, "RtlCaptureStackBackTrace");

}

void	stack_walker::Finalize()
{
	gRtiCaptureStackBackTrace = nullptr;
	os::UnloadDLL(mHandlePtr);
}