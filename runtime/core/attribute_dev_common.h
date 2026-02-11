//	Copyright (C) 2026 NOX ENGINE All rights reserved.

///	@file	attribute_dev_common.h
///	@brief	開発用の属性定義
#pragma once
#include	"attribute.h"

namespace nox::attr::dev
{
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


	class Hide : public nox::attr::Attribute
	{
		NOX_DECLARE_OBJECT(Hide, nox::attr::Attribute);
	};

}