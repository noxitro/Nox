//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	parallel_execute_checker.h
///	@brief	並列チェッカー
#pragma once
#include	<source_location>
#include	"basic_definition.h"
#include	"basic_type.h"

namespace nox::util
{
#if !NOX_MASTER
	class ParallelExecuteChecker;

	/// @brief 並列実行チェック
	class ParallelExecuteCheckScope
	{
	public:
		enum class Option : nox::uint8
		{
			SourceLocation,
			Callstack,
		};

	public:
		ParallelExecuteCheckScope(nox::util::ParallelExecuteChecker& checker, Option option = Option::SourceLocation, const std::source_location location = std::source_location::current());
		~ParallelExecuteCheckScope();

		inline constexpr ParallelExecuteCheckScope(const ParallelExecuteCheckScope&)noexcept = delete;
		inline constexpr ParallelExecuteCheckScope(ParallelExecuteCheckScope&&)noexcept = delete;

		inline constexpr ParallelExecuteCheckScope& operator =(const ParallelExecuteCheckScope&)noexcept = delete;
	private:
		nox::util::ParallelExecuteChecker& checker_;
	};

	/// @brief 並列実行チェッカー
	class ParallelExecuteChecker
	{
	public:
		inline constexpr ParallelExecuteChecker()noexcept :ref_counter_(0) {}
		inline ~ParallelExecuteChecker()noexcept {}

		inline	constexpr ParallelExecuteChecker(const ParallelExecuteChecker&)noexcept = delete;
		inline	constexpr ParallelExecuteChecker(ParallelExecuteChecker&&)noexcept = delete;

		inline constexpr ParallelExecuteChecker& operator =(const ParallelExecuteChecker&)noexcept = delete;

		void Enter(nox::util::ParallelExecuteCheckScope::Option option, const std::source_location& location);
		void Exit();
	private:
		nox::int8 ref_counter_;
	};

#endif // !NOX_MASTER
}