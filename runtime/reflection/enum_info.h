///	@file	enum_info.h
///	@brief	enum_info
#pragma once
#include	"type.h"
#include	"reflection_object.h"

namespace nox::reflection
{
	/// @brief Enum値情報
	class EnumeratorInfo
	{
	public:
		//inline constexpr EnumeratorInfo()noexcept :
		//	value_int8_(0),
		//	name_(u8""),
		//	attribute_ptr_table_(nullptr),
		//	attribute_length_(0) {}

		inline constexpr explicit EnumeratorInfo(
			const std::int8_t value,
			const ReflectionStringView name,
			const ReflectionStringView fullname,
			const std::reference_wrapper<const ReflectionObject>* attribute_list,
			const std::uint8_t attribute_length
		)noexcept :
			value_int8_(value),
			name_(name),
			fullname_(fullname),
			attribute_list_(attribute_list),
			attribute_length_(attribute_length) {}

		inline constexpr explicit EnumeratorInfo(
			const std::uint8_t value,
			const ReflectionStringView name,
			const ReflectionStringView fullname,
			const std::reference_wrapper<const ReflectionObject>* attribute_list,
			const std::uint8_t attribute_length
		)noexcept :
			value_uint8_(value),
			name_(name),
			fullname_(fullname),
			attribute_list_(attribute_list),
			attribute_length_(attribute_length) {}

		inline constexpr explicit EnumeratorInfo(
			const std::int16_t value,
			const ReflectionStringView name,
			const ReflectionStringView fullname,
			const std::reference_wrapper<const ReflectionObject>* attribute_list,
			const std::uint8_t attribute_length
		)noexcept :
			value_int16_(value),
			name_(name),
			fullname_(fullname),
			attribute_list_(attribute_list),
			attribute_length_(attribute_length) {}

		inline constexpr explicit EnumeratorInfo(
			const std::uint16_t value,
			const ReflectionStringView name,
			const ReflectionStringView fullname,
			const std::reference_wrapper<const ReflectionObject>* attribute_list,
			const std::uint8_t attribute_length
		)noexcept :
			value_uint16_(value),
			name_(name),
			fullname_(fullname),
			attribute_list_(attribute_list),
			attribute_length_(attribute_length) {}

		inline constexpr explicit EnumeratorInfo(
			const std::int32_t value,
			const ReflectionStringView name,
			const ReflectionStringView fullname,
			const std::reference_wrapper<const ReflectionObject>* attribute_list,
			const std::uint8_t attribute_length
		)noexcept :
			value_int32_(value),
			name_(name),
			fullname_(fullname),
			attribute_list_(attribute_list),
			attribute_length_(attribute_length) {}

		inline constexpr explicit EnumeratorInfo(
			const std::uint32_t value,
			const ReflectionStringView name,
			const ReflectionStringView fullname,
			const std::reference_wrapper<const ReflectionObject>* attribute_list,
			const std::uint8_t attribute_length
		)noexcept :
			value_uint32_(value),
			name_(name),
			fullname_(fullname),
			attribute_list_(attribute_list),
			attribute_length_(attribute_length) {}

		inline constexpr explicit EnumeratorInfo(
			const std::int64_t value,
			const ReflectionStringView name,
			const ReflectionStringView fullname,
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
			const ReflectionStringView name,
			const ReflectionStringView fullname,
			const std::reference_wrapper<const ReflectionObject>* attribute_list,
			const std::uint8_t attribute_length
		)noexcept :
			value_uint64_(value),
			name_(name),
			fullname_(fullname),
			attribute_list_(attribute_list),
			attribute_length_(attribute_length) {}

		inline constexpr explicit EnumeratorInfo(
			const char8_t value,
			const ReflectionStringView name,
			const ReflectionStringView fullname,
			const std::reference_wrapper<const ReflectionObject>* attribute_list,
			const std::uint8_t attribute_length
		)noexcept :
			value_char8_(value),
			name_(name),
			fullname_(fullname),
			attribute_list_(attribute_list),
			attribute_length_(attribute_length) {}

		inline constexpr explicit EnumeratorInfo(
			const char16_t value,
			const ReflectionStringView name,
			const ReflectionStringView fullname,
			const std::reference_wrapper<const ReflectionObject>* attribute_list,
			const std::uint8_t attribute_length
		)noexcept :
			value_char16_(value),
			name_(name),
			fullname_(fullname),
			attribute_list_(attribute_list),
			attribute_length_(attribute_length) {}

		inline constexpr explicit EnumeratorInfo(
			const char32_t value,
			const ReflectionStringView name,
			const ReflectionStringView fullname,
			const std::reference_wrapper<const ReflectionObject>* attribute_list,
			const std::uint8_t attribute_length
		)noexcept :
			value_char32_(value),
			name_(name),
			fullname_(fullname),
			attribute_list_(attribute_list),
			attribute_length_(attribute_length) {}

		inline constexpr explicit EnumeratorInfo(
			const wchar_t value,
			const ReflectionStringView name,
			const ReflectionStringView fullname,
			const std::reference_wrapper<const ReflectionObject>* attribute_list,
			const std::uint8_t attribute_length
		)noexcept :
			value_wchar16_(value),
			name_(name),
			fullname_(fullname),
			attribute_list_(attribute_list),
			attribute_length_(attribute_length) {}

		inline constexpr explicit EnumeratorInfo(
			const char value,
			const ReflectionStringView name,
			const ReflectionStringView fullname,
			const std::reference_wrapper<const ReflectionObject>* attribute_list,
			const std::uint8_t attribute_length
		)noexcept :
			value_char_(value),
			name_(name),
			fullname_(fullname),
			attribute_list_(attribute_list),
			attribute_length_(attribute_length) {}

		inline constexpr explicit EnumeratorInfo(
			const bool value,
			const ReflectionStringView name,
			const ReflectionStringView fullname,
			const std::reference_wrapper<const ReflectionObject>* attribute_list,
			const std::uint8_t attribute_length
		)noexcept :
			value_bool_(value),
			name_(name),
			fullname_(fullname),
			attribute_list_(attribute_list),
			attribute_length_(attribute_length) {}

		/// @brief 名前を取得
		/// @return 
		[[nodiscard]] inline constexpr ReflectionStringView GetName()const noexcept { return name_; }

		/// @brief フルネームを取得
		/// @return 
		[[nodiscard]] inline constexpr ReflectionStringView GetFullName()const noexcept { return fullname_; }
		
		/// @brief 整数型を指定して値を取得
		template<std::integral T>
		[[nodiscard]] inline constexpr T GetValue()const noexcept { return nox::reflection::EnumeratorInfo::GetValueImpl<T>(); }

		/// @brief enumの型を指定して値を取得
		/// @tparam T Enum型
		/// @return 値
		template<concepts::Enum T>
		[[nodiscard]] inline constexpr T GetValue()const noexcept { return static_cast<T>(nox::reflection::EnumeratorInfo::GetValueImpl<std::underlying_type_t<T>>()); }

	private:
		template<std::integral T>
		inline constexpr T GetValueImpl()const noexcept;

		
		
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
		/// @brief 属性テーブルの長さ
		std::uint8_t attribute_length_;

		/// @brief 属性テーブル
		const std::reference_wrapper<const ReflectionObject>*const attribute_list_;

		/// @brief 名前
		ReflectionStringView name_;
		ReflectionStringView fullname_;
	};

	template<>	inline	constexpr std::int8_t nox::reflection::EnumeratorInfo::GetValueImpl<std::int8_t>()const noexcept { return value_int8_; }
	template<>	inline	constexpr std::uint8_t nox::reflection::EnumeratorInfo::GetValueImpl<std::uint8_t>()const noexcept { return value_uint8_; }
	template<>	inline	constexpr std::int16_t nox::reflection::EnumeratorInfo::GetValueImpl<std::int16_t>()const noexcept { return value_int16_; }
	template<>	inline	constexpr std::uint16_t nox::reflection::EnumeratorInfo::GetValueImpl<std::uint16_t>()const noexcept { return value_uint16_; }
	template<>	inline	constexpr std::int32_t nox::reflection::EnumeratorInfo::GetValueImpl<std::int32_t>()const noexcept { return value_int32_; }
	template<>	inline	constexpr std::uint32_t nox::reflection::EnumeratorInfo::GetValueImpl<std::uint32_t>()const noexcept { return value_uint32_; }
	template<>	inline	constexpr std::int64_t nox::reflection::EnumeratorInfo::GetValueImpl<std::int64_t>()const noexcept { return value_int64_; }
	template<>	inline	constexpr std::uint64_t nox::reflection::EnumeratorInfo::GetValueImpl<std::uint64_t>()const noexcept { return value_uint64_; }
	template<>	inline	constexpr char nox::reflection::EnumeratorInfo::GetValueImpl<char>()const noexcept { return value_char_; }
	template<>	inline	constexpr char8_t nox::reflection::EnumeratorInfo::GetValueImpl<char8_t>()const noexcept { return value_char8_; }
	template<>	inline	constexpr char16_t nox::reflection::EnumeratorInfo::GetValueImpl<char16_t>()const noexcept { return value_char16_; }
	template<>	inline	constexpr char32_t nox::reflection::EnumeratorInfo::GetValueImpl<char32_t>()const noexcept { return value_char32_; }
	template<>	inline	constexpr wchar_t nox::reflection::EnumeratorInfo::GetValueImpl<wchar_t>()const noexcept { return value_wchar16_; }
	template<>	inline	constexpr bool nox::reflection::EnumeratorInfo::GetValueImpl<bool>()const noexcept { return value_bool_; }


	/// @brief Enum情報
	class EnumInfo
	{
		inline constexpr explicit EnumInfo(const EnumInfo&)noexcept = delete;
		inline constexpr explicit EnumInfo(const EnumInfo&&)noexcept = delete;
	public:
		inline consteval explicit EnumInfo(
			const nox::reflection::Type& underlying_type,
			ReflectionStringView name,
			ReflectionStringView fullname,
			ReflectionStringView _namespace,
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
			underlying_type_(underlying_type)
		{}

		/// @brief 基底型を取得
		inline	constexpr	const nox::reflection::Type& GetUnderlyingType()const noexcept { return underlying_type_; }

		inline	constexpr	ReflectionStringView	GetName()const noexcept { return name_; }
		inline	constexpr	ReflectionStringView	GetFullName()const noexcept { return fullname_; }
		inline	constexpr	ReflectionStringView	GetNamespace()const noexcept { return namespace_; }

		inline	constexpr	std::uint8_t	GetAttributeLength()const noexcept { return attribute_length_; }
		inline	constexpr	std::span<const std::reference_wrapper<const ReflectionObject>> GetAttributeList()const noexcept { return std::span(attribute_list_, attribute_length_); }

		inline	constexpr	std::span<const std::reference_wrapper<const EnumeratorInfo>> GetVariableList()const noexcept { return std::span(variable_list_, variable_length_); }
		inline	constexpr	std::uint8_t	GetVariableLength()const noexcept { return variable_length_; }

		template<std::integral T>
		inline	constexpr	std::optional<std::span<T>> GetValueList(std::span<T> buffer)const noexcept
		{
			if (underlying_type_ != nox::reflection::Typeof<T>())
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
			if (underlying_type_ != nox::reflection::Typeof<T>())
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
		const nox::reflection::Type& underlying_type_;

		/// @brief 名前
		const	ReflectionStringView	name_;

		/// @brief 型名
		const	ReflectionStringView	fullname_;

		/// @brief 名前空間
		const ReflectionStringView namespace_;
	};
}