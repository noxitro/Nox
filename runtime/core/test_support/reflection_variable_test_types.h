// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	reflection_variable_test_types.h
/// @brief	nox::reflection::VariableInfo の getter / setter を検証するためのテスト用型。
/// @details リフレクション生成器は解析の起点 (reflection_generated/reflect.cpp) から
///          辿れる型しか見ない。テスト専用の型を生成対象に載せるための既存の作法に従い、
///          このヘッダは core/test_support/test_types.h からだけインクルードする
///          (test_types.h 自体が reflect.cpp と reflection_generated/pch.h から見えている)。
///
///          ここに置く型は「リフレクションの getter / setter が何を返すべきか」を
///          網羅するためだけのもので、エンジンの機能には一切使わない。
#pragma once

#include	"../../kernel/basic_type.h"

#include	<atomic>

namespace nox::test::reflection
{
	/// @brief 代入演算子を持たない型。setter が黙って無視されることの確認用。
	struct NonAssignableValue
	{
		nox::int32 value = 0;

		NonAssignableValue() = default;
		NonAssignableValue(const NonAssignableValue&) = default;
		NonAssignableValue& operator=(const NonAssignableValue&) = delete;
	};

	/// @brief ビットフィールドの幅を表す列挙。幅 4 のビットフィールドの型として使う。
	enum class VariableAccessKind : nox::uint32
	{
		None = 0,
		First = 1,
		Second = 2,
		Last = 15,
	};

	/// @brief VariableInfo のアクセス系 API を一通り踏むためのテスト用型。
	struct VariableAccessTarget
	{
		/// @brief コピー可能な通常のメンバ。getter が値を返し、setter が書き込めること。
		nox::int32 plain_value = 1;

		/// @brief コピー不可のメンバ。getter が nullopt を返すこと。
		std::atomic_bool atomic_flag{ false };

		/// @brief const メンバ。setter が黙って無視されること。
		const nox::int32 const_value = 7;

		/// @brief 代入不可のメンバ。setter が黙って無視されること。
		nox::test::reflection::NonAssignableValue non_assignable{};

		/// @brief 幅 1 のビットフィールド。
		nox::uint32 bit_flag : 1 = 0u;

		/// @brief 幅 4 のビットフィールド。
		nox::test::reflection::VariableAccessKind bit_kind : 4 = nox::test::reflection::VariableAccessKind::None;

		/// @brief 静的メンバ。static 版の getter / setter の対象。
		static inline nox::int32 static_value = 0;
	};

	/// @brief 参照メンバを持つ型。
	/// @details 参照メンバにはメンバポインタを作れないため、生成器は
	///          CreateVariableInfoMemberRef 側の経路を通る。ビットフィールドと同じ経路。
	struct ReferenceMemberTarget
	{
		nox::int32& reference_value;
	};
}
