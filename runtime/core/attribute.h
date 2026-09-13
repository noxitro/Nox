//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	attribute.h
///	@brief	attribute
#pragma once
#include	"object.h"

namespace nox::attr
{
	/// @brief 属性基底
	class Attribute : public Object, public nox::reflection::IAttribute
	{
		NOX_DECLARE_OBJECT(Attribute, Object);
	public:
		inline constexpr Attribute()noexcept {}
		inline constexpr ~Attribute()override {}
	};

	enum class AttributeTargets : nox::uint16
	{
		Class = 1 << 0,
		Union = 1 << 1,
		Function = 1 << 2,
		Variable = 1 << 3,
		All = 0xFFFF
	};

	/// @brief 属性の使用方法
	class AttributeUsage : public nox::attr::Attribute
	{
		NOX_DECLARE_OBJECT(AttributeUsage, nox::attr::Attribute);
	public:
		inline constexpr explicit AttributeUsage(nox::attr::AttributeTargets targets)noexcept :
			target_(targets)
		{
		}

		inline constexpr nox::attr::AttributeTargets GetAttributeTarget()const noexcept { return target_; }

	private:
		const nox::attr::AttributeTargets target_;
	};
}