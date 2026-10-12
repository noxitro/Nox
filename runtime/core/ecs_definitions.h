//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	ecs_definitions.h
///	@brief	ecs_definitions
#pragma once

/// @brief	ECS関係の型宣言に問題がないかコンパイル時チェックを行うマクロ（定義しなくても良い）
///			型定義の中で書くこと。
/// @details	クラスの中のラムダは private のメンバも見えるので、外からは見えないメソッドの有無を EcsAccessVerifier に渡す
///				(EntitySystem の OnUpdate / OnAdd / OnRemove が private だと、コンパイラによって登録されたりされなかったりするため)
/// @tparam Class チェック対象のクラス
#define NOX_ECS_DECLARE_VERIFY(Class)\
	private:\
		inline consteval void EcsVerify()const noexcept \
		{\
			static_assert(std::is_same_v<Class, std::remove_cvref_t<decltype(*this)>>, "Class type mismatch"); \
			static_assert(Class::StaticDeclareVerify());\
			static_assert(::nox::detail::EcsAccessVerifier<Class>::Verify(\
				[]<class U>(std::type_identity<U>)consteval noexcept\
				{\
					return ::nox::detail::EcsDeclaredMethods{ requires { &U::OnUpdate; }, requires { &U::OnAdd; }, requires { &U::OnRemove; } };\
				}));\
		}\
	public:

namespace nox::detail
{
	/// @brief	クラスの中から見た OnUpdate / OnAdd / OnRemove の有無 (private も含む)
	struct EcsDeclaredMethods final
	{
		bool has_update;
		bool has_add;
		bool has_remove;
	};

	/// @brief	NOX_ECS_DECLARE_VERIFY が、クラスの中からしか見えない情報を渡して確かめる口。種類ごとに特殊化する (既定は何もしない)
	template<class T>
	struct EcsAccessVerifier
	{
		template<class Probe>
		static consteval bool Verify(Probe)noexcept
		{
			return true;
		}
	};
}