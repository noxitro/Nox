// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	service.h
/// @brief	service
#pragma once
#include "object.h"

namespace nox
{
	class Service : public ::nox::Object
	{
		friend class World;
		NOX_DECLARE_OBJECT(Service, nox::Object);
	protected:
		enum class PhaseType : nox::uint8
		{
			Init,
			Start,
			Update,
			Terminate
		};

		struct Phase
		{
			void(nox::Service::* func)(nox::World&);
			std::u8string_view name;
			nox::Service::PhaseType type;
		};

		template<nox::Service::PhaseType _PhaseType>
		struct PhaseImpl : public Phase
		{
			template<class T>
			inline constexpr explicit PhaseImpl(
				void(T::* func)(nox::World&),
				std::u8string_view name
			)noexcept :
				nox::Service::Phase{
					static_cast<void(nox::Service::*)(nox::World&)>(func),
					name,
					_PhaseType
			}
			{
			}
		};

		using PhaseInit = PhaseImpl<nox::Service::PhaseType::Init>;
		using PhaseStart = PhaseImpl<nox::Service::PhaseType::Start>;
		using PhaseUpdate = PhaseImpl<nox::Service::PhaseType::Update>;
		using PhaseTerminate = PhaseImpl<nox::Service::PhaseType::Terminate>;

		struct PhaseRegister
		{
		private:
			static constexpr size_t k_max_dependency_length = 16;
			using PhaseRelationInitializer = std::initializer_list<std::reference_wrapper<const Phase>>;
		public:
			template<nox::Service::PhaseType _PhaseType, std::same_as<PhaseImpl<_PhaseType>>... _Deps>
				requires(sizeof...(_Deps) <= k_max_dependency_length)
			inline constexpr explicit PhaseRegister(
				const PhaseImpl<_PhaseType>& phase_,
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

			template<nox::Service::PhaseType _PhaseType>
			inline constexpr explicit PhaseRegister(
				const PhaseImpl<_PhaseType>& phase_,
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
			inline constexpr const Phase& GetPhase()const noexcept { return phase_; }

			inline constexpr std::span<const std::reference_wrapper<const Phase>> GetDependencies()const noexcept
			{
				return std::span(dependencies_.data(), dependency_count_);
			}

			inline constexpr std::span<const std::reference_wrapper<const Phase>> GetDepended()const noexcept
			{
				return std::span(depended_.data(), depended_count_);
			}

		private:
			template<class... _Deps, size_t... _PadIs>
			static inline constexpr auto MakeDependencies(
				const Phase& sentinel,
				std::index_sequence<_PadIs...>,
				const _Deps&... deps
			) noexcept
			{
				return std::array<std::reference_wrapper<const Phase>, k_max_dependency_length>{
					std::cref<const Phase>(deps)...,
						((void)_PadIs, std::cref<const Phase>(sentinel))...
				};
			}

			static inline constexpr auto MakeDependencies(
				const Phase& sentinel,
				PhaseRelationInitializer phases
			) noexcept
			{
				auto result = MakeDependencies(sentinel, std::make_index_sequence<k_max_dependency_length>{});
				size_t index = 0;
				for (const std::reference_wrapper<const Phase>& phase : phases)
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
			const Phase& phase_;
			/// @brief 依存先のPhase	これらのフェーズが全て完了してから当該フェーズが実行される
			const std::array<std::reference_wrapper<const Phase>, k_max_dependency_length> dependencies_;
			/// @brief 当該フェーズに依存するPhase	これらのフェーズは当該フェーズが完了してから実行される
			const std::array<std::reference_wrapper<const Phase>, k_max_dependency_length> depended_;
			const nox::uint8 dependency_count_;
			const nox::uint8 depended_count_;

		};

	public:
		inline constexpr virtual std::span<const nox::Service::PhaseRegister> GetPhaseRegisterList()const noexcept = 0;

	protected:
		inline Service()noexcept
		{
		}

		virtual ~Service()override {}
	};
}