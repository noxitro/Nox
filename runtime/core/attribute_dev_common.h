//	Copyright (C) 2026 NOX ENGINE All rights reserved.

///	@file	attribute_dev_common.h
///	@brief	開発用の属性定義
#pragma once
#include	"attribute.h"

namespace nox::attr::dev
{
	/// @brief edtorでの表示名
	class DisplayName : public nox::attr::Attribute
	{
		NOX_DECLARE_OBJECT(DisplayName, nox::attr::Attribute);
	private:

	public:
		inline	constexpr explicit DisplayName(const std::u8string_view display_name)noexcept :
			display_name_(display_name) {
		}

	private:
		const std::u8string_view display_name_;
	};

	class Description : public nox::attr::Attribute
	{
		NOX_DECLARE_OBJECT(Description, nox::attr::Attribute);
	public:

		inline	constexpr explicit Description(const std::u8string_view description)noexcept :
			description_(description) {
		}

	private:
		const std::u8string_view description_;
	};

	/// @brief インスペクタへ非公開にする属性
	class Hide : public nox::attr::Attribute
	{
		NOX_DECLARE_OBJECT(Hide, nox::attr::Attribute);
	};

	/// @brief		c++関数をeditorでc#プロパティとして扱うことを表明する属性
	/// @details	property_name_を空文字列で渡した場合、Set, Get, Isを除去したメンバ変数を探しに行きます
	class 
		NOX_ATTR_TYPE(::nox::attr::AttributeUsage(nox::attr::AttributeTargets::Function))
		PropertySetter : public nox::attr::Attribute
	{
		NOX_DECLARE_OBJECT(PropertySetter, nox::attr::Attribute);
	public:
		inline constexpr PropertySetter()noexcept:
			property_name_(u8"") {
		}

		inline constexpr explicit PropertySetter(std::u8string_view name)noexcept:
			property_name_(name) {
		}

	private:
		const std::u8string_view property_name_;
	};

	/// @brief		c++関数をeditorでc#プロパティとして扱うことを表明する属性
	/// @details	property_name_を空文字列で渡した場合、Set, Get, Isを除去したメンバ変数を探しに行きます
	class
		NOX_ATTR_TYPE(::nox::attr::AttributeUsage(nox::attr::AttributeTargets::Function))
		PropertyGetter : public nox::attr::Attribute
	{
		NOX_DECLARE_OBJECT(PropertyGetter, nox::attr::Attribute);
	public:
		inline constexpr PropertyGetter()noexcept :
			property_name_(u8"") {
		}

		inline constexpr explicit PropertyGetter(std::u8string_view name)noexcept :
			property_name_(name) {
		}

	private:
		const std::u8string_view property_name_;
	};

	class Property : public nox::attr::Attribute
	{
		NOX_DECLARE_OBJECT(Property, nox::attr::Attribute);
	public:

	};
}