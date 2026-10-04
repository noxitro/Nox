//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	entity_logic_attribute_legacy.h
///	@brief	entity_logic_attribute
///	@note	旧実装 (nox::legacy)。新しい ECS への書き直しが終わるまでの退避先で、移行後にこのファイルごと消す。
#pragma once
#include	"attribute.h"
#include	"system_phase_type.h"

namespace nox::legacy::attr
{
	/// @brief		EntityLogicの更新メソッドとして購読させる。
	/// @details	この属性を付けたメソッドだけが、リフレクション生成コードの作る
	///				nox::legacy::EntityLogicMethodTable に載る。フェーズはここで指定した値が
	///				生成コード側でコンパイル時に読み出される(生成器は引数を解釈しない)。
	class EntityLogicMethod : public nox::attr::Attribute
	{
		NOX_DECLARE_OBJECT(EntityLogicMethod, nox::attr::Attribute);
	public:
		inline constexpr explicit EntityLogicMethod(const nox::SystemPhaseType phase)noexcept :
			phase_(phase)
		{
		}

		[[nodiscard]] inline constexpr nox::SystemPhaseType GetPhase()const noexcept { return phase_; }

	private:
		const nox::SystemPhaseType phase_;
	};
}
