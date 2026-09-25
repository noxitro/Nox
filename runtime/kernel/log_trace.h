// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

///	@file	log_trace.h
///	@brief	ログ出力
#pragma once

#include	"advanced_type.h"
#include	"convert_string.h"
#include	"string_format.h"

/// @brief		ログID構造体の定義名前空間
namespace nox::log_id
{
	/// @brief ログID
	/// @details ログIDを定義する構造体を継承することで、ログIDを定義できます
	struct LogId
	{
	protected:
		constexpr LogId()noexcept = default;
		constexpr ~LogId()noexcept = default;
	};

	/// @brief 無効なログID
	struct Invalid final : LogId
	{
		inline constexpr std::u8string_view operator()() const noexcept { return u8"Invalid"; }
	};
}

namespace nox::debug
{
	/// @brief ログタイプ
	enum class LogLevel : uint8
	{
		Info,
		Warning,
		Error,
		_Max
	};

	struct LogHandlerArgs
	{
		nox::uint32 column;
		nox::debug::LogLevel level;
		std::u8string_view message;
		std::u8string_view callstack;
		std::u8string_view channel;
	};

	void AttachLogHandler(std::function<void(const nox::debug::LogHandlerArgs&)>);
	void DetachLogHandler();

	namespace detail
	{
		void	TraceDirect(LogLevel log_category, const std::u8string_view category, const std::u8string_view message, bool isNewLine, const std::source_location& source_location);
		void	TraceDirect(LogLevel log_category, const std::u8string_view category, const std::u16string_view message, bool isNewLine, const std::source_location& source_location);

		//template<class... Args>
		//void	TraceDirectArgs(LogCategory log_category, const std::u16string_view category, bool isNewLine, const std::source_location& source_location, const std::u32string_view message, Args&&...args)
		//{
		//	//	動的メモリ確保を行わないように確保済みのバッファを使用
		//	std::array<nox::char32, 5096> buffer = { 0 };
		//	nox::util::Format(buffer, message.data(), std::forward<Args>(args)...);

		//	nox::debug::detail::TraceDirect(log_category, category, buffer.data(), isNewLine, source_location);
		//}

		template<class... Args>
		void	TraceDirectArgs(LogLevel log_category, const std::u8string_view category, bool isNewLine, const std::source_location& source_location, const std::u8string_view message, Args&&...args)
		{
			//	動的メモリ確保を行わないように確保済みのバッファを使用
			std::array<nox::char8, 5096> buffer = { 0 };
			nox::util::Format(buffer, message.data(), std::forward<Args>(args)...);

			nox::debug::detail::TraceDirect(log_category, category, buffer.data(), isNewLine, source_location);
		}

		template<class... Args>
		void	TraceDirectArgs(LogLevel log_category, const std::u8string_view category, bool isNewLine, const std::source_location& source_location, const std::u16string_view message, Args&&...args)
		{
			//	動的メモリ確保を行わないように確保済みのバッファを使用
			std::array<nox::char16, 5096> buffer = { 0 };
			nox::util::Format(buffer, message.data(), std::forward<Args>(args)...);

			nox::debug::detail::TraceDirect(log_category, category, buffer.data(), isNewLine, source_location);
		}

	}

	//template<std::derived_from<log_id::LogId> LogId> 
	//	requires(std::is_same_v<std::u16string_view, decltype(LogId()())>)
	//inline	void	LogTrace(LogCategory log_category, const std::u32string_view message, const std::source_location source_location = std::source_location::current())
	//{
	//	nox::debug::detail::TraceDirect(log_category, LogId()(), message, true, source_location);
	//}

	template<std::derived_from<log_id::LogId> LogId, class... Args>
		requires(std::is_polymorphic_v<LogId> == false && std::is_same_v<std::u8string_view, decltype(LogId()())>)
	inline	void	LogTraceArgs(LogLevel log_category, const std::source_location& source_location, const std::u8string_view message, Args&&... args)
	{
		constexpr std::u8string_view log_tag = LogId()();
		nox::debug::detail::TraceDirectArgs(log_category, log_tag, true, source_location, message, std::forward<Args>(args)...);
	}

	///// @brief		ログ出力
	///// @details	フォーマット処理を動的メモリ確保を行わないように確保済みのバッファを使用する
	///// @tparam ...Args 
	///// @tparam LogId 
	///// @param log_category 
	///// @param source_location 
	///// @param message 
	///// @param ...args 
	//template<std::derived_from<log_id::LogId> LogId, class... Args>
	//	requires(std::is_polymorphic_v<LogId> == false && std::is_same_v<std::u16string_view, decltype(LogId()())>)
	//inline	void	LogTraceArgs(LogCategory log_category, const std::source_location& source_location, const std::u32string_view message, Args&&... args)
	//{
	//	constexpr std::u16string_view log_tag = LogId()();
	//	nox::debug::detail::TraceDirectArgs(log_category, log_tag, true, source_location, message, std::forward<Args>(args)...);
	//}
	template<std::derived_from<log_id::LogId> LogId, class... Args>
		requires(std::is_polymorphic_v<LogId> == false && std::is_same_v<std::u8string_view, decltype(LogId()())>)
	inline	void	LogTraceArgs(LogLevel log_category, const std::source_location& source_location, const std::u16string_view message, Args&&... args)
	{
		constexpr std::u8string_view log_tag = LogId()();
		nox::debug::detail::TraceDirectArgs(log_category, log_tag, true, source_location, message, std::forward<Args>(args)...);
	}
}

#if !NOX_MASTER
/// @brief		ログ出力 レベル：Info
/// @details	フォーマット処理を動的メモリ確保を行わないように確保済みのバッファを使用する
#define NOX_INFO_LINE(LodId, ...) ::nox::debug::LogTraceArgs<LodId>(::nox::debug::LogLevel::Info, ::std::source_location::current(), __VA_ARGS__)
#define NOX_WARNING_LINE(LodId, ...) ::nox::debug::LogTraceArgs<LodId>(::nox::debug::LogLevel::Warning, ::std::source_location::current(), __VA_ARGS__)
#define NOX_ERROR_LINE(LodId, ...) ::nox::debug::LogTraceArgs<LodId>(::nox::debug::LogLevel::Error, ::std::source_location::current(), __VA_ARGS__)
#else
namespace nox::debug
{
	/// @brief		ログ出力の無効化時に引数だけを型検査するためのプローブ
	/// @details	NOX_*_LINE は Master ビルドではログを出さないが、引数を単純に
	///				捨てると 2 つの問題が出る。
	///				  1. ログにしか使っていないローカル変数・引数が未参照になり、
	///				     C4189 / C4100 が湧く。呼び出し側へ [[maybe_unused]] を
	///				     撒く羽目になり、ログ行を書くたびに増え続ける。
	///				  2. Master では引数に何を書いても通ってしまうため、
	///				     開発ビルド側 LogTraceArgs との不整合 (引数の個数・型・LogId の
	///				     取り違え) を Master ビルドで検出できない。
	///				     実際、無効化側が (LodId, message) の 2 引数固定のまま
	///				     壊れていたことに長く気付けなかった。
	///
	///				そこで NOX_*_LINE を sizeof の未評価オペランドへ展開し、
	///				この関数テンプレートの多重定義解決だけを行わせる。
	///				  - 未評価オペランドなので引数は「評価されない」。
	///				    副作用のある式が実行されることはなく、コードも生成されない。
	///				  - odr-use されないため定義は不要。宣言だけを置いてあるので、
	///				    コード生成ゼロは最適化任せではなく言語仕様で保証される。
	///				  - 変数は「参照された」と扱われるので C4189 / C4100 は出ない。
	///
	///				シグネチャは LogTraceArgs の鏡写しにしてある (テンプレート引数
	///				LogId + requires 制約 + message + Args&&...)。こうしておくと
	///				「開発ビルドで通る呼び出しは Master でも通り、逆も成立する」が
	///				保たれ、引数個数の不一致が構造的に再発しなくなる。
	///
	///				sizeof は完全型を要求するので戻り値は void ではなく int。
	template<std::derived_from<log_id::LogId> LogId, class... Args>
		requires(std::is_polymorphic_v<LogId> == false && std::is_same_v<std::u8string_view, decltype(LogId()())>)
	int		LogTraceProbe(const std::u8string_view message, Args&&... args);

	template<std::derived_from<log_id::LogId> LogId, class... Args>
		requires(std::is_polymorphic_v<LogId> == false && std::is_same_v<std::u8string_view, decltype(LogId()())>)
	int		LogTraceProbe(const std::u16string_view message, Args&&... args);
}

#define NOX_INFO_LINE(LodId, ...) ((void)sizeof(::nox::debug::LogTraceProbe<LodId>(__VA_ARGS__)))
#define NOX_WARNING_LINE(LodId, ...) ((void)sizeof(::nox::debug::LogTraceProbe<LodId>(__VA_ARGS__)))
#define NOX_ERROR_LINE(LodId, ...) ((void)sizeof(::nox::debug::LogTraceProbe<LodId>(__VA_ARGS__)))

#endif // !NOX_MASTER
