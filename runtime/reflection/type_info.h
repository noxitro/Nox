//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	type_info.h
///	@brief	type_info
#pragma once
namespace nox::reflection
{
	class TypeInfo
	{
	public:
		inline constexpr const nox::reflection::Type& GetRawType()const noexcept { return raw_type_; }

	protected:
		inline constexpr explicit TypeInfo(const nox::reflection::Type& raw_type)noexcept :
			raw_type_(raw_type)
		{

		}
	private:
		const nox::reflection::Type& raw_type_;
	};

	class PrimitiveTypeInfo : public TypeInfo
	{
		
	};

	class FunctionTypeInfo : public TypeInfo
	{
	public:

	private:
		const nox::reflection::TypeInfo& return_type_;
	};
}