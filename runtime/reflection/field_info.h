///	@file	field_info.h
///	@brief	field_info
#pragma once
#include	"type.h"

namespace nox::reflection
{
	class FieldInfo
	{
	public:
		inline	constexpr	std::u8string_view	GetName()const noexcept { return name_; }

		const class ClassInfo* GetOwnerClass(std::uint8_t typeID)const noexcept;

		template<concepts::ClassUnion T>
		const class ClassInfo* GetOwnerClass()const noexcept { return GetOwnerClass(util::GetUniqueTypeID<T>()); }

		inline	constexpr	std::uint32_t GetTypeID()const noexcept { return type_.GetTypeID(); }
		inline	constexpr	const Type& GetType()const noexcept { return type_; }
		inline	constexpr	AccessLevel	GetAccessLevel()const noexcept { return access_level_; }
		inline	constexpr	std::span<const class ReflectionObject*>	GetAttributeList()const noexcept { return std::span(attribute_ptr_table_, attribute_length_); }
	private:
		std::u8string_view name_;

		/// @brief 属性テーブル
		const class ReflectionObject** attribute_ptr_table_;

		/// @brief 属性テーブルの長さ
		std::uint8_t attribute_length_;

		/// @brief 自身のタイプ
		const Type& type_;

		const Type& owner_class_type_;

		AccessLevel access_level_;
	};
}