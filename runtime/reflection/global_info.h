///	@file	global_info.h
///	@brief	global_info
#pragma once

namespace nox::reflection
{
	/// @brief グローバル空間の
	class GlobalInfo
	{
	private:
		std::u8string_view assembly_;

		std::u8string_view namespace_;

		/// @brief 変数情報ポインタテーブル
		const class FieldInfo* const* field_ptr_table_;

		/// @brief 関数情報ポインタテーブル
		const class MethodInfo* const* method_ptr_table_;

		/// @brief 内部列挙体テーブル
		const class EnumInfo* const* enum_ptr_table;

		/// @brief 変数テーブルの長さ
		std::uint8_t field_length_;

		/// @brief 関数テーブルの長さ
		std::uint8_t method_length_;

		/// @brief 列挙体の数
		std::uint8_t enum_length_;
	};
}