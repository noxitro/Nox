//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	file_stream_writer.h
///	@brief	file_stream_writer
#pragma once
#include	"stream_writer.h"

namespace nox::io
{
	class FileStreamWriter : public nox::io::StreamWriter
	{
	public:
		inline constexpr FileStreamWriter()noexcept :
			native_file_handle_(nullptr) 
		{
		}

		~FileStreamWriter()override;

		void Open(std::u8string_view path);
		void Close();
		bool IsOpen()const noexcept;
	private:
		void* native_file_handle_;
	};
}