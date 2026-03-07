//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	attribute_common.h
///	@brief	パッケージ環境（非開発環境）に含める属性
#pragma once
#include	"attribute.h"

namespace nox::attr
{
	/// @brief シリアライズ対象
	class DataMember : public nox::attr::Attribute
	{
		NOX_DECLARE_OBJECT(DataMember, nox::attr::Attribute);
	public:
		inline constexpr DataMember()noexcept {}
		inline constexpr ~DataMember() noexcept override {}
	};

	/// @brief シリアライズ非対象
	class IgnoreDataMember : public nox::attr::Attribute
	{
		NOX_DECLARE_OBJECT(IgnoreDataMember, nox::attr::Attribute);
	};

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
	template<>
	struct nox::reflection::IsIgnoreAttribute<nox::attr::DataMember, nox::attr::IgnoreDataMember> : std::true_type {};
}