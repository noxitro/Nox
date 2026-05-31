//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	FileWin64.cpp
///	@brief	FileWin64
#include	"pch.h"
#include	"file_win64.h"
#if NOX_WIN64
nox::os::detail::FileWin64::~FileWin64()
{
	Close();
}

bool nox::os::detail::FileWin64::Open(std::u8string_view path, std::u8string_view mode)
{
	const ::errno_t err = ::fopen_s(&file_, reinterpret_cast<const char*>(path.data()), reinterpret_cast<const char*>(mode.data()));
	if (err != 0 || file_ == nullptr)
	{
		return false;
	}
	return true;
}

void nox::os::detail::FileWin64::Close()
{
	if (file_ != nullptr)
	{
		std::fclose(file_);
		file_ = nullptr;
	}
}

void nox::os::detail::FileWin64::Write(std::span<const std::byte> src)const
{
	if (IsOpen() == false)
	{
		return;
	}
	std::fwrite(src.data(), sizeof(std::byte), src.size(), file_);
}

std::span<std::byte> nox::os::detail::FileWin64::Read(std::span<std::byte> dest)const
{
	if (IsOpen() == false)
	{
		return {};
	}

	const std::size_t read_size = std::fread(dest.data(), sizeof(std::byte), dest.size(), file_);
	return dest.subspan(0, read_size);
}
#endif