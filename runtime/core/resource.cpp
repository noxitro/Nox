//	Copyright (c) 2025 NOX ENGINE All rights reserved.

///	@file	resource.cpp
///	@brief	resource
#include	"pch.h"
#include	"resource.h"
#include	"../kernel/convert_string.h"
#include	"../kernel/os/windows.h"
#include	"../kernel/unicode_converter.h"
#include	"../kernel/io/stream_reader.h"
#include	"../kernel/io/binary_reader.h"

namespace nox
{
	namespace
	{
		/// @brief small optimize用
		constexpr std::size_t k_stack_alloc_threshold = 2048;
	}

}
bool nox::Resource::Initialize(const nox::U8StringView path)
{
	path_ = nox::U8String(path);

	const std::filesystem::path fs_path = path.data();

	nox::io::FileSpanStreamReader reader(path);
	nox::io::BinaryReader binary_reader(reader);

	Header header;
	binary_reader.Read(header);

	is_initialized_ = OnInitialize(binary_reader);
	if (is_initialized_ == false)
	{
		NOX_ASSERT(false, u8"failed to initialize resource.");
	}

	return is_initialized_;
}