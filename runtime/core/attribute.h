//	Copyright (C) 2025 NOX ENGINE All rights reserved.

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
}