//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	FileWin64.cpp
///	@brief	FileWin64
#include	"pch.h"
#include	"file_win64.h"

#include	"../file_system.h"
#include	"../../assertion.h"
#include	"../os_definition.h"
#include	"../windows.h"

#if NOX_WIN64
namespace nox::os::detail
{
	namespace
	{
		struct NativeFileOpenMode
		{
			::DWORD desired_access;
			::DWORD share_mode;
			::DWORD creation_disposition;
		};

		constexpr ::DWORD k_default_share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE;
		constexpr std::size_t k_max_io_size = static_cast<std::size_t>((std::numeric_limits<::DWORD>::max)());

		inline constexpr ::HANDLE ToNativeHandle(void* native_file_handle) noexcept
		{
			return static_cast<::HANDLE>(native_file_handle);
		}

		inline ::DWORD GetChunkSize(const std::size_t size) noexcept
		{
			return static_cast<::DWORD>(std::min(size, k_max_io_size));
		}

		bool TryConvertPath(const std::u8string_view path, std::span<wchar_t> dest_buffer, std::wstring_view& result) noexcept
		{
			if (path.empty() == true || dest_buffer.size() < 2)
			{
				return false;
			}

			if (path.size() > static_cast<std::size_t>((std::numeric_limits<int>::max)()))
			{
				return false;
			}

			const int native_length = ::MultiByteToWideChar(
				CP_UTF8,
				MB_ERR_INVALID_CHARS,
				reinterpret_cast<const char*>(path.data()),
				static_cast<int>(path.size()),
				dest_buffer.data(),
				static_cast<int>(dest_buffer.size() - 1));
			if (native_length <= 0)
			{
				return false;
			}

			dest_buffer[static_cast<std::size_t>(native_length)] = L'\0';
			result = std::wstring_view(dest_buffer.data(), static_cast<std::size_t>(native_length));
			return true;
		}

		bool TryParseOpenMode(const std::u8string_view mode, NativeFileOpenMode& result) noexcept
		{
			if (mode.empty() == true)
			{
				return false;
			}

			bool is_plus_mode = false;
			for (std::size_t i = 1; i < mode.size(); ++i)
			{
				switch (mode[i])
				{
				case u8'+':
					if (is_plus_mode == true)
					{
						return false;
					}
					is_plus_mode = true;
					break;
				case u8'b':
				case u8't':
					break;
				default:
					return false;
				}
			}

			switch (mode.front())
			{
			case u8'r':
				result = NativeFileOpenMode
				{
					.desired_access = is_plus_mode ? GENERIC_READ | GENERIC_WRITE : GENERIC_READ,
					.share_mode = k_default_share_mode,
					.creation_disposition = OPEN_EXISTING,
				};
				return true;
			case u8'w':
				result = NativeFileOpenMode
				{
					.desired_access = is_plus_mode ? GENERIC_READ | GENERIC_WRITE : GENERIC_WRITE,
					.share_mode = k_default_share_mode,
					.creation_disposition = CREATE_ALWAYS,
				};
				return true;
			case u8'a':
				result = NativeFileOpenMode
				{
					.desired_access = is_plus_mode ? GENERIC_READ | FILE_APPEND_DATA : FILE_APPEND_DATA,
					.share_mode = k_default_share_mode,
					.creation_disposition = OPEN_ALWAYS,
				};
				return true;
			default:
				return false;
			}
		}
	}
}

bool nox::os::Exists(std::u8string_view path)
{
	std::array<wchar_t, nox::os::k_max_path_length> native_path_buffer{};
	std::wstring_view native_path;
	if (nox::os::detail::TryConvertPath(path, native_path_buffer, native_path) == false)
	{
		return false;
	}
	::WIN32_FILE_ATTRIBUTE_DATA file_attribute_data{};
	const ::BOOL result = ::GetFileAttributesExW(native_path_buffer.data(), ::GetFileExInfoStandard, &file_attribute_data);
	return result != FALSE;
}

nox::os::detail::FileWin64::~FileWin64() noexcept
{
	Close();
}

bool nox::os::detail::FileWin64::Open(std::u8string_view path, std::u8string_view mode)
{
	Close();

	NativeFileOpenMode native_mode{};
	if (TryParseOpenMode(mode, native_mode) == false)
	{
		return false;
	}

	std::array<wchar_t, nox::os::k_max_path_length> native_path_buffer{};
	std::wstring_view native_path;
	if (TryConvertPath(path, native_path_buffer, native_path) == false)
	{
		return false;
	}

	::HANDLE const native_handle = ::CreateFileW(
		native_path_buffer.data(),
		native_mode.desired_access,
		native_mode.share_mode,
		nullptr,
		native_mode.creation_disposition,
		FILE_ATTRIBUTE_NORMAL,
		nullptr);
	if (native_handle == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	native_file_handle_ = native_handle;
	return true;
}

void nox::os::detail::FileWin64::Close() noexcept
{
	if (native_file_handle_ != nullptr)
	{
		::CloseHandle(ToNativeHandle(native_file_handle_));
		native_file_handle_ = nullptr;
	}
}

void nox::os::detail::FileWin64::Write(std::span<const std::byte> src)const
{
	NOX_ASSERT(IsOpen() == true, u"file is not open");
	if (IsOpen() == false)
	{
		return;
	}

	std::span<const std::byte> remain = src;
	while (remain.empty() == false)
	{
		const ::DWORD chunk_size = GetChunkSize(remain.size());

		::DWORD written_size = 0;
		const ::BOOL is_success = ::WriteFile(
			ToNativeHandle(native_file_handle_),
			remain.data(),
			chunk_size,
			&written_size,
			nullptr);
		NOX_ASSERT(is_success == TRUE, u"WriteFile failed");
		NOX_ASSERT(written_size != 0, u"WriteFile wrote zero bytes");
		if (is_success == FALSE || written_size == 0)
		{
			return;
		}

		remain = remain.subspan(written_size);
	}
}

std::span<std::byte> nox::os::detail::FileWin64::Read(std::span<std::byte> dest)const
{
	NOX_ASSERT(IsOpen() == true, u"file is not open");
	if (IsOpen() == false)
	{
		return {};
	}

	std::size_t total_read_size = 0;
	std::span<std::byte> remain = dest;
	while (remain.empty() == false)
	{
		const ::DWORD chunk_size = GetChunkSize(remain.size());

		::DWORD read_size = 0;
		const ::BOOL is_success = ::ReadFile(
			ToNativeHandle(native_file_handle_),
			remain.data(),
			chunk_size,
			&read_size,
			nullptr);
		NOX_ASSERT(is_success == TRUE, u"ReadFile failed");
		if (is_success == FALSE || read_size == 0)
		{
			break;
		}

		total_read_size += read_size;
		remain = remain.subspan(read_size);

		if (read_size < chunk_size)
		{
			break;
		}
	}

	return dest.subspan(0, total_read_size);
}

nox::uint64 nox::os::detail::FileWin64::GetSize()const
{
	NOX_ASSERT(IsOpen() == true, u"file is not open");
	if (IsOpen() == false)
	{
		return 0;
	}

	::LARGE_INTEGER file_size{};
	const ::BOOL is_success = ::GetFileSizeEx(ToNativeHandle(native_file_handle_), &file_size);
	NOX_ASSERT(is_success == TRUE, u"GetFileSizeEx failed");
	if (is_success == FALSE || file_size.QuadPart < 0)
	{
		return 0;
	}

	return static_cast<nox::uint64>(file_size.QuadPart);
}

nox::os::detail::ReadOnlyMappedFileWin64::~ReadOnlyMappedFileWin64() noexcept
{
	Close();
}

bool nox::os::detail::ReadOnlyMappedFileWin64::Open(std::u8string_view path)
{
	Close();

	std::array<wchar_t, nox::os::k_max_path_length> native_path_buffer{};
	std::wstring_view native_path;
	if (TryConvertPath(path, native_path_buffer, native_path) == false)
	{
		return false;
	}

	::HANDLE const file_handle = ::CreateFileW(
		native_path_buffer.data(),
		GENERIC_READ,
		FILE_SHARE_READ,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		nullptr);
	if (file_handle == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	::LARGE_INTEGER file_size{};
	if (::GetFileSizeEx(file_handle, &file_size) == FALSE || file_size.QuadPart <= 0)
	{
		//	サイズ0（破損扱い）や取得失敗はマップできない
		::CloseHandle(file_handle);
		return false;
	}

	::HANDLE const mapping_handle = ::CreateFileMappingW(
		file_handle,
		nullptr,
		PAGE_READONLY,
		0,
		0,
		nullptr);
	if (mapping_handle == nullptr)
	{
		::CloseHandle(file_handle);
		return false;
	}

	void* const view = ::MapViewOfFile(mapping_handle, FILE_MAP_READ, 0, 0, 0);
	if (view == nullptr)
	{
		::CloseHandle(mapping_handle);
		::CloseHandle(file_handle);
		return false;
	}

	file_handle_ = file_handle;
	mapping_handle_ = mapping_handle;
	data_ = static_cast<const std::byte*>(view);
	size_ = static_cast<std::size_t>(file_size.QuadPart);
	return true;
}

void nox::os::detail::ReadOnlyMappedFileWin64::Close() noexcept
{
	if (data_ != nullptr)
	{
		::UnmapViewOfFile(data_);
		data_ = nullptr;
	}
	if (mapping_handle_ != nullptr)
	{
		::CloseHandle(static_cast<::HANDLE>(mapping_handle_));
		mapping_handle_ = nullptr;
	}
	if (file_handle_ != nullptr)
	{
		::CloseHandle(static_cast<::HANDLE>(file_handle_));
		file_handle_ = nullptr;
	}
	size_ = 0;
}
#endif