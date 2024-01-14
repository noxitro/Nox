///	@file	class_info.h
///	@brief	class_info
#pragma once
#include	"type.h"

namespace nox::reflection
{
	//	前方宣言
	class MethodInfo;
	class FieldInfo;
	class EnumInfo;

	/// @brief クラス型情報
	class ClassInfo
	{
	public:
#pragma region アクセサ
		[[nodiscard]] inline	constexpr	std::u8string_view GetName()const noexcept { return name_; }
		[[nodiscard]] inline	constexpr	std::u8string_view GetFullName()const noexcept { return fullname_; }
		[[nodiscard]] inline	constexpr	std::u8string_view GetNamespace()const noexcept { return namespace_; }
		[[nodiscard]] inline	constexpr	std::uint32_t GetNamespaceID()const noexcept { return util::crc32(namespace_); }


		[[nodiscard]] inline	constexpr	const Type& GetType()const noexcept { return type_; }
		[[nodiscard]] inline	constexpr	std::uint32_t GetTypeID()const noexcept { return type_.GetTypeID(); }

		/// @brief 継承型リストを取得
		/// @return span型
		[[nodiscard]] inline constexpr const std::span<const reflection::Type* const> GetBaseTypeList()const noexcept {
			return std::span(base_type_table_, base_type_length_);
		}

		/// @brief 継承型を取得
		[[nodiscard]] inline constexpr const reflection::Type* GetBaseType(std::uint8_t typeID)const noexcept {
			return util::SafeDeref(util::FindIf(base_type_table_, base_type_table_ + base_type_length_, [typeID](const reflection::Type* x)noexcept {return x->GetTypeID() == typeID; }));
		}

		template<concepts::ClassUnion T>
		[[nodiscard]] inline constexpr const reflection::Type* GetBaseType(std::uint8_t typeID)const noexcept { return GetBaseType(util::GetUniqueTypeID<T>()); }

		[[nodiscard]] inline constexpr std::uint8_t GetBaseTypeLength()const noexcept { return base_type_length_; }

		[[nodiscard]] inline constexpr const std::span<const ClassInfo* const> GetInternalClassList()const noexcept { return std::span(internal_class_ptr_table_, internal_class_length_); }

		[[nodiscard]] inline constexpr std::uint8_t GetInternalClassLength()const noexcept { return internal_class_length_; }

		[[nodiscard]] inline constexpr const ClassInfo* GetInternalClass(std::uint8_t typeID)const noexcept {
			return util::SafeDeref(util::FindIf(internal_class_ptr_table_, internal_class_ptr_table_ + internal_class_length_, [typeID](const ClassInfo* x)noexcept {return x->GetTypeID() == typeID; }));
		}

		template<concepts::ClassUnion T>
		[[nodiscard]] inline constexpr const ClassInfo* GetInternalClass()const noexcept { return GetInternalClass(util::GetUniqueTypeID<T>()); }

		[[nodiscard]] inline constexpr const std::span<const class FieldInfo* const> GetFieldList()const noexcept { return std::span(field_ptr_table_, field_length_); }
		[[nodiscard]] const class FieldInfo* GetField(std::uint8_t typeID)const noexcept;
#pragma endregion

	private:
		inline constexpr ClassInfo(const ClassInfo&)noexcept = delete;
		inline constexpr ClassInfo(const ClassInfo&&)noexcept = delete;
		inline constexpr void operator =(const ClassInfo&)noexcept = delete;
		inline constexpr void operator =(const ClassInfo&&)noexcept = delete;

		inline constexpr void operator ==(const ClassInfo&)noexcept = delete;
		inline constexpr void operator ==(const ClassInfo&&)noexcept = delete;
		inline constexpr void operator !=(const ClassInfo&)noexcept = delete;
		inline constexpr void operator !=(const ClassInfo&&)noexcept = delete;
	private:
		/// @brief 名前
		std::u8string_view name_;

		/// @brief フルネーム
		std::u8string_view fullname_;

		/// @brief 名前空間
		std::u8string_view namespace_;

		/// @brief 自身のタイプ
		const Type& type_;

		/// @brief 自身が所属するクラスのID
		const std::uint32_t	external_class_type_id_;

		/// @brief 継承型テーブル
		const reflection::Type*const* base_type_table_;

		/// @brief 属性テーブル
		const class ReflectionObject* const* attribute_ptr_table_;

		/// @brief 変数情報ポインタテーブル
		const class FieldInfo* const* field_ptr_table_;

		/// @brief 関数情報ポインタテーブル
		const class MethodInfo* const* method_ptr_table_;

		/// @brief 内部クラステーブル
		const class ClassInfo* const* internal_class_ptr_table_;

		/// @brief 内部列挙体テーブル
		const class EnumInfo* const* enum_ptr_table;

		/// @brief 継承型テーブルの長さ
		std::uint8_t base_type_length_;

		/// @brief 属性テーブルの長さ
		std::uint8_t attribute_length_;

		/// @brief 変数テーブルの長さ
		std::uint8_t field_length_;

		/// @brief 関数テーブルの長さ
		std::uint8_t method_length_;

		/// @brief 内部クラスの長さ
		std::uint8_t internal_class_length_;

		/// @brief 列挙体の数
		std::uint8_t enum_length_;
	};

	constexpr auto sizeo = sizeof(ClassInfo);
}