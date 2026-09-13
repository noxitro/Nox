//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	file_win64_test.cpp
///	@brief	file_win64_test
#include "pch.h"

#include <array>
#include <optional>

#include "../os/file.h"
#include "../os/os_definition.h"
#include "../os/windows.h"
#include "../unicode_converter.h"

namespace
{
	struct TempFilePath
	{
		static constexpr std::size_t k_utf8_path_buffer_size = static_cast<std::size_t>(nox::os::k_max_path_length) * 4;

		std::array<wchar_t, nox::os::k_max_path_length> native_path{};
		std::array<nox::char8, k_utf8_path_buffer_size> utf8_path{};
		std::size_t native_length = 0;
		std::size_t utf8_length = 0;

		[[nodiscard]] std::wstring_view NativePath() const noexcept
		{
			return std::wstring_view(native_path.data(), native_length);
		}

		[[nodiscard]] std::u8string_view Utf8Path() const noexcept
		{
			return std::u8string_view(utf8_path.data(), utf8_length);
		}
	};

	struct TempFileCleanup final
	{
		const wchar_t* native_path = nullptr;

		~TempFileCleanup()
		{
			if (native_path != nullptr)
			{
				::DeleteFileW(native_path);
			}
		}
	};

	std::optional<TempFilePath> MakeTempFilePath()
	{
		TempFilePath temp_file_path{};

		const ::DWORD directory_length = ::GetTempPathW(
			static_cast<::DWORD>(temp_file_path.native_path.size()),
			temp_file_path.native_path.data());
		if (directory_length == 0 || directory_length >= temp_file_path.native_path.size())
		{
			return std::nullopt;
		}

		const int file_name_length = ::swprintf_s(
			temp_file_path.native_path.data() + directory_length,
			temp_file_path.native_path.size() - directory_length,
			L"nox_file_win64_%lu_%llu_テスト.bin",
			static_cast<unsigned long>(::GetCurrentProcessId()),
			static_cast<unsigned long long>(::GetTickCount64()));
		if (file_name_length <= 0)
		{
			return std::nullopt;
		}

		temp_file_path.native_length = directory_length + static_cast<std::size_t>(file_name_length);
		const std::u8string_view utf8_path = nox::unicode::ConvertU8String(
			temp_file_path.NativePath(),
			std::span<nox::char8>(temp_file_path.utf8_path.data(), temp_file_path.utf8_path.size()));
		temp_file_path.utf8_length = utf8_path.size();
		temp_file_path.utf8_path[temp_file_path.utf8_length] = u8'\0';

		return temp_file_path;
	}
}

TEST(KernelFileWin64Test, WriteAndReadBinaryWithUtf8Path)
{
	const std::optional<TempFilePath> temp_file_path = MakeTempFilePath();
	ASSERT_TRUE(temp_file_path.has_value());

	TempFileCleanup cleanup{ temp_file_path->native_path.data() };
	::DeleteFileW(temp_file_path->native_path.data());

	const std::array<std::byte, 4> expected =
	{
		std::byte{ 0x10 },
		std::byte{ 0x20 },
		std::byte{ 0x30 },
		std::byte{ 0x40 }
	};

	nox::os::File writer;
	ASSERT_TRUE(writer.Open(temp_file_path->Utf8Path(), u8"wb"));
	writer.Write(expected);
	writer.Close();

	std::array<std::byte, expected.size()> actual{};
	nox::os::File reader;
	ASSERT_TRUE(reader.Open(temp_file_path->Utf8Path(), u8"rb"));
	const std::span<std::byte> read_span = reader.Read(actual);
	reader.Close();

	EXPECT_EQ(read_span.size(), expected.size());
	EXPECT_EQ(actual, expected);
}

TEST(KernelFileWin64Test, AppendModeWritesAtEnd)
{
	const std::optional<TempFilePath> temp_file_path = MakeTempFilePath();
	ASSERT_TRUE(temp_file_path.has_value());

	TempFileCleanup cleanup{ temp_file_path->native_path.data() };
	::DeleteFileW(temp_file_path->native_path.data());

	const std::array<std::byte, 2> head =
	{
		std::byte{ 0xAB },
		std::byte{ 0xCD }
	};
	const std::array<std::byte, 2> tail =
	{
		std::byte{ 0xEF },
		std::byte{ 0x12 }
	};
	const std::array<std::byte, 4> expected =
	{
		head[0],
		head[1],
		tail[0],
		tail[1]
	};

	nox::os::File writer;
	ASSERT_TRUE(writer.Open(temp_file_path->Utf8Path(), u8"wb"));
	writer.Write(head);
	writer.Close();

	nox::os::File appender;
	ASSERT_TRUE(appender.Open(temp_file_path->Utf8Path(), u8"ab"));
	appender.Write(tail);
	appender.Close();

	std::array<std::byte, expected.size()> actual{};
	nox::os::File reader;
	ASSERT_TRUE(reader.Open(temp_file_path->Utf8Path(), u8"rb"));
	const std::span<std::byte> read_span = reader.Read(actual);
	reader.Close();

	EXPECT_EQ(read_span.size(), expected.size());
	EXPECT_EQ(actual, expected);
}
