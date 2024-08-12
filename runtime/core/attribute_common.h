//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	attribute_common.h
///	@brief	attribute_common
#pragma once
#include	"object.h"
namespace nox
{
	namespace attr
	{
		/// @brief 属性基底
		class Attribute : public Object, public nox::reflection::IAttribute
		{
			NOX_DECLARE_OBJECT(Attribute, Object);
		public:
			inline constexpr Attribute()noexcept {}
		};

		/// @brief シリアライズ対象
		class DataMember : public Attribute
		{
			NOX_DECLARE_OBJECT(DataMember, Attribute);
		};

		/// @brief シリアライズ非対象
		class IgnoreDataMember : public Attribute
		{
			NOX_DECLARE_OBJECT(IgnoreDataMember, Attribute);
		};

		class Hide : public Attribute
		{
			NOX_DECLARE_OBJECT(Hide, Attribute);
		};

		/// @brief リソースクラス
		class Resource : public Attribute
		{
			NOX_DECLARE_OBJECT(Resource, Attribute);
		public:
			inline constexpr explicit Resource(std::u8string_view extension, int32 version)noexcept:
				extension_(extension),
				version_(version)
			{}

		private:
			std::u8string_view extension_;
			int32 version_;
		};
		
		namespace dev
		{
			class DisplayName : public Attribute
			{
				NOX_DECLARE_OBJECT(DisplayName, Object);
			private:

			public:
				inline	constexpr explicit DisplayName(const std::u32string_view display_name)noexcept :
					display_name_(display_name) {}

			private:
				const std::u32string_view display_name_;
			};

			class Description : public Attribute
			{
				NOX_DECLARE_OBJECT(Description, Object);
			public:
				inline	constexpr explicit Description(const std::u32string_view description)noexcept :
					description_(description) {}

			private:
				const std::u32string_view description_;
			};
		}
	}

	template<>
	struct nox::reflection::IsIgnoreAttribute<attr::DataMember, attr::IgnoreDataMember> : std::true_type {};

}