//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	resource.h
///	@brief	resource
#pragma once
#include	"object.h"
#include	"attribute.h"

namespace nox::attr
{
	/// @brief リソースクラス
	class Resource : public nox::attr::Attribute
	{
		NOX_DECLARE_OBJECT(Resource, nox::attr::Attribute);
	public:
		inline constexpr explicit Resource(std::u8string_view extension, nox::uint32 version)noexcept :
			extension_(extension),
			version_(version)
		{
		}

		inline constexpr std::u8string_view GetExtension()const noexcept { return extension_; }
		inline constexpr nox::uint32 GetVersion()const noexcept { return version_; }

	private:
		/// @brief 拡張子
		std::u8string_view extension_;

		/// @brief リソースバージョン
		nox::uint32 version_;
	};
}

namespace nox
{
	/// @brief		リソースの基底クラス
	/// @details	ネイティブリソース(バイナリ)の構成:
	///				
	///				[header]
	class Resource : public nox::Object
	{
		NOX_DECLARE_OBJECT(Resource, nox::Object);
	protected:
		struct alignas(16) Header
		{
			nox::uint32 magic;
			nox::uint16 version;
			nox::uint16 flags;
		};
	public:
		Resource() noexcept = default;
		~Resource()override = default;

		bool Initialize(nox::U8StringView path);
		std::u8string_view GetPath()const noexcept { return path_; }
		bool IsInitialized()const noexcept { return is_initialized_; }

	protected:
		virtual bool OnInitialize(nox::io::BinaryReader& reader) = 0;

	private:
		nox::U8String path_;
		bool is_initialized_ = false;
	};
}