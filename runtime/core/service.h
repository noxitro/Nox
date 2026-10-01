// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	service.h
/// @brief	service
/// @details Worldが所有する共有機能。Worldの破棄がそのままServiceの破棄になるため、
///          シングルトンのような特別な終了処理・再初期化を持たない。
///          System / EntityLogic は引数に Service* を書くだけで受け取れる。
#pragma once
#include	"ecs_definitions.h"

namespace nox
{
	class World;


	/// @brief	System,EntityLogic,Service間で参照可能な機能
	///			Systemなどと同じ実行ノードを持ち、Init,Update,Terminateの各フェーズを定義可能
	///			Phase関数の引数は、ServiceとExtraResrouceのみ定義可能
	class ServiceBase
	{
	public:
		enum class ServicePhaseType : nox::uint8
		{
			Init,
			Update,
			Terminate,
		};

	protected:
		struct Phase
		{
			void operator()()const
			{

			}

			ServicePhaseType type;
		};

		template<ServicePhaseType phase_type, auto Func>
		struct PhaseImpl : public Phase
		{

		};

		template<auto Func>
		using PhaseInit = PhaseImpl<ServicePhaseType::Init, Func>;
		template<auto Func>
		using PhaseUpdate = PhaseImpl<ServicePhaseType::Update, Func>;
		template<auto Func>
		using PhaseTerminate = PhaseImpl<ServicePhaseType::Terminate, Func>;

		template<class... Phases>
		struct PhaseRegister;
	};

	template<class T>
	class Service : public nox::ServiceBase
	{
	public:
		//	static constexpr auto GetPhaseList()noexcept {}

	protected:
		static consteval bool StaticDeclareVerify()noexcept
		{
			static_assert(std::is_polymorphic_v<T> == false, "Service must not be polymorphic");
			// GetPhaseList()を実装しているかチェック
			static_assert(requires { T::GetPhaseList(); }, "Service must implement GetPhaseList()");
			return true;
		}
	};

	namespace detail
	{
		template<class T>
		struct Phase
	}

	class SampleService final : public nox::Service<SampleService>
	{
		NOX_ECS_DECLARE_VERIFY(SampleService);

	public:
		static constexpr void GetPhaseList()noexcept
		{

		}
	};

	namespace detail
	{
		/// @brief Worldに登録済みのServiceを型で引く。
		/// @details entity_query.hがworld.hに依存しないための橋渡し。
		[[nodiscard]] nox::Service* TryGetServiceOfWorld(nox::World& world, const nox::reflection::Type& type)noexcept;
	}
}
