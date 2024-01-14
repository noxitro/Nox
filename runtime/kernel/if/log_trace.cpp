///	@file	log_trace.cpp
///	@brief	log_trace
#include	"stdafx.h"
#include	"log_trace.h"

#include	"../memory/stl_allocate_adapter.h"

#if NOX_WIN64
#include	"../os/x64.h"
#endif // NOX_WIN64


#include	<iostream>
using namespace nox;

namespace
{

}


void	debug::detail::TraceDirect(LogCategory log_category, std::u16string_view category, const U16String& message, bool isNewLine, const char16* file, uint32 line)
{
//	std::array<c16, 1024> buffer = { 0 };
	const wchar_t* converted_str = util::CharCast<const wchar_t*>(message.c_str());


	//	コンソールへの出力
#if NOX_WIN64
	//	デバッグウィンドウに出力
	::OutputDebugStringW(converted_str);
#endif // NOX_WIN64
	std::wcout << converted_str << std::endl;
}