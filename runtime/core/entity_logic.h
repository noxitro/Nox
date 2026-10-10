//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	entity_logic.h
///	@brief	entity_logic
#pragma once
#include	"entity.h"
#include	"entity_options.h"

namespace nox
{
	namespace util
	{
	}

	namespace detail
	{
		class EntityLogicBase : public nox::detail::UpdaterNodeOwnerBase
		{
		public:
			/// @brief メソッドの呼び出しフェーズ。EntityLogicの更新メソッドはこのいずれかで呼ばれる
			enum class Trigger : nox::uint8
			{
				Update,
				Add,
				Remove,
			};

		protected:
			template<auto Method, nox::detail::EntityLogicBase::Trigger TriggerValue, class... Options>
			struct Register final
			{
				constexpr Register()noexcept = delete;
				static constexpr nox::detail::EntityLogicBase::Trigger trigger = TriggerValue;
			};

			constexpr EntityLogicBase()noexcept = default;
		};
	}

	/// @brief メンバ変数を持てる振る舞いクラス
	/// @tparam T 
	/// @tparam ...Options RunAfter / RunBefore / RequireComponents / ExcludeComponents などのオプション
	template<class T, class... Options>
	class EntityLogic : public nox::detail::EntityLogicBase
	{
	protected:
		static consteval bool StaticDeclareVerify()noexcept
		{
			static_assert(std::is_polymorphic_v<T> == false, "EntityLogicは仮想関数を持てません");
			static_assert(nox::is_tuple_like_v<typename T::RegisterList>, "RegisterListが定義されていません");
			return true;
		}
	};
}