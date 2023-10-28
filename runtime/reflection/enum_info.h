///	@file	enum_info.h
///	@brief	enum_info
#pragma once
#include	"type.h"

namespace nox::reflection
{
	class ReflectionObject;
	/// @brief Enum値情報
	class EnumVariableInfo
	{
	public:
		inline constexpr EnumVariableInfo()noexcept :
			value_int8_(0),
			name_(u8""),
			attribute_ptr_table_(nullptr),
			attribute_length_(0) {}

		inline constexpr explicit EnumVariableInfo(
			const std::int8_t value,
			const std::u8string_view variableName,
			const class ReflectionObject* const* const attributePtrAry,
			const u8 attributeLength
		)noexcept :
			value_int8_(value),
			name_(variableName),
			attribute_ptr_table_(attributePtrAry),
			attribute_length_(attributeLength) {}

		inline constexpr explicit EnumVariableInfo(
			const std::uint8_t value,
			const std::u8string_view variableName,
			const class ReflectionObject* const* const attributePtrAry,
			const u8 attributeLength
		)noexcept :
			value_uint8_(value),
			name_(variableName),
			attribute_ptr_table_(attributePtrAry),
			attribute_length_(attributeLength) {}

		inline constexpr explicit EnumVariableInfo(
			const std::int16_t value,
			const std::u8string_view variableName,
			const class ReflectionObject* const* const attributePtrAry,
			const u8 attributeLength
		)noexcept :
			value_int16_(value),
			name_(variableName),
			attribute_ptr_table_(attributePtrAry),
			attribute_length_(attributeLength) {}

		inline constexpr explicit EnumVariableInfo(
			const std::uint16_t value,
			const std::u8string_view variableName,
			const class ReflectionObject* const* const attributePtrAry,
			const u8 attributeLength
		)noexcept :
			value_uint16_(value),
			name_(variableName),
			attribute_ptr_table_(attributePtrAry),
			attribute_length_(attributeLength) {}

		inline constexpr explicit EnumVariableInfo(
			const std::int32_t value,
			const std::u8string_view variableName,
			const class ReflectionObject* const* const attributePtrAry,
			const u8 attributeLength
		)noexcept :
			value_int32_(value),
			name_(variableName),
			attribute_ptr_table_(attributePtrAry),
			attribute_length_(attributeLength) {}

		inline constexpr explicit EnumVariableInfo(
			const std::uint32_t value,
			const std::u8string_view variableName,
			const class ReflectionObject* const* const attributePtrAry,
			const u8 attributeLength
		)noexcept :
			value_uint32_(value),
			name_(variableName),
			attribute_ptr_table_(attributePtrAry),
			attribute_length_(attributeLength) {}

		inline constexpr explicit EnumVariableInfo(
			const std::int64_t value,
			const std::u8string_view variableName,
			const class ReflectionObject* const* const attributePtrAry,
			const u8 attributeLength
		)noexcept :
			value_int64_(value),
			name_(variableName),
			attribute_ptr_table_(attributePtrAry),
			attribute_length_(attributeLength) {}

		inline constexpr explicit EnumVariableInfo(
			const std::uint64_t value,
			const std::u8string_view variableName,
			const class ReflectionObject* const* const attributePtrAry,
			const u8 attributeLength
		)noexcept :
			value_uint64_(value),
			name_(variableName),
			attribute_ptr_table_(attributePtrAry),
			attribute_length_(attributeLength) {}

		inline constexpr explicit EnumVariableInfo(
			const char8_t value,
			const std::u8string_view variableName,
			const class ReflectionObject* const* const attributePtrAry,
			const u8 attributeLength
		)noexcept :
			value_char8_(value),
			name_(variableName),
			attribute_ptr_table_(attributePtrAry),
			attribute_length_(attributeLength) {}

		inline constexpr explicit EnumVariableInfo(
			const char16_t value,
			const std::u8string_view variableName,
			const class ReflectionObject* const* const attributePtrAry,
			const u8 attributeLength
		)noexcept :
			value_char16_(value),
			name_(variableName),
			attribute_ptr_table_(attributePtrAry),
			attribute_length_(attributeLength) {}

		inline constexpr explicit EnumVariableInfo(
			const char32_t value,
			const std::u8string_view variableName,
			const class ReflectionObject* const* const attributePtrAry,
			const u8 attributeLength
		)noexcept :
			value_char32_(value),
			name_(variableName),
			attribute_ptr_table_(attributePtrAry),
			attribute_length_(attributeLength) {}

		inline constexpr explicit EnumVariableInfo(
			const wchar_t value,
			const std::u8string_view variableName,
			const class ReflectionObject* const* const attributePtrAry,
			const u8 attributeLength
		)noexcept :
			value_wchar16_(value),
			name_(variableName),
			attribute_ptr_table_(attributePtrAry),
			attribute_length_(attributeLength) {}

		inline constexpr explicit EnumVariableInfo(
			const char value,
			const std::u8string_view variableName,
			const class ReflectionObject* const* const attributePtrAry,
			const u8 attributeLength
		)noexcept :
			value_char_(value),
			name_(variableName),
			attribute_ptr_table_(attributePtrAry),
			attribute_length_(attributeLength) {}

		/*inline constexpr explicit EnumVariableInfo(
			const unsigned char value,
			const std::u8string_view variableName,
			const Attribute* const* const attributePtrAry,
			const u8 attributeLength
		)noexcept :
			mValueUChar(value),
			name_(variableName),
			attribute_ptr_table_(attributePtrAry),
			attribute_length_(attributeLength) {}

		inline constexpr explicit EnumVariableInfo(
			const signed char value,
			const std::u8string_view variableName,
			const Attribute* const* const attributePtrAry,
			const u8 attributeLength
		)noexcept :
			mValueSChar(value),
			name_(variableName),
			attribute_ptr_table_(attributePtrAry),
			attribute_length_(attributeLength) {}*/

		inline constexpr explicit EnumVariableInfo(
			const bool value,
			const std::u8string_view variableName,
			const class ReflectionObject* const* const attributePtrAry,
			const u8 attributeLength
		)noexcept :
			value_bool_(value),
			name_(variableName),
			attribute_ptr_table_(attributePtrAry),
			attribute_length_(attributeLength) {}

		/**
		 * @brief enum値の名前を取得
		 * @return 名前
		*/
		[[nodiscard]] inline constexpr std::u8string_view GetVariableName()const noexcept { return name_; }
		/**
		 * @brief 整数型を指定して値を取得
		 * @return 整数値
		*/
		template<std::integral T>
		[[nodiscard]] inline constexpr T GetValue()const noexcept { return GetValueImpl<T>(); }

		/**
		 * @brief enumの型を指定して値を取得
		 * @return enum値
		*/
		template<concepts::Enum T>
		[[nodiscard]] inline constexpr T GetValue()const noexcept { return static_cast<T>(GetValueImpl<std::underlying_type_t<T>>()); }

	private:
		template<std::integral T>
		inline constexpr T GetValueImpl()const noexcept;

		template<>	inline	constexpr std::int8_t GetValueImpl<std::int8_t>()const noexcept { return value_int8_; }
		template<>	inline	constexpr std::uint8_t GetValueImpl<std::uint8_t>()const noexcept { return value_uint8_; }
		template<>	inline	constexpr std::int16_t GetValueImpl<std::int16_t>()const noexcept { return value_int16_; }
		template<>	inline	constexpr std::uint16_t GetValueImpl<std::uint16_t>()const noexcept { return value_uint16_; }
		template<>	inline	constexpr std::int32_t GetValueImpl<std::int32_t>()const noexcept { return value_int32_; }
		template<>	inline	constexpr std::uint32_t GetValueImpl<std::uint32_t>()const noexcept { return value_uint32_; }
		template<>	inline	constexpr std::int64_t GetValueImpl<std::int64_t>()const noexcept { return value_int64_; }
		template<>	inline	constexpr std::uint64_t GetValueImpl<std::uint64_t>()const noexcept { return value_uint64_; }
		template<>	inline	constexpr char GetValueImpl<char>()const noexcept { return value_char_;; }
		//template<>	inline	constexpr unsigned char GetValueImpl<unsigned char>()const noexcept { return mValueUChar; }
		//template<>	inline	constexpr signed char GetValueImpl<signed char>()const noexcept { return mValueSChar; }
		template<>	inline	constexpr char8_t GetValueImpl<char8_t>()const noexcept { return value_char8_; }
		template<>	inline	constexpr char16_t GetValueImpl<char16_t>()const noexcept { return value_char16_; }
		template<>	inline	constexpr char32_t GetValueImpl<char32_t>()const noexcept { return value_char32_; }
		template<>	inline	constexpr wchar_t GetValueImpl<wchar_t>()const noexcept { return value_wchar16_; }
		template<>	inline	constexpr bool GetValueImpl<bool>()const noexcept { return value_bool_; }

	private:
		//	enumが使える値の型
		union
		{
			std::int8_t value_int8_ = 0;
			std::uint8_t value_uint8_;

			std::int16_t value_int16_;
			std::uint16_t value_uint16_;

			std::int32_t value_int32_;
			std::uint32_t value_uint32_;

			std::int64_t value_int64_;
			std::uint64_t value_uint64_;

			char value_char_;
			char8_t value_char8_;
			char16_t value_char16_;
			char32_t value_char32_;
			wchar_t value_wchar16_;
			bool value_bool_;
		};

		/**
		 * @brief 名前
		*/
		std::u8string_view name_;

		/// @brief 属性テーブル
		const class ReflectionObject* const* attribute_ptr_table_;

		/// @brief 属性テーブルの長さ
		std::uint8_t attribute_length_;
	};
	class EnumInfo
	{
	public:

	private:
		std::u8string_view	fullname_;

		/// @brief 属性テーブル
		const class ReflectionObject* const* attribute_ptr_table_;

		/// @brief 属性テーブルの長さ
		std::uint8_t attribute_length_;

		const Type& type_;
	};


}