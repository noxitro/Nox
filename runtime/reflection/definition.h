///	@file	definition.h
///	@brief	definition
#pragma once
#include	<string_view>

namespace nox::reflection
{
	using ReflectionCharType = char32_t;
	using ReflectionStringView = std::basic_string_view<char32_t>;


	/// @brief タイプ種別
	enum class TypeKind : std::uint8_t
	{
		/**
		 * @brief 不明
		*/
		Invalid,

		/**
		 * @brief void
		*/
		Void,

		/**
		 * @brief Boolean
		*/
		Bool,

		/**
		 * @brief 8ビット符号付き整数
		*/
		Int8,

		/**
		 * @brief 8ビット符号なし整数
		*/
		Uint8,

		Char,
		SChar,
		UChar,


		/**
		 * @brief
		*/
		Char8,

		/**
		 * @brief 16ビット符号付き整数
		*/
		Int16,

		/**
		 * @brief 16ビット符号なし整数
		*/
		Uint16,

		Int64,
		UInt64,

		/**
		 * @brief
		*/
		Char16,

		/**
		 * @brief
		*/
		Wchar16,

		/**
		 * @brief 32ビット符号付き整数
		*/
		Int32,

		/**
		 * @brief 32ビット符号なし整数
		*/
		Uint32,


		/**
		 * @brief
		*/
		Char32,

		/**
		 * @brief 半精度浮動小数点数
		*/
		F32,

		/**
		 * @brief 倍精度浮動小数点数
		*/
		F64,

		/**
		 * @brief 列挙型
		*/
		Enum,

		/**
		 * @brief スコープを持つ列挙型
		*/
		ScopedEnum,

		/**
		 * @brief クラス
		*/
		Class,

		/**
		 * @brief 共用体
		*/
		Union,

		/**
		 * @brief 関数
		*/
		Function,

		/**
		 * @brief メンバ関数
		*/
		MemberFunction,

		/**
		 * @brief ラムダ式
		*/
		Lambda,

		/**
		 * @brief キャプチャありラムダ式
		*/
		CaptureLambda,

		_Max
	};

	/**
	 * @brief タイプ情報種別
	*/
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

	/**
	 * @brief タイプ属性
	*/
	enum class TypeAttributeFlag : std::uint16_t
	{
		None = 0,

		/**
		 * @brief const 修飾子
		*/
		Const = 1 << 0,

		/**
		 * @brief volatile 修飾子
		*/
		Volatile = 1 << 1,

		/**
		 * @brief ポインタ
		*/
		Pointer = 1 << 2,

		/**
		 * @brief 左辺参照
		*/
		LvalueReference = 1 << 3,

		/**
		 * @brief 右辺参照
		*/
		RvalueReference = 1 << 4,

		/**
		 * @brief final修飾子
		*/
		Final = 1 << 5,

		/**
		 * @brief abstract 修飾子
		*/
		Abstract = 1 << 6,

		/**
		 * @brief 配列
		*/
		Array = 1 << 7,

		/**
		 * @brief 要素数が判明していない配列
		*/
		UnboundedArray = 1 << 8,

		/**
		 * @brief 符号なし
		*/
		Unsigned = 1 << 9,

		/**
		 * @brief const pointer修飾子
		*/
		ConstPointer = 1 << 10,

		/**
		 * @brief const lvalue reference修飾子
		*/
		ConstLvalueReference = 1 << 11,

		/**
		 * @brief const rvalue reference修飾子
		*/
		ConstRvalueReference = 1 << 12,
	};

	/**
	 * @brief 型修飾子
	*/
	enum class TypeQualifierFlag : std::uint8_t
	{
		None = 0,

		/**
		 * @brief const 修飾子
		*/
		Const = 1 << 0,

		/**
		 * @brief volatile 修飾子
		*/
		Volatile = 1 << 1,

		/// @brief 参照　修飾子
		Reference = 1 << 2
	};

	/**
	 * @brief アクセスレベル
	*/
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
	enum class MethodAttributeFlag : std::uint16_t
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


		ConstructorP = 1 << 12,

		ConstructorNewArray = 1 << 13
	};

	/**
	 * @brief メソッドの種類
	*/
	enum class MethodType : std::uint8_t
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

	/**
	 * @brief フィールド属性情報
	*/
	enum class FieldAttributeFlag : std::uint8_t
	{
		None = 0,

		/**
		 * @brief メンバー
		*/
		Member = 1 << 0,
	};

	/**
	 * @brief 修飾子識別
	*/
	enum class Qualifier : std::uint16_t
	{
		None = 0,
		Const = 1 << 0,
		Noexcept = 1 << 2
	};



	
}