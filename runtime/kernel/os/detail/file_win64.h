//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	FileWin64.h
///	@brief	FileWin64
#pragma once
#include	"../../basic_definition.h"
#include	"file_base.h"

#if NOX_WIN64
namespace nox::os::detail
{
	class FileWin64 final : public nox::os::FileBase
	{
	public:
		inline constexpr FileWin64() noexcept :
			file_(nullptr)
		{}

		~FileWin64();

		bool Open(std::u8string_view path, std::u8string_view mode);
		void Close();

		inline constexpr bool IsOpen()const noexcept { return file_ != nullptr; }
		
		void Write(std::span<const std::byte> src)const;
		std::span<std::byte> Read(std::span<std::byte> dest)const;

		inline constexpr std::FILE* GetNativeHandle()const noexcept { return file_; }

	private:
		std::FILE* file_;
	};
}
#endif