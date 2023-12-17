//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	attribute_common.h
///	@brief	attribute_common
#pragma once
#include	"object.h"
namespace nox
{
	class Attribute : public Object, nox::reflection::IAttribute
	{
		NOX_DECLARE_OBJECT(Attribute, Object);
	public:
		inline constexpr Attribute()noexcept {}
	};

	class DataMember : public Attribute
	{
		NOX_DECLARE_OBJECT(DataMember, Object);
	};

	namespace dev
	{
		class DisplayName : public Attribute
		{
			NOX_DECLARE_OBJECT(DisplayName, Object);
		private:

		public:
			inline	constexpr explicit DisplayName(const std::u16string_view display_name)noexcept :
				display_name_(display_name) {}

		private:
			const std::u16string_view display_name_;
		};
	}
}