//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	stack_trace.cpp
///	@brief	stack_trace
#include	"pch.h"
#include	"stack_trace.h"
#include	"basic_definition.h"

#if NOX_WINDOWS
#include	"os/windows.h"
#pragma warning(push, 0)
#pragma warning(disable:4514)
#pragma warning(disable:4820)
#include	<ImageHlp.h>
#pragma	warning(pop)
#pragma comment(lib, "imagehlp.lib")
#pragma comment(lib, "Dbghelp.lib")
#endif // NITRO_WIN64

#include	"os/mutex.h"
#include	"os/os_utility.h"
#include	"string_format.h"
#include	"log_id.h"
#include	"math/math_algorithm.h"
#include	"assertion.h"
#include	"memory/pmr.h"
#include	"memory/pmr_buffer.h"

namespace nox
{
	namespace
	{
		//	グローバル変数

		/// @brief スタックトレースAPI
		using StackBackTraceFuncType = ::USHORT(WINAPI*)(::ULONG, ::ULONG, ::PVOID*, __out_opt::PULONG);

		constinit StackBackTraceFuncType gRtiCaptureStackBackTrace = nullptr;

		static constinit inline void* mHandlePtr = nullptr;

		//
		nox::os::Mutex g_resolve_mutex;

		//	関数
		inline bool	ResolveStack(std::span<nox::stack_walker::StackFrame> stack_table)
		{
			::HANDLE const processHandle = ::GetCurrentProcess();
			if (processHandle == nullptr)
			{
				return false;
			}

			/* シンボル名最大サイズをセット */
			constexpr size_t MaxNameSize = 255;

			/* シンボル情報サイズを算出 */
			constexpr size_t SymbolInfoSize = sizeof(::SYMBOL_INFOW) + ((MaxNameSize + 1) * sizeof(nox::wchar16));

			//	シンボル情報のメモリ確保
			std::array<nox::uint8, SymbolInfoSize> symbolBuffer;
			::SYMBOL_INFOW* const symbol = reinterpret_cast <::SYMBOL_INFOW*>(symbolBuffer.data());
			symbol->MaxNameLen = MaxNameSize;
			symbol->SizeOfStruct = sizeof(::SYMBOL_INFOW);

			////	Symbol情報
			//::IMAGEHLP_SYMBOL64* symbolInfo = nullptr;
			//u8 symbolInfoBuffer[MAX_PATH + sizeof(::IMAGEHLP_SYMBOL64)] = { 0 };

			//symbolInfo = reinterpret_cast<::IMAGEHLP_SYMBOL64*>(symbolInfoBuffer);
			//symbolInfo->SizeOfStruct = sizeof(::IMAGEHLP_SYMBOL64);
			//symbolInfo->MaxNameLength = MAX_PATH;

			//	hbgHelpはスレッドセーフではないので、ロックする必要がある
			//	https://learn.microsoft.com/ja-jp/windows/win32/api/dbghelp/nf-dbghelp-symfromaddr
			NOX_LOCAL_SCOPE(nox::os::ScopedLock{ g_resolve_mutex });

			//	シンボルハンドラの初期化
			::SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
			::SymInitialize(processHandle, nullptr, TRUE);

			//	情報を収集
			const nox::uint8 stack_num = static_cast<nox::uint8>(stack_table.size());
			for (nox::uint8 i = 0; i < stack_num; ++i)
			{
				nox::stack_walker::StackFrame& stack = stack_table[i];

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
					std::array<nox::char16, MaxNameSize> u32_symbol_name;
					nox::unicode::ConvertU16String(symbol->Name, u32_symbol_name);
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
					std::array<nox::char16, 1024> u32_file_name;
					nox::unicode::ConvertU16String(line.FileName, u32_file_name);
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
					std::array<nox::char16, 1024> u32_module_name;
					nox::unicode::ConvertU16String(moduleInfo.ModuleName, u32_module_name);
					stack.SetModuleName(u32_module_name.data());
				}

				//	解決済みにする
				stack.SetResolved(true);
			}

			return true;
		}
	}
}

void	nox::stack_walker::StackFrame::SetModuleName(std::u16string_view name)
{
	nox::util::StrCopy(name, std::span(module_name_.data(), module_name_.size()));
}

void	nox::stack_walker::StackFrame::SetFileName(std::u16string_view name)
{
	nox::util::StrCopy(name, std::span(file_name_.data(), file_name_.size()));
}

void	nox::stack_walker::StackFrame::SetSymbolName(std::u16string_view name)
{
	nox::util::StrCopy(name, std::span(symbol_name_.data(), symbol_name_.size()));
}

bool	nox::stack_walker::detail::WalkerBase::Collect(const nox::uint8 startDepth)
{
	if (gRtiCaptureStackBackTrace == nullptr)
	{
		return false;
	}

	//	コールスタックバッファ
	std::array<void*, nox::stack_walker::MAX_STACK_DEPTH> bufferAry = { nullptr };

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

bool	nox::stack_walker::detail::WalkerBase::Resolve()
{
	if (nox::ResolveStack(std::span(this->stack_table_, this->stack_length_)) == false)
	{
		return false;
	}

	is_resolved_ = true;
	return true;
}

void	nox::stack_walker::detail::WalkerBase::Trace()const
{
	NOX_INFO_LINE(log_id::Kernel, u"===CallStackTrace開始===");

	for (uint8 i = 0; i < stack_length_; ++i)
	{
		const StackFrame& stack = stack_table_[i];
		if (stack.IsResolved() == false)
		{
			//	失敗したスタックがあればそこで終了
			break;
		}

		//	[Symbol名]([ライン])
		NOX_INFO_LINE(log_id::Kernel, u"{0} ({1})", stack.GetSymbolName().data(), stack.GetLine());

	}

	NOX_INFO_LINE(log_id::Kernel, u"===CallStackTrace終了===");
}

nox::U16String	nox::stack_walker::detail::WalkerBase::GetStackTraceString()const
{
	nox::U16String buffer;

	for (uint8 i = 0; i < stack_length_; ++i)
	{
		const StackFrame& stack = stack_table_[i];
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
		
		buffer += util::Format(u"{0} ({1})", stack.GetSymbolName().data(), stack.GetLine());
	}

	return buffer;
}

namespace
{
	template<nox::concepts::Char CharType>
	inline	std::span<CharType>	GetStackTraceStringImpl(std::span<nox::stack_walker::StackFrame*const> stack_table, std::span< CharType> dest_buffer)
	{
		return dest_buffer;
	}
}

std::span<nox::char32>	nox::stack_walker::detail::WalkerBase::GetStackTraceString(std::span<nox::char32> dest_buffer)const
{
	GetStackTraceStringImpl<char32>({ &stack_table_, stack_length_ }, dest_buffer);

	return dest_buffer;
}

std::span<nox::char16>	nox::stack_walker::detail::WalkerBase::GetStackTraceU16String(std::span<char16> dest_buffer)const
{
	return dest_buffer;
}

const nox::stack_walker::StackFrame& nox::stack_walker::detail::WalkerBase::GetStack(const uint8 index)const {
	NOX_ASSERT_ID(index < collect_length_, nox::assertion::id::OutOfRange, u"コールスタックの取得に失敗");
	return stack_table_[index];
}

nox::stack_walker::StackFrame& nox::stack_walker::detail::WalkerBase::GetStack(const uint8 index) {
	NOX_ASSERT_ID(index < collect_length_, nox::assertion::id::OutOfRange, u"コールスタックの取得に失敗");
	return stack_table_[index];
}

void nox::stack_walker::detail::WalkerBase::SetCollectLength(const uint8 length)
{
	NOX_ASSERT_ID(length <= stack_length_, nox::assertion::id::OutOfRange, u"コールスタックの取得に失敗");
	collect_length_ = length;
}

bool	nox::stack_walker::detail::WalkerSlimBase::Collect(const uint8 startDepth)
{
	if (gRtiCaptureStackBackTrace == nullptr)
	{
		return false;
	}

	//	コールスタックバッファ
	std::array<void*, nox::stack_walker::MAX_STACK_DEPTH> bufferAry = { nullptr };

	//	取得
	uint8 stackLength = static_cast<uint8>(gRtiCaptureStackBackTrace(startDepth, stack_length_, bufferAry.data(), nullptr));

	stackLength = math::Min(stackLength, stack_length_);

	for (uint8 i = 0; i < stackLength; ++i)
	{
		stack_table_[i].SetAddress(reinterpret_cast<std::size_t>(bufferAry.at(i)));
	}

	is_collected_ = true;
	collect_length_ = stackLength;
	return true;
}

void	nox::stack_walker::detail::WalkerSlimBase::Clear()noexcept
{
	is_collected_ = false;
}

namespace nox::stack_walker
{
	namespace
	{
		constexpr	nox::int32	k_max_name_size = 1024;

		/// @brief [Symbol名]([ライン])を取得
		/// @param dest_buffer 
		/// @param processHandle 
		/// @param symbol 
		/// @param stack 
		/// @return 
		inline std::optional<std::u16string_view> GetCallStack(std::span<nox::char16> dest_buffer, ::HANDLE const processHandle, ::SYMBOL_INFOW* const symbol, const nox::stack_walker::SlimStackFrame& stack)
		{
			//	アドレス
			const ::DWORD64 nativeAddr = static_cast<::DWORD64>(stack.GetAddress());
			::DWORD64  dwDisplacement64 = 0;
			/* トレースアドレスからシンボル情報を取得 */
			if (::SymFromAddrW(processHandle, nativeAddr, &dwDisplacement64, symbol) == false)
			{
				return std::nullopt;
			}
			std::array<nox::char16, nox::stack_walker::k_max_name_size> u32_symbol_name{ 0 };
			nox::unicode::ConvertU16String(symbol->Name, u32_symbol_name);

			//	ラインを取得
			::IMAGEHLP_LINEW64 line;
			line.SizeOfStruct = sizeof(::IMAGEHLP_LINEW64);
			::DWORD dwDisplacement = 0x10000000;
			//	::SymSetOptions()
			if (::SymGetLineFromAddrW64(processHandle, nativeAddr, &dwDisplacement, &line) == FALSE)
			{
				return std::u16string_view(u32_symbol_name.data(), symbol->NameLen);
			}
			std::array<nox::char16, 1024> u32_file_name = { 0 };
			nox::unicode::ConvertU16String(line.FileName, u32_file_name);
			util::Format(dest_buffer, u"{0} ({1})", std::u16string_view(u32_symbol_name.data(), symbol->NameLen), line.LineNumber);
			return std::u16string_view(dest_buffer.data());
		}

		inline	bool	TraceImpl(::HANDLE const processHandle, ::SYMBOL_INFOW* const symbol, const nox::stack_walker::SlimStackFrame& stack)
		{
			//	アドレス
			const ::DWORD64 nativeAddr = static_cast<::DWORD64>(stack.GetAddress());
			::DWORD64  dwDisplacement64 = 0;

			/* トレースアドレスからシンボル情報を取得 */
			if (::SymFromAddrW(processHandle, nativeAddr, &dwDisplacement64, symbol) == false)
			{
				return false;
			}

			std::array<nox::char16, nox::stack_walker::k_max_name_size> u32_symbol_name{ 0 };
			nox::unicode::ConvertU16String(symbol->Name, u32_symbol_name);

			//	ラインを取得
			::IMAGEHLP_LINEW64 line;
			line.SizeOfStruct = sizeof(::IMAGEHLP_LINEW64);

			::DWORD dwDisplacement = 0x10000000;
			//	::SymSetOptions()

			if (::SymGetLineFromAddrW64(processHandle, nativeAddr, &dwDisplacement, &line) == FALSE)
			{
				//	[Symbol名]([ライン])
				NOX_INFO_LINE(log_id::Kernel, u"{0} (invalid)", std::u16string_view(u32_symbol_name.data(), symbol->NameLen));
				return true;
				//			return false;
			}

			std::array<nox::char16, 1024> u32_file_name = { 0 };
			nox::unicode::ConvertU16String(line.FileName, u32_file_name);

			//	モジュール情報
			::IMAGEHLP_MODULEW64 moduleInfo;
			moduleInfo.SizeOfStruct = sizeof(::IMAGEHLP_MODULEW64);

			if (::SymGetModuleInfoW64(processHandle, nativeAddr, &moduleInfo) == FALSE)
			{
				return false;
			}

			std::array<nox::char16, 1024> u32_module_name;
			nox::unicode::ConvertU16String(moduleInfo.ModuleName, u32_module_name);

			//	[Symbol名]([ライン])
			NOX_INFO_LINE(log_id::Kernel, u"{0} ({1})", std::u16string_view(u32_symbol_name.data(), symbol->NameLen), line.LineNumber);
			return true;
		}
	}
}

void	nox::stack_walker::detail::WalkerSlimBase::Trace()const
{
	if (is_collected_ == false)
	{
		return;
	}

	::HANDLE const processHandle = ::GetCurrentProcess();
	NOX_ASSERT(FAILED(processHandle), u8"processHandle is failed");

	/* シンボル情報サイズを算出 */
	constexpr size_t SymbolInfoSize = sizeof(::SYMBOL_INFOW) + ((k_max_name_size + 1) * sizeof(nox::wchar16));

	//	シンボル情報のメモリ確保
	std::array<nox::uint8, SymbolInfoSize> symbolBuffer;
	::SYMBOL_INFOW* const symbol = reinterpret_cast <::SYMBOL_INFOW*>(symbolBuffer.data());
	symbol->MaxNameLen = k_max_name_size;
	symbol->SizeOfStruct = sizeof(::SYMBOL_INFOW);

	//	hbgHelpはスレッドセーフではないので、ロックする必要がある
	//	https://learn.microsoft.com/ja-jp/windows/win32/api/dbghelp/nf-dbghelp-symfromaddr
	NOX_LOCAL_SCOPE(nox::os::ScopedLock{ g_resolve_mutex });

	//	シンボルハンドラの初期化
	::SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
	::SymInitialize(processHandle, nullptr, TRUE);

	NOX_INFO_LINE(log_id::Kernel, u"===CallStackTrace開始===");

	for (uint8 i = 0; i < stack_length_; ++i)
	{
		const nox::stack_walker::SlimStackFrame& stack = stack_table_[i];

		if (nox::stack_walker::TraceImpl(processHandle, symbol, stack) == false)
		{
			NOX_INFO_LINE(log_id::Kernel, u"途中で失敗しました");
			break;
		}
	}

	NOX_INFO_LINE(log_id::Kernel, u"===CallStackTrace終了===\n");

	//	シンボルハンドラのクリーンアップ
	::SymCleanup(processHandle);
}

std::u16string_view nox::stack_walker::detail::WalkerSlimBase::GetStackTraceU16(std::span<nox::char16> dest)const
{
	
	return {};
}

void nox::stack_walker::detail::WalkerSlimBase::SetCollectLength(const uint8 length)
{
	NOX_ASSERT_ID(length <= stack_length_, nox::assertion::id::OutOfRange, u"コールスタックの取得に失敗");
	collect_length_ = length;
}

const nox::stack_walker::SlimStackFrame& nox::stack_walker::detail::WalkerSlimBase::GetStack(const uint8 index)const {
	NOX_ASSERT_ID(index < collect_length_, nox::assertion::id::OutOfRange, u"コールスタックの取得に失敗");
	return stack_table_[index];
}

nox::stack_walker::SlimStackFrame& nox::stack_walker::detail::WalkerSlimBase::GetStack(const uint8 index) {
	NOX_ASSERT_ID(index < collect_length_, nox::assertion::id::OutOfRange, u"コールスタックの取得に失敗");
	return stack_table_[index];
}

void	nox::stack_walker::Initialize()
{
	NOX_ASSERT(mHandlePtr == nullptr, u"初期化済み");
	mHandlePtr = os::LoadDLL(u"kernel32.dll");
	gRtiCaptureStackBackTrace = os::GetProcAddress<StackBackTraceFuncType>(mHandlePtr, "RtlCaptureStackBackTrace");

}

void	nox::stack_walker::Finalize()
{
	NOX_ASSERT(mHandlePtr != nullptr, u"破棄済み");
	gRtiCaptureStackBackTrace = nullptr;
	os::UnloadDLL(mHandlePtr);
}

void nox::stack_walker::Trace(std::span<const size_t> address_list)
{
	nox::stack_walker::StackWalkerSlim walker;
	walker.SetCollectLength(static_cast<nox::uint8>(address_list.size()));
	walker.Collected();

	for (int32 i = 0; i < address_list.size(); ++i)
	{
		walker.GetStack(static_cast<nox::uint8>(i)).SetAddress(address_list[i]);
	}

	walker.Trace();
}