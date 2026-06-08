// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	asset.h
/// @brief	asset
#pragma once
#include	"object.h"

namespace nox
{
	class AssetManager;
	class Asset : public nox::Object
	{
		NOX_DECLARE_OBJECT(Asset, nox::Object);
		struct alignas(16) Header
		{
			nox::uint32 magic;
			nox::uint16 version;
			nox::uint16 flags;
		};
	public:
		inline Asset() : is_initialized_(false) {}

		void Bind(std::u8string_view path, nox::AssetManager& manager);
		bool Initialize();

		inline bool IsReady()const noexcept { return is_initialized_; }
		inline std::u8string_view GetPath()const noexcept { return path_; }

		inline nox::AssetManager& GetManager()const { return manager_.Get(); }
	protected:
		virtual bool OnInitialize(nox::io::BinaryReader& reader) = 0;
	private:
		nox::U8String path_;
		nox::util::InitOnceRef<nox::AssetManager> manager_;
		bool is_initialized_;
	};
}