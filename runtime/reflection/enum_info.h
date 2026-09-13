// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

///	@file	enum_info.h
///	@brief	enum_info
#pragma once

namespace nox::reflection
{
	//	前方宣言
	class ClassInfo;
	class ReflectionObject;

	/// @brief Enum値情報
	class EnumeratorInfo
	{
	public:
		inline constexpr explicit EnumeratorInfo(
			const std::int64_t value,
			const std::u8string_view name,
			const std::u8string_view fullname,
			const std::reference_wrapper<const ReflectionObject>* attribute_list,
			const std::uint8_t attribute_length
		)noexcept :
			value_int64_(value),
			name_(name),
			fullname_(fullname),
			attribute_list_(attribute_list),
			attribute_length_(attribute_length) {}

		inline constexpr explicit EnumeratorInfo(
			const std::uint64_t value,
			const std::u8string_view name,
			const std::u8string_view fullname,
			const std::reference_wrapper<const ReflectionObject>* attribute_list,
			const std::uint8_t attribute_length
		)noexcept :
			value_uint64_(value),
			name_(name),
			fullname_(fullname),
			attribute_list_(attribute_list),
			attribute_length_(attribute_length) {}

		/// @brief 名前を取得
		/// @return 
		[[nodiscard]] inline constexpr std::u8string_view GetName()const noexcept { return name_; }

		/// @brief フルネームを取得
		/// @return 
		[[nodiscard]] inline constexpr std::u8string_view GetFullName()const noexcept { return fullname_; }
		
		/// @brief 整数型を指定して値を取得
		template<std::integral T>
		[[nodiscard]] inline constexpr T GetValue()const noexcept 
		{ 
			if constexpr (std::is_signed_v<T>)
			{
				return value_int64_;
			}
			else
			{
				return value_uint64_;
			}
		}

		/// @brief enumの型を指定して値を取得
		/// @tparam T Enum型
		/// @return 値
		template<concepts::Enum T>
		[[nodiscard]] inline constexpr T GetValue()const noexcept { return static_cast<T>(this->GetValue<std::underlying_type_t<T>>()); }


	private:
		
	private:
		//	enumが使える値の型
		union
		{
			std::int64_t value_int64_;
			std::uint64_t value_uint64_;
		};
		/// @brief 属性テーブルの長さ
		std::uint8_t attribute_length_;

		/// @brief 属性テーブル
		const std::reference_wrapper<const ReflectionObject>*const attribute_list_;

		/// @brief 名前
		std::u8string_view name_;
		std::u8string_view fullname_;
	};

	/// @brief Enum情報
	class EnumInfo
	{
		inline constexpr explicit EnumInfo(const EnumInfo&)noexcept = delete;
		inline constexpr explicit EnumInfo(const EnumInfo&&)noexcept = delete;
	public:
		inline consteval explicit EnumInfo(
			const nox::reflection::Type& type,
			std::u8string_view name,
			std::u8string_view fullname,
			std::u8string_view _namespace,
			const nox::reflection::AccessLevel access_level,
			const std::reference_wrapper<const ReflectionObject>*const attribute_list,
			std::uint8_t attribute_length,
			const std::reference_wrapper<const EnumeratorInfo>*const variable_list,
			std::uint8_t variable_length
		)noexcept :
			name_(name),
			fullname_(fullname),
			namespace_(_namespace),
			access_level_(access_level),
			attribute_list_(attribute_list),
			attribute_length_(attribute_length),
			variable_list_(variable_list),
			variable_length_(variable_length),
			type_(type)
		{}

		/// @brief 基底型を取得
		inline	constexpr	const nox::reflection::Type& GetType()const noexcept { return type_; }
		inline	constexpr	const nox::reflection::Type& GetUnderlyingType()const noexcept { return type_.GetUnderlyingType(); }

		inline	constexpr	std::u8string_view	GetName()const noexcept { return name_; }
		inline	constexpr	std::u8string_view	GetFullName()const noexcept { return fullname_; }
		inline	constexpr	std::u8string_view	GetNamespace()const noexcept { return namespace_; }

		inline	constexpr	std::uint8_t	GetAttributeLength()const noexcept { return attribute_length_; }
		inline	constexpr	std::span<const std::reference_wrapper<const ReflectionObject>> GetAttributeList()const noexcept { return std::span(attribute_list_, attribute_length_); }

		inline	constexpr	std::span<const std::reference_wrapper<const EnumeratorInfo>> GetVariableList()const noexcept { return std::span(variable_list_, variable_length_); }
		inline	constexpr	std::uint8_t	GetVariableLength()const noexcept { return variable_length_; }

		template<std::integral T>
		inline	constexpr	std::optional<std::span<T>> GetValueList(std::span<T> buffer)const noexcept
		{
			if (GetUnderlyingType() != nox::reflection::Typeof<T>())
			{
				return std::nullopt;
			}

			if (buffer.size() < variable_length_)
			{
				return std::nullopt;
			}

			return nox::reflection::EnumInfo::GetValueListImpl<T>(buffer);
		}

		template<std::integral T>
		inline	constexpr	std::optional<nox::Vector<T>> GetValueList()const noexcept
		{
			if (GetUnderlyingType() != nox::reflection::Typeof<T>())
			{
				return std::nullopt;
			}

			nox::Vector<T> buffer(variable_length_);
			if (!nox::reflection::EnumInfo::GetValueListImpl<T>(std::span(buffer.data(), buffer.size())))
			{
				return std::nullopt;
			}

			return buffer;
		}

	private:
		template<std::integral T>
		inline	constexpr	std::optional<std::span<T>> GetValueListImpl(std::span<T> buffer)const noexcept
		{
			for (std::uint8_t i = 0; i < variable_length_; ++i)
			{
				buffer[i] = variable_list_[i].get().GetValue<T>();
			}
			return buffer;
		}

	private:
		/// @brief 属性テーブルの長さ
		const std::uint8_t attribute_length_;

		/// @brief Enum値情報テーブルの長さ
		const std::uint8_t	variable_length_;

		/// @brief アクセスレベル
		const AccessLevel access_level_;

		/// @brief 属性テーブル
		const std::reference_wrapper<const ReflectionObject>*const attribute_list_;

		/// @brief Enum値情報テーブル
		const std::reference_wrapper<const EnumeratorInfo>*const variable_list_;

		/// @brief 型情報
		const nox::reflection::Type& type_;

		/// @brief 名前
		const	std::u8string_view	name_;

		/// @brief 型名
		const	std::u8string_view	fullname_;

		/// @brief 名前空間
		const std::u8string_view namespace_;
	};
}