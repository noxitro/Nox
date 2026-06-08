// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	asset.cpp
/// @brief	asset
#include "pch.h"
#include "asset.h"

void nox::Asset::Bind(std::u8string_view path, nox::AssetManager& manager)
{
	manager_ = manager;
	path_ = nox::U8String(path);
}

bool nox::Asset::Initialize()
{
	nox::io::FileSpanStreamReader reader(path_);
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