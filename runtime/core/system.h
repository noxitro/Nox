// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	system.h
/// @brief	system
#pragma once

namespace nox
{
	struct ISystem
	{
		enum class PhaseType : std::uint8_t
		{
			Init,
			Start,
			Update,
			Terminate,
			_Max
		};

		struct SystemPhase
		{

		};
	};

	class SystemBase : public ISystem
	{

	};

	template<class T>
	consteval bool StaticAssertIsSystem()noexcept
	{
		return true;
	}
}