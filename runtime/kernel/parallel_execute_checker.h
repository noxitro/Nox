//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	parallel_execute_checker.h
///	@brief	並列チェッカー
#pragma once
#include	<atomic>
#include	<source_location>
#include	<string_view>
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
	/// @details 「Readerは何本でも同時に入れる / Writerは他のReader・Writerと排他」というRWロックの
	///          検査版。ロックは取らず、宣言(依存解析)が破れた瞬間にその場でアサートするだけ。
	///
	///          状態は uint64 1ワードに詰めてCASで回すため、ノードごとの入退場は数命令で済む。
	///          レイアウト:
	///            bit  0..23 : Reader数
	///            bit 24..31 : Writerの再入深度(0ならWriter不在)
	///            bit 32..63 : Writerを保持しているネイティブスレッドID
	///
	///          同一スレッドからの再入は許可する。Writerを保持しているスレッドは、同じチェッカーへ
	///          Read/Writeで再入してよい(自分自身との競合は競合ではない)。Reader同士の入れ子は
	///          そもそもReader数が増えるだけなので自然に通る。
	class RWParallelExecuteChecker
	{
	public:
		inline constexpr RWParallelExecuteChecker()noexcept :state_(0ull), name_() {}
		inline ~RWParallelExecuteChecker()noexcept {}
		inline constexpr RWParallelExecuteChecker(const RWParallelExecuteChecker&)noexcept = delete;
		inline constexpr RWParallelExecuteChecker(RWParallelExecuteChecker&&)noexcept = delete;
		inline constexpr RWParallelExecuteChecker& operator =(const RWParallelExecuteChecker&)noexcept = delete;

		/// @brief 検出時のメッセージに載せるラベルを設定する。
		/// @details 文字列はコピーしない。呼び出し側が寿命を保証すること(型名など静的な文字列を想定)。
		inline constexpr void SetName(std::string_view name)noexcept { name_ = name; }
		[[nodiscard]] inline constexpr std::string_view GetName()const noexcept { return name_; }

		/// @brief 読み取りとして入る。Writer(自スレッド以外)がいれば違反。
		void EnterRead(nox::util::detail::ParallelExecuteCheckOption option, const std::source_location& location);
		void ExitRead();

		/// @brief 書き込みとして入る。ReaderかWriter(自スレッド以外)がいれば違反。
		void EnterWrite(nox::util::detail::ParallelExecuteCheckOption option, const std::source_location& location);
		void ExitWrite();

		/// @brief 旧APIの互換。書き込み扱いで入る。
		inline void Enter(nox::util::detail::ParallelExecuteCheckOption option, const std::source_location& location) { EnterWrite(option, location); }
		inline void Exit() { ExitWrite(); }

	private:
		void ReportViolation(
			std::u8string_view access_kind,
			nox::uint64 observed_state,
			nox::util::detail::ParallelExecuteCheckOption option,
			const std::source_location& location)const;

	private:
		std::atomic<nox::uint64> state_;
		/// @brief 診断用のラベル。所有しない。
		std::string_view name_;
	};

#endif // !NOX_MASTER
}