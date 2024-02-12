///	@file	global_info.h
///	@brief	global_info
#pragma once

namespace nox::reflection
{
	///// @brief グローバル空間の
	//class GlobalInfo
	//{
	//public:
	//	inline constexpr GlobalInfo(
	//		ReflectionStringView _namespace,
	//		const class FieldInfo* const* field_ptr_table,
	//		std::uint8_t field_length,
	//		const class FunctionInfo* const* method_ptr_table,
	//		std::uint8_t method_length,
	//		const class EnumInfo* const* enum_ptr_table,
	//		std::uint8_t enum_length
	//	)noexcept :
	//		namespace_(_namespace),
	//		field_ptr_table_(field_ptr_table),
	//		function_ptr_table_(method_ptr_table),
	//		enum_ptr_table_(enum_ptr_table),
	//		field_length_(field_length),
	//		function_length_(method_length),
	//		enum_length_(enum_length)
	//	{}

	//	inline	constexpr	ReflectionStringView	GetNamespace()const noexcept { return namespace_; }

	//private:
	//	/// @brief 名前空間
	//	ReflectionStringView namespace_;

	//	/// @brief 変数情報ポインタテーブル
	//	const class FieldInfo* const* field_ptr_table_;

	//	/// @brief 関数情報ポインタテーブル
	//	const class FunctionInfo* const* function_ptr_table_;

	//	/// @brief 内部列挙体テーブル
	//	const class EnumInfo* const* enum_ptr_table_;

	//	/// @brief 変数テーブルの長さ
	//	std::uint8_t field_length_;

	//	/// @brief 関数テーブルの長さ
	//	std::uint8_t function_length_;

	//	/// @brief 列挙体の数
	//	std::uint8_t enum_length_;
	//};

	//namespace detail
	//{
	//	
	//}
}