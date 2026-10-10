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

	class RequireComponentsBase : public nox::attr::Attribute
	{
	protected:
		constexpr explicit RequireComponentsBase(
			const nox::reflection::Type* const* components,
			const nox::uint8 component_count)noexcept:
			components_(components),
			component_count_(component_count)
		{}
	public:
		inline constexpr const std::span<const nox::reflection::Type* const> GetComponents() const noexcept
		{
			return std::span<const nox::reflection::Type* const>(components_, component_count_);
		}

	private:
		const nox::reflection::Type* const* components_;
		const nox::uint8 component_count_;
	};

	/// @brief RequireComponentsの属性
	template<class... Components>
	class RequireComponents final: public nox::attr::RequireComponentsBase
	{
	public:
		constexpr RequireComponents()noexcept :
			nox::attr::RequireComponentsBase(components_.data(), static_cast<nox::uint8>(components_.size()))	
		{

		}

	private:
		static constexpr std::array<const nox::reflection::Type*, sizeof...(Components)> components_ = { &nox::reflection::Typeof<Components>()... };
	};
}

namespace nox
{
	template<>
	struct nox::reflection::IsIgnoreAttribute<nox::attr::DataMember, nox::attr::IgnoreDataMember> : std::true_type {};
}