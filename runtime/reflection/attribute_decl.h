//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	attribute_decl.h
///	@brief	属性定義
#pragma once
#include	"attribute.h"
#include	"reflection_object.h"

namespace nox::reflection::attr
{
	/// @brief 	標準属性
	class StandardAttribute : public nox::reflection::ReflectionObject, public IAttribute
	{
		NOX_DECLARE_REFLECTION_OBJECT(nox::reflection::attr::StandardAttribute);
	public:
		inline constexpr explicit StandardAttribute(nox::reflection::AttrKind kind)noexcept:
			kind_(kind)
		{}

	private:
		nox::reflection::AttrKind kind_;
	};
}