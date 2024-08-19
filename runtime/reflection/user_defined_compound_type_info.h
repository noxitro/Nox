///	@file	user_defined_compound_type_info.h
///	@brief	user_defined_compound_type_info
#pragma once
#include	"type.h"
#include	"reflection_object.h"

namespace nox::reflection
{
	//	前方宣言
	class FunctionInfo;
	class VariableInfo;
	class EnumInfo;

	/// @brief ユーザー定義の複合型情報
	/// @details クラス、構造体、共用体が該当します
	/// UserDefinedCompoundTypeInfo
	class UserDefinedCompoundTypeInfo
	{
	public:
		inline constexpr explicit UserDefinedCompoundTypeInfo(
			const nox::reflection::Type& type,
			ReflectionStringView name,
			ReflectionStringView fullname,
			ReflectionStringView namespace_,
			const nox::reflection::Type& external_class_type,
			const std::reference_wrapper<const nox::reflection::Type>* base_type_list,
			const std::uint8_t base_type_length,
			const std::reference_wrapper<const nox::reflection::ReflectionObject>* attribute_list,
			const std::uint8_t attribute_length,
			const std::reference_wrapper<const nox::reflection::VariableInfo>* variable_list,
			const std::uint8_t variable_length,
			const std::reference_wrapper<const nox::reflection::FunctionInfo>* function_list,
			const std::uint8_t function_length,
			const std::reference_wrapper<const nox::reflection::EnumInfo>* enum_list,
			const std::uint8_t enum_length,
			const std::reference_wrapper<const nox::reflection::UserDefinedCompoundTypeInfo>* internal_type_list,
			const std::uint8_t internal_class_length
		)noexcept:
			type_(type),
			name_(name),
			fullname_(fullname),
			namespace_(namespace_),
			external_class_type_(external_class_type),
			base_type_list_(base_type_list),
			base_type_length_(base_type_length),
			attribute_list_(attribute_list),
			attribute_length_(attribute_length),
			variable_list_(variable_list),
			variable_length_(variable_length),
			function_list_(function_list),
			function_length_(function_length),
			internal_type_list_(internal_type_list),
			internal_class_length_(internal_class_length),
			enum_list_(enum_list),
			enum_length_(enum_length)
		{

		}

#pragma region アクセサ
		[[nodiscard]] inline	constexpr	bool	IsValid()const noexcept { return type_.IsValid(); }

		[[nodiscard]] inline	constexpr	ReflectionStringView GetName()const noexcept { return name_; }
		[[nodiscard]] inline	constexpr	ReflectionStringView GetFullName()const noexcept { return fullname_; }
		[[nodiscard]] inline	constexpr	ReflectionStringView GetNamespace()const noexcept { return namespace_; }


		[[nodiscard]] inline	constexpr	const nox::reflection::Type& GetType()const noexcept { return type_; }
		[[nodiscard]] inline	constexpr	const nox::reflection::Type& GetExternalType()const noexcept { return external_class_type_; }

		[[nodiscard]] inline	constexpr	std::uint8_t	GetAttributeListLength()const noexcept { return attribute_length_; }
		[[nodiscard]] inline	constexpr	const std::span<const std::reference_wrapper<const reflection::ReflectionObject>> GetAttributeList()const noexcept { return std::span(attribute_list_, attribute_length_); }
		[[nodiscard]] inline	constexpr	const nox::reflection::ReflectionObject& GetAttribute(const std::uint8_t index)const noexcept { return nox::util::At(attribute_list_, attribute_length_, index); }

		template<class T> requires
			(
				std::is_base_of_v<nox::reflection::ReflectionObject, T> && 
				std::is_base_of_v<nox::reflection::IAttribute, T> 
				)
		[[nodiscard]] inline	constexpr	const T* GetAttribute()const noexcept
		{
			for (std::int32_t i = 0; i < attribute_length_; ++i)
			{
				if (attribute_list_[i].get().GetType() == nox::reflection::Typeof<T>())
				{
					return static_cast<const T*>(&attribute_list_[i].get());
				}
			}
			return nullptr;
		}

		[[nodiscard]] inline constexpr std::uint8_t GetBaseTypeLength()const noexcept { return base_type_length_; }
		[[nodiscard]] inline constexpr const std::span<const std::reference_wrapper<const reflection::Type>> GetBaseTypeList()const noexcept { return std::span(base_type_list_, base_type_length_); }


		[[nodiscard]] inline constexpr std::uint8_t GetVariableLength()const noexcept { return variable_length_; }
		[[nodiscard]] inline constexpr const std::span<const std::reference_wrapper<const nox::reflection::VariableInfo>> GetVariableList()const noexcept { return std::span(variable_list_, variable_length_); }
		[[nodiscard]] inline constexpr std::uint8_t GetFunctionLength()const noexcept { return function_length_; }
		[[nodiscard]] inline constexpr const std::span<const std::reference_wrapper<const nox::reflection::FunctionInfo>> GetFunctionList()const noexcept { return std::span(function_list_, function_length_); }
		[[nodiscard]] inline constexpr std::uint8_t GetInternalClassLength()const noexcept { return internal_class_length_; }
		[[nodiscard]] inline constexpr const std::span<const std::reference_wrapper<const nox::reflection::UserDefinedCompoundTypeInfo>> GetInternalClassList()const noexcept { return std::span(internal_type_list_, internal_class_length_); }
		[[nodiscard]] inline constexpr std::uint8_t GetEnumLength()const noexcept { return enum_length_; }
		[[nodiscard]] inline	constexpr	std::span<const std::reference_wrapper<const nox::reflection::EnumInfo>> GetEnumInfoList()const noexcept { return std::span(enum_list_, enum_length_); }
		[[nodiscard]] inline constexpr const nox::reflection::EnumInfo& GetEnumInfo(std::uint8_t index)const noexcept { return nox::util::At(enum_list_, enum_length_, index); }

#pragma endregion

	private:
		/*inline constexpr UserDefinedCompoundTypeInfo(const UserDefinedCompoundTypeInfo&)noexcept = delete;
		inline constexpr UserDefinedCompoundTypeInfo(const UserDefinedCompoundTypeInfo&&)noexcept = delete;
		inline constexpr void operator =(const UserDefinedCompoundTypeInfo&)noexcept = delete;
		inline constexpr void operator =(const UserDefinedCompoundTypeInfo&&)noexcept = delete;

		inline constexpr void operator ==(const UserDefinedCompoundTypeInfo&)noexcept = delete;
		inline constexpr void operator ==(const UserDefinedCompoundTypeInfo&&)noexcept = delete;
		inline constexpr void operator !=(const UserDefinedCompoundTypeInfo&)noexcept = delete;
		inline constexpr void operator !=(const UserDefinedCompoundTypeInfo&&)noexcept = delete;*/
	private:
		/// @brief 継承型テーブルの長さ
		std::uint8_t base_type_length_;

		/// @brief 属性テーブルの長さ
		std::uint8_t attribute_length_;

		/// @brief 変数テーブルの長さ
		std::uint8_t variable_length_;

		/// @brief 関数テーブルの長さ
		std::uint8_t function_length_;

		/// @brief 内部クラスの長さ
		std::uint8_t internal_class_length_;

		/// @brief 列挙体の数
		std::uint8_t enum_length_;

		/// @brief 自身のタイプ
		const Type& type_;

		/// @brief 自身が所属するクラス
		const nox::reflection::Type&	external_class_type_;

		/// @brief 継承型テーブル
		const std::reference_wrapper<const nox::reflection::Type>* base_type_list_;

		/// @brief 属性テーブル
		const std::reference_wrapper<const nox::reflection::ReflectionObject>* attribute_list_;

		/// @brief 変数情報ポインタテーブル
		const std::reference_wrapper<const class nox::reflection::VariableInfo>* variable_list_;

		/// @brief 関数情報ポインタテーブル
		const std::reference_wrapper<const class nox::reflection::FunctionInfo>* function_list_;

		/// @brief 内部複合型テーブル
		const std::reference_wrapper<const nox::reflection::UserDefinedCompoundTypeInfo>* internal_type_list_;

		/// @brief 内部列挙体テーブル
		const std::reference_wrapper<const class nox::reflection::EnumInfo>* enum_list_;

		/// @brief 名前
		ReflectionStringView name_;

		/// @brief フルネーム
		ReflectionStringView fullname_;

		/// @brief 名前空間
		ReflectionStringView namespace_;
	};

	namespace detail
	{
		class UserDefinedCompoundTypeInfoInvalid : public nox::reflection::UserDefinedCompoundTypeInfo
		{
		public:
			inline constexpr UserDefinedCompoundTypeInfoInvalid()noexcept :
				nox::reflection::UserDefinedCompoundTypeInfo(
					nox::reflection::InvalidType,
					U"",
					U"",
					U"",
					nox::reflection::InvalidType,

					nullptr,
					0,
					nullptr,
					0,
					nullptr,
					0,
					nullptr,
					0,
					nullptr,
					0,
					nullptr,
					0
				)
			{}
		};

		constexpr UserDefinedCompoundTypeInfoInvalid InvalidUserDefinedCompoundTypeInfo{};

	/*	template<class T> requires(std::is_class_v<T> || std::is_union_v<T>)
		inline constexpr nox::reflection::UserDefinedCompoundTypeInfo CreateUserDefinedCompoundTypeInfo(
			nox::reflection::ReflectionStringView name,
			nox::reflection::ReflectionStringView fullname,
			nox::reflection::ReflectionStringView _namespace,
			const nox::reflection::UserDefinedCompoundTypeInfo& external_class_type,
			const std::reference_wrapper<const nox::reflection::UserDefinedCompoundTypeInfo>* base_type_list,
			std::uint8_t base_type_length,
			const std::reference_wrapper<const nox::reflection::ReflectionObject>* attribute_list,
			std::uint8_t attribute_length,
			const std::reference_wrapper<const nox::reflection::VariableInfo>* variable_list,
			std::uint8_t variable_length,
			const std::reference_wrapper<const nox::reflection::FunctionInfo>* function_list,
			std::uint8_t function_length,
			const std::reference_wrapper<const nox::reflection::UserDefinedCompoundTypeInfo>* internal_type_list,
			std::uint8_t internal_type_length,
			const std::reference_wrapper<const nox::reflection::EnumInfo>* enum_list,
			std::uint8_t enum_length


		)noexcept
		{
			return nox::reflection::UserDefinedCompoundTypeInfo(
				nox::reflection::Typeof<T>(),
				name,
				fullname,
				_namespace,
				external_class_type,
				base_type_list,
				base_type_length,
				attribute_list,
				attribute_length,
				variable_list,
				variable_length,
				function_list,
				function_length,
				internal_type_list,
				internal_type_length,
				enum_list,
				enum_length
			);
		}*/
	}
}