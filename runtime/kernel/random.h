// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	random.h
/// @brief	random
#pragma once
#include	<random>
#include	"basic_type.h"

namespace nox::random
{
	constexpr nox::uint32 k_default_seed = 5489U;
	inline nox::uint32 GenSeed()
	{
		return std::random_device{}();
	}

	namespace detail
	{
		template<std::uniform_random_bit_generator _EngineType>
		class RandomImpl
		{
		private:
			using _ResultType = typename _EngineType::result_type;
		public:
			inline explicit RandomImpl(const _ResultType seed = static_cast<_ResultType>(nox::random::k_default_seed)) : engine_(seed) {}

			inline void Reseed(_ResultType seed)noexcept(noexcept(engine_.seed(seed)))
			{
				engine_.seed(seed);
			}

			inline void Discard(nox::uint64 z)noexcept(noexcept(engine_.discard(z)))
			{
				engine_.discard(z);
			}

			inline _ResultType Generate()noexcept(noexcept(engine_()))
			{
				return engine_();
			}

			inline static constexpr _ResultType Min()noexcept
			{
				return _EngineType::min();
			}

			inline static constexpr _ResultType Max()noexcept
			{
				return _EngineType::max();
			}

			template<std::integral T = nox::int32>
			inline T GenInteger()const
			{
				std::uniform_int_distribution<T> dist(Min(), Max());
				return dist(engine_);
			}

			template<std::floating_point T = nox::float_t>
			inline T GenReal()const
			{
				std::uniform_real_distribution<T> dist(static_cast<T>(0), static_cast<T>(1));
				return dist(engine_);
			}
		private:
			_EngineType engine_;
		};
	}

	using MT19937 = detail::RandomImpl<std::mt19937>;
	using Default = MT19937;

	namespace detail
	{
		template<typename _EngineType>
		_EngineType& GetEngine()noexcept;
	}

	template<std::integral T, class Engine = nox::random::Default>
	inline T GenInteger()
	{
		return nox::random::detail::GetEngine<Engine>().GenInteger<T>();
	}

	template<std::floating_point T, class Engine = nox::random::Default>
	inline T GenReal()
	{
		return nox::random::detail::GetEngine<Engine>().GenReal<T>();
	}
}