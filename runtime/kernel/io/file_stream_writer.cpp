//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	file_stream_writer.cpp
///	@brief	file_stream_writer
#include	"pch.h"
#include	"file_stream_writer.h"
//#include	<filesystem>
#include	"../os/windows.h"

nox::io::FileStreamWriter::~FileStreamWriter()
{
	Close();
}

void nox::io::FileStreamWriter::Open(std::u8string_view path)
{
	std::array<nox::char16, 256> max_pass_buffer = { 0 };

//	::CreateFileW()
}

void nox::io::FileStreamWriter::Close()
{
	if (native_file_handle_ == nullptr)
	{
		return;
	}

	::CloseHandle(native_file_handle_);
	native_file_handle_ = nullptr;
}

bool nox::io::FileStreamWriter::IsOpen()const noexcept
{
	if (native_file_handle_ == nullptr ||
		native_file_handle_ == INVALID_HANDLE_VALUE
		)
	{
		return false;
	}

	return true;
}