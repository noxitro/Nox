//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	type_info.h
///	@brief	type_info
#pragma once
#include	"type.h"

namespace nox::reflection
{
	/// @brief 型情報
	class TypeInfo
	{
	public:
		inline	constexpr explicit TypeInfo(
			ReflectionStringView name,
			ReflectionStringView fullname,
			ReflectionStringView _namespace,
			const Type& type)noexcept
			: name_(name),
			fullname_(fullname),
			namespace_(_namespace),
			type_(type)
		{}

		[[nodiscard]] inline	constexpr ReflectionStringView GetName()const noexcept { return name_; }
		[[nodiscard]] inline	constexpr ReflectionStringView GetFullName()const noexcept { return fullname_; }
		[[nodiscard]] inline	constexpr ReflectionStringView GetNamespace()const noexcept { return namespace_; }
		[[nodiscard]] inline	constexpr const Type& GetThisType()const noexcept { return type_; }
	private:
		/// @brief 名前
		ReflectionStringView name_;
		/// @brief フルネーム
		ReflectionStringView fullname_;
		/// @brief 名前空間
		ReflectionStringView namespace_;
		/// @brief タイプ
		const Type& type_;
	};
}