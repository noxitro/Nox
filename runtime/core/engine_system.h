// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	engine_system.h
/// @brief	engine_system
#pragma once
#include	"object.h"
#include	"module_entry_category.h"

namespace nox
{
	class Application;

	/// @brief エンジン規定のシステムクラス　モジュールエントリの実装で使用される
	class EngineSystem : public nox::Object
	{
		NOX_DECLARE_OBJECT(EngineSystem, nox::Object);
	protected:
		struct SystemPhase
		{
			void(nox::EngineSystem::* func)(nox::Application&);
			std::u8string_view name;
			nox::SystemPhaseType type;
		};

		template<nox::SystemPhaseType _PhaseType>
		struct PhaseDeclareImpl : public SystemPhase
		{
			template<class T>
			inline consteval explicit PhaseDeclareImpl(
				void(T::* func)(nox::Application&),
				std::u8string_view name
			)noexcept :
				nox::EngineSystem::SystemPhase{
					static_cast<void(nox::EngineSystem::*)(nox::Application&)>(func),
					name,
					_PhaseType
				}
			{
			}
		};
		
		using SystemPhaseInit = PhaseDeclareImpl<nox::SystemPhaseType::Init>;
		using SystemPhaseStart = PhaseDeclareImpl<nox::SystemPhaseType::Start>;
		using SystemPhaseUpdate = PhaseDeclareImpl<nox::SystemPhaseType::Update>;
		using SystemPhaseTerminate = PhaseDeclareImpl<nox::SystemPhaseType::Terminate>;

		struct PhaseRegister
		{
		private:
			static constexpr size_t k_max_dependency_length = 16;
		public:
			template<nox::SystemPhaseType _PhaseType, std::same_as<PhaseDeclareImpl<_PhaseType>>... _Deps>
				requires(sizeof...(_Deps) <= k_max_dependency_length)
			inline constexpr explicit PhaseRegister(
				const PhaseDeclareImpl<_PhaseType>& phase_,
				const _Deps&... deps
				)noexcept:
				phase_(phase_),
				dependencies_(MakeDependencies(
					phase_,
					std::make_index_sequence<k_max_dependency_length - sizeof...(_Deps)>{},
					deps...
				)),
				dependency_count_(sizeof...(_Deps))
			{
			}

		public:
			inline constexpr const SystemPhase& GetPhase()const noexcept { return phase_; }

			inline constexpr std::span<const std::reference_wrapper<const SystemPhase>> GetDependencies()const noexcept
			{
				return std::span(dependencies_.data(), dependency_count_);
			}

		private:
			template<class... _Deps, size_t... _PadIs>
			static inline constexpr auto MakeDependencies(
				const SystemPhase& sentinel,
				std::index_sequence<_PadIs...>,
				const _Deps&... deps
			) noexcept
			{
				return std::array<std::reference_wrapper<const SystemPhase>, k_max_dependency_length>{
					std::cref<const SystemPhase>(deps)...,
					((void)_PadIs, std::cref<const SystemPhase>(sentinel))...
				};
			}

		private:
			const SystemPhase& phase_;
			const std::array<std::reference_wrapper<const SystemPhase>, k_max_dependency_length> dependencies_;
			const nox::uint8 dependency_count_;
		};

	public:
		EngineSystem() {}

		virtual std::span<const nox::EngineSystem::PhaseRegister> GetPhaseRegisterList()const noexcept = 0;
	};
}