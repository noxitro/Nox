//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	ecs_definitions.h
///	@brief	ecs_definitions
#pragma once
#include	<concepts>

/// @brief	ECS関係の型宣言に問題がないかコンパイル時チェックを行うマクロ（定義しなくても良い）
///			型定義の中で書くこと。
/// @tparam Class チェック対象のクラス
#define NOX_ECS_DECLARE_VERIFY(Class)\
	private:\
		inline consteval void EcsVerify()const noexcept \
		{\
			static_assert(std::is_same_v<Class, std::remove_cvref_t<decltype(*this)>>, "Class type mismatch"); \
			static_assert(Class::StaticDeclareVerify());\
		}\
	public:

namespace nox
{
	class World;
}

namespace nox::detail
{

}