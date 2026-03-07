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
	class RWParallelExecuteChecker;

	namespace detail
	{
		enum class ParallelExecuteCheckOption : nox::uint8
		{
			SourceLocation,
			StackTrace,
		};
	}

	/// @brief 並列実行チェック
	class ParallelExecuteCheckScope
	{
	public:
		ParallelExecuteCheckScope(nox::util::ParallelExecuteChecker& checker, nox::util::detail::ParallelExecuteCheckOption option = nox::util::detail::ParallelExecuteCheckOption::SourceLocation, const std::source_location location = std::source_location::current());
		~ParallelExecuteCheckScope();

		inline constexpr ParallelExecuteCheckScope(const ParallelExecuteCheckScope&)noexcept = delete;
		inline constexpr ParallelExecuteCheckScope(ParallelExecuteCheckScope&&)noexcept = delete;

		inline constexpr ParallelExecuteCheckScope& operator =(const ParallelExecuteCheckScope&)noexcept = delete;
	private:
		nox::util::ParallelExecuteChecker& checker_;
	};

	class ReadParallelExecuteCheckScope
	{
	public:
		ReadParallelExecuteCheckScope(nox::util::RWParallelExecuteChecker& checker, nox::util::detail::ParallelExecuteCheckOption option = nox::util::detail::ParallelExecuteCheckOption::SourceLocation, const std::source_location location = std::source_location::current());
		~ReadParallelExecuteCheckScope();
		inline constexpr ReadParallelExecuteCheckScope(const ReadParallelExecuteCheckScope&)noexcept = delete;
		inline constexpr ReadParallelExecuteCheckScope(ReadParallelExecuteCheckScope&&)noexcept = delete;
		inline constexpr ReadParallelExecuteCheckScope& operator =(const ReadParallelExecuteCheckScope&)noexcept = delete;
	private:
		nox::util::RWParallelExecuteChecker& checker_;
	};

	class WriteParallelExecuteCheckScope
	{
	public:
		WriteParallelExecuteCheckScope(nox::util::RWParallelExecuteChecker& checker, nox::util::detail::ParallelExecuteCheckOption option = nox::util::detail::ParallelExecuteCheckOption::SourceLocation, const std::source_location location = std::source_location::current());
		~WriteParallelExecuteCheckScope();
		inline constexpr WriteParallelExecuteCheckScope(const WriteParallelExecuteCheckScope&)noexcept = delete;
		inline constexpr WriteParallelExecuteCheckScope(WriteParallelExecuteCheckScope&&)noexcept = delete;
		inline constexpr WriteParallelExecuteCheckScope& operator =(const WriteParallelExecuteCheckScope&)noexcept = delete;
	private:
		nox::util::RWParallelExecuteChecker& checker_;
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

		void Enter(nox::util::detail::ParallelExecuteCheckOption option, const std::source_location& location);
		void Exit();
	private:
		nox::int8 ref_counter_;
	};

	/// @brief Read/Write 並列実行チェッカー
	class RWParallelExecuteChecker
	{
	public:
		inline constexpr RWParallelExecuteChecker()noexcept :ref_counter_(0) {}
		inline ~RWParallelExecuteChecker()noexcept {}
		inline constexpr RWParallelExecuteChecker(const RWParallelExecuteChecker&)noexcept = delete;
		inline constexpr RWParallelExecuteChecker(RWParallelExecuteChecker&&)noexcept = delete;
		inline constexpr RWParallelExecuteChecker& operator =(const RWParallelExecuteChecker&)noexcept = delete;
		void Enter(nox::util::detail::ParallelExecuteCheckOption option, const std::source_location& location);
		void Exit();

	private:
		nox::int8 ref_counter_;
	};

#endif // !NOX_MASTER
}