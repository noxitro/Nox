// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	random.cpp
/// @brief	random
#include "pch.h"
#include "random.h"



namespace nox::random::detail
{
	namespace
	{
		template<class _EngineType>
		thread_local _EngineType k_engine{ nox::random::GenSeed()};
	}
}

template<>
nox::random::MT19937& nox::random::detail::GetEngine<nox::random::MT19937>()noexcept
{
	return k_engine<nox::random::MT19937>;
}