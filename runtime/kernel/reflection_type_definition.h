//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	type_definition.h
///	@brief	type_definition
#pragma once

#include	"advanced_type.h"

namespace nox::reflection
{
#if defined(NOX_REFLECTION_BUILD_CHAR)
	/// @brief リフレクションシステムで扱う文字の型
	using ReflectionCharType = char;
#elif	defined(NOX_REFLECTION_BUILD_WCHAR)
	/// @brief リフレクションシステムで扱う文字の型
	using ReflectionCharType = wchar_t;
#elif	defined(NOX_REFLECTION_BUILD_CHAR8)
	/// @brief リフレクションシステムで扱う文字の型
	using ReflectionCharType = char8_t;
#elif	defined(NOX_REFLECTION_BUILD_CHAR16)
	/// @brief リフレクションシステムで扱う文字の型
	using ReflectionCharType = char16_t;
#elif	defined(NOX_REFLECTION_BUILD_CHAR32)
	/// @brief リフレクションシステムで扱う文字の型
	using ReflectionCharType = char32_t;
#else
	/// @brief リフレクションシステムで扱う文字の型
	using ReflectionCharType = char32_t;
#endif // 

#undef	NOX_REFLECTION_BUILD_CHAR
#undef	NOX_REFLECTION_BUILD_WCHAR
#undef	NOX_REFLECTION_BUILD_CHAR8
#undef	NOX_REFLECTION_BUILD_CHAR16
#undef	NOX_REFLECTION_BUILD_CHAR32

	/// @brief リフレクションシステムで扱うstring_viewの型
	using ReflectionStringView = std::basic_string_view<ReflectionCharType>;

	/// @brief タイプ種別
	enum class TypeKind : std::uint8_t
	{
		/// @brief 
		Invalid,

		/// @brief 
		Void,

		/// @brief nullptr_t
		NullPtr,

		/// @brief 
		Bool,

		/**
		 * @brief 8ビット符号付き整数
		*/
		Int8,

		/// @brief 8ビット符号なし整数
		Uint8,

		/// @brief 文字
		Char,

		/// @brief 符号付き文字
		SChar,

		/// @brief 符号なし文字
		UChar,

		/// @brief 8ビット符号付き整数
		Char8,

		/// @brief 16ビット符号付き整数
		Int16,

		/**
		 * @brief 16ビット符号なし整数
		*/
		Uint16,

		/// @brief 
		Int64,

		/// @brief 
		UInt64,

		/// @brief 
		Char16,

		/// @brief 
		Wchar16,

		/// @brief 32ビット符号付き整数
		Int32,

		/// @brief  32ビット符号なし整数
		Uint32,


		/// @brief 
		Char32,

		/// @brief 半精度浮動小数点数
		Float,

		/// @brief 倍精度浮動小数点数
		Double,

		/// @brief 列挙型
		Enum,

		/// @brief スコープを持つ列挙型
		ScopedEnum,

		/// @brief クラス
		Class,

		/// @brief 共用体
		Union,

		/// @brief 関数
		Delegate,

		/// @brief メンバ関数
		MemberFunction,

		/// @brief ラムダ式
		Lambda,

		/// @brief キャプチャありラムダ式
		CaptureLambda,

		/// @brief 配列
		Array,

		/// @brief 要素数が判明していない配列
		UnboundedArray,

		/// @brief ポインタ
		Pointer,

		/// @brief 左辺参照
		LvalueReference,

		/// @brief 右辺参照
		RvalueReference,

		_Max
	};

	/// @brief タイプ情報種別
	enum class TypeInfoKind : std::uint8_t
	{
		/**
		 * @brief 不明
		*/
		Invalid,

		/**
		 * @brief クラス
		*/
		Class,

		/**
		 * @brief 列挙隊
		*/
		Enum,

		/**
		 * @brief	グローバル
		 * @details	関数、変数
		*/
		Global,
	};

	/// @brief 型修飾子
	enum class TypeQualifierFlag : std::uint16_t
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
		Polymorphic = 1 << 5
	};

	///// @brief 型修飾子
	//enum class TypeQualifierFlag : std::uint8_t
	//{
	//	None = 0,

	//	/// @brief const 修飾子
	//	Const = 1 << 0,

	//	/// @brief volatile 修飾子
	//	Volatile = 1 << 1,

	//	/// @brief 参照　修飾子
	//	Reference = 1 << 2
	//};

	/// @brief アクセスレベル
	enum class AccessLevel : std::uint8_t
	{
		/**
		 * @brief private
		*/
		Private,

		/**
		 * @brief protected
		*/
		Protected,

		/**
		 * @brief public
		*/
		Public
	};

	/// @brief 関数属性情報
	enum class FunctionAttributeFlag : std::uint16_t
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

		/// @brief Constructor
		Constructor = 1 << 11,

		/// @brief コピーコンストラクタ
		CopyConstructor = 1 << 12,

		/// @brief ムーブコンストラクタ
		MoveConstructor = 1 << 13,
	};

	/// @brief メソッドの種類
	enum class FunctionType : std::uint8_t
	{
		/**
		 * @brief 通常の関数
		*/
		Default,

		/**
		 * @brief コンストラクタ ( new operator )
		*/
		Constructor,
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
	enum class AttrKind : std::uint8_t
	{
		/// @brief 不明
		Invalid,
	};
}