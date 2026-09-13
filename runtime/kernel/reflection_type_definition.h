//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	type_definition.h
///	@brief	type_definition
#pragma once

#include	"advanced_type.h"
#include	"reflection_attribute.h"

namespace nox::reflection
{
	/// @brief タイプ種別
	enum class TypeKind : std::uint8_t
	{
		Unknown,
		Void,
		Bool,
		Char,
		//	SignedChar = Int8,
		//	UnsignedChar = Uint8,
		Char8,
		Char16,
		Char32,
		WideChar,
		Int8,
		UInt8,
		Int16,
		UInt16,
		Int32,
		UInt32,
		Int64,
		UInt64,
		//	compiler extension
		Int128,
		UInt128,
		//	end compiler extension
		Float,
		Double,
		LongDouble,
		//	c++23
		Float16,
		BFloat16,
		Float128,
		//	end c++23
		Long,
		UnsignedLong,
		Enum,
		ScopedEnum,
		Class,
		Union,
		Function,
		FunctionPointer,
		MemberFunctionPointer,
		MemberObjectPointer,
		Pointer,
		LValueReference,
		RValueReference,
		Array,
		UnboundedArray,
		Nullptr,
		_Max
	};

	/// @brief タイプ情報種別
	enum class TypeInfoKind : std::uint8_t
	{
		Invalid,
		Class,
		Enum,
		Global,
	};

	/// @brief 型修飾子
	enum class TypeAttributeFlag : std::uint64_t
	{
		None = 0,

		/// @brief const 修飾子
		Const = 1 << 0,

		/// @brief volatile 修飾子
		Volatile = 1 << 1,

		/// @brief final修飾子
		Final = 1 << 2,

		/// @brief abstract 修飾子
		Abstract = 1 << 3,

		/// @brief 符号なし
		Unsigned = 1 << 4,

		/// @brief 多相的
		Polymorphic = 1 << 5,

		/// @brief 集成体
		Aggregate = 1 << 6,

		/// @brief 破壊可能
		Destructible = 1 << 7,

		/// @brief 代入可能
		Assignable = 1 << 8,

		/// @brief 交換可能
		Swapable = 1 << 9,

		Construtible = 1 << 10,

		DefaultConstructible = 1 << 11,

		CopyConstructible = 1 << 12,

		MoveConstructible = 1 << 13,

		/// @brief インターフェースクラス nox::Interfaceを継承している
		Interface = 1 << 14,

		TrivialCopyable = 1 << 15,
	};

	/// @brief アクセスレベル
	enum class AccessLevel : std::uint8_t
	{
		Private,
		Protected,
		Public
	};

	/// @brief 関数属性情報
	enum class FunctionAttributeFlag : std::uint32_t
	{
		/// @brief 無し
		None = 0,

		/// @brief 純粋仮想関数
		Abstract = 1 << 0,

		/// @brief 仮想関数
		Virtual = 1 << 1,

		/// @brief 可変指定子
		Const = 1 << 2,

		/// @brief noexcept
		Noexcept = 1 << 3,

		/// @brief static
		Static = 1 << 4,

		/// @brief Volatile
		Volatile = 1 << 5,

		/// @brief 左辺値
		LvalueRef = 1 << 6,

		/// @brief 右辺値
		RvalueRef = 1 << 7,

		/// @brief inline
		Inline = 1 << 8,

		/// @brief override
		Override = 1 << 9,

		/// @brief constexpr
		Constexpr = 1 << 10,

		/// @brief デフォルトコンストラクタ
		DefaultConstructor = 1 << 11,

		/// @brief コピーコンストラクタ
		CopyConstructor = 1 << 12,

		/// @brief ムーブコンストラクタ
		MoveConstructor = 1 << 13,

		/// @brief ムーブ代入演算子
		MoveAssignment = 1 << 14,

		/// @brief コピー代入演算子
		CopyAssignment = 1 << 15,

		/// @brief consteval
		Consteval = 1 << 16,
	};

	/// @brief フィールド属性情報
	enum class VariableAttributeFlag : std::uint8_t
	{
		None = 0,

		/// @brief 静的
		Static = 1 << 0,

		/// @brief 定数
		Constexpr = 1 << 1,

		/// @brief 初期化定数
		Constinit = 1 << 2,
	};

	/// @brief 修飾子識別
	enum class Qualifier : std::uint16_t
	{
		None = 0,
		Const = 1 << 0,
		Noexcept = 1 << 2
	};

	enum class BindingFlag : std::uint16_t
	{
		Default,
		Public,
		Private,
		Static,
	};

	/// @brief 標準属性の定義
	enum class StandardAttrKind : std::uint8_t
	{
		/// @brief 不明
		Invalid,

		/// @brief [[nodiscard]]
		NoDiscard,

		/// @brief annotation
		Annotate,
	};

	/// @brief		完全修飾名の最大長
	/// @note		現状小さめに設定しているので、templateクラスだと1024にしないと足らないかも
	constexpr nox::uint16 k_max_fqn_length = 256;
}