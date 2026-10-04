// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	asset.h
/// @brief	asset
#pragma once
#include	"object.h"

namespace nox
{
	class AssetManager;
	class AssetChunkLoader;
	class Asset : public nox::Object
	{
		NOX_DECLARE_OBJECT(Asset, nox::Object);
	public:
		inline Asset() : is_initialized_(false) {}

		void Bind(std::u8string_view path, nox::AssetManager& manager);

		/// @brief ネイティブリソースから初期化する
		/// @param native_path コンバート済みネイティブファイルのフルパス
		/// @return 初期化に成功したか
		bool Initialize(std::u8string_view native_path);

		inline bool IsReady()const noexcept { return is_initialized_; }
		inline std::u8string_view GetPath()const noexcept { return path_; }

		inline nox::AssetManager& GetManager()const { return manager_.Get(); }
	protected:
		virtual bool OnInitialize(nox::io::BinaryReader& reader) = 0;
	private:
		/// @brief assetrootからの相対パス
		nox::U8String path_;
		nox::util::InitOnceRef<nox::AssetManager> manager_;
		bool is_initialized_;
	};
}