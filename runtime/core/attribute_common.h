//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

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
}

namespace nox
{
	template<>
	struct nox::reflection::IsIgnoreAttribute<nox::attr::DataMember, nox::attr::IgnoreDataMember> : std::true_type {};
}