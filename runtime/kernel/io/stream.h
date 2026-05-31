//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	stream.h
///	@brief	.NET 風 Stream 抽象と実装（NOX型/アロケータ・例外非依存・NOX_ASSERT使用）
#pragma once
#include	"../advanced_type.h"
#include	"../type_traits/concepts.h"

namespace nox::io
{
	enum class SeekOrigin : nox::uint8
	{
		Begin,
		Current,
		End
	};

	class Stream
	{
	public:
		inline constexpr Stream()noexcept {}
		inline constexpr virtual ~Stream()noexcept {}

		virtual void Flush() {}
	};
}
