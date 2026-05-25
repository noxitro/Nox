// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	system.h
/// @brief	system
#pragma once
#include	"system_phase_type.h"
#include	"object.h"

namespace nox
{
	class World;

	/// @brief managed systemの基底クラス
	class SystemBase : public nox::Object
	{
		friend class World;
		NOX_DECLARE_OBJECT(SystemBase, nox::Object);
	protected:
		struct SystemPhase
		{
			void(nox::SystemBase::* func)(nox::World&);
			std::u8string_view name;
			nox::SystemPhaseType type;
		};

		template<nox::SystemPhaseType _PhaseType>
		struct SystemPhaseImpl : public SystemPhase
		{
			template<class T>
			inline constexpr explicit SystemPhaseImpl(
				void(T::* func)(nox::World&),
				std::u8string_view name
			)noexcept :
				nox::SystemBase::SystemPhase{
					static_cast<void(nox::SystemBase::*)(nox::World&)>(func),
					name,
					_PhaseType
			}
			{
			}
		};

		using SystemPhaseInit = SystemPhaseImpl<nox::SystemPhaseType::Init>;
		using SystemPhaseStart = SystemPhaseImpl<nox::SystemPhaseType::Start>;
		using SystemPhaseUpdate = SystemPhaseImpl<nox::SystemPhaseType::Update>;
		using SystemPhaseTerminate = SystemPhaseImpl<nox::SystemPhaseType::Terminate>;

		struct PhaseRegister
		{
		private:
			static constexpr size_t k_max_dependency_length = 16;
			using PhaseRelationInitializer = std::initializer_list<std::reference_wrapper<const SystemPhase>>;
		public:
			template<nox::SystemPhaseType _PhaseType, std::same_as<SystemPhaseImpl<_PhaseType>>... _Deps>
				requires(sizeof...(_Deps) <= k_max_dependency_length)
			inline constexpr explicit PhaseRegister(
				const SystemPhaseImpl<_PhaseType>& phase_,
				const _Deps&... deps
			)noexcept :
				phase_(phase_),
				dependencies_(MakeDependencies(
					phase_,
					std::make_index_sequence<k_max_dependency_length - sizeof...(_Deps)>{},
					deps...
				)),
				depended_(MakeDependencies(
					phase_,
					std::make_index_sequence<k_max_dependency_length>{}
				)),
				dependency_count_(sizeof...(_Deps)),
				depended_count_(0)
			{
			}

			template<nox::SystemPhaseType _PhaseType>
			inline constexpr explicit PhaseRegister(
				const SystemPhaseImpl<_PhaseType>& phase_,
				PhaseRelationInitializer dependencies,
				PhaseRelationInitializer depended = {}
			)noexcept :
				phase_(phase_),
				dependencies_(MakeDependencies(phase_, dependencies)),
				depended_(MakeDependencies(phase_, depended)),
				dependency_count_(MakeRelationCount(dependencies)),
				depended_count_(MakeRelationCount(depended))
			{
			}

		public:
			inline constexpr const SystemPhase& GetPhase()const noexcept { return phase_; }

			inline constexpr std::span<const std::reference_wrapper<const SystemPhase>> GetDependencies()const noexcept
			{
				return std::span(dependencies_.data(), dependency_count_);
			}

			inline constexpr std::span<const std::reference_wrapper<const SystemPhase>> GetDepended()const noexcept
			{
				return std::span(depended_.data(), depended_count_);
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

			static inline constexpr auto MakeDependencies(
				const SystemPhase& sentinel,
				PhaseRelationInitializer phases
			) noexcept
			{
				auto result = MakeDependencies(sentinel, std::make_index_sequence<k_max_dependency_length>{});
				size_t index = 0;
				for (const std::reference_wrapper<const SystemPhase>& phase : phases)
				{
					if (index >= k_max_dependency_length)
					{
						break;
					}
					result[index] = phase;
					++index;
				}
				return result;
			}

			static inline constexpr nox::uint8 MakeRelationCount(PhaseRelationInitializer phases)noexcept
			{
				return static_cast<nox::uint8>(std::min(phases.size(), k_max_dependency_length));
			}

		private:
			const SystemPhase& phase_;
			/// @brief 依存先のSystemPhase	これらのフェーズが全て完了してから当該フェーズが実行される
			const std::array<std::reference_wrapper<const SystemPhase>, k_max_dependency_length> dependencies_;
			/// @brief 当該フェーズに依存するSystemPhase	これらのフェーズは当該フェーズが完了してから実行される
			const std::array<std::reference_wrapper<const SystemPhase>, k_max_dependency_length> depended_;
			const nox::uint8 dependency_count_;
			const nox::uint8 depended_count_;

		};

	public:
		inline constexpr virtual std::span<const nox::SystemBase::PhaseRegister> GetPhaseRegisterList()const noexcept = 0;

	protected:
		inline SystemBase()noexcept
		{}

		virtual ~SystemBase()override {}

	};

	template<class T>
	consteval bool StaticAssertIsSystem()noexcept
	{
		return true;
	}
}