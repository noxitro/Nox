///	@file	log_trace.h
///	@brief	ログ出力
#pragma once

#include	"advanced_type.h"
#include	"convert_string.h"
#include	"nox_string_view.h"
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
		inline constexpr std::u16string_view operator()() const noexcept { return u"Invalid"; }
	};
}

namespace nox::debug
{
	/// @brief ログタイプ
	enum class LogCategory : uint8
	{
		Info,
		Warning,
		Error,
		_Max
	};

	namespace detail
	{
		void	TraceDirect(LogCategory log_category, const std::u16string_view category, const std::u32string_view message, bool isNewLine, const std::source_location& source_location);
		void	TraceDirect(LogCategory log_category, const std::u16string_view category, const std::u16string_view message, bool isNewLine, const std::source_location& source_location);

		//template<class... Args>
		//void	TraceDirectArgs(LogCategory log_category, const std::u16string_view category, bool isNewLine, const std::source_location& source_location, const std::u32string_view message, Args&&...args)
		//{
		//	//	動的メモリ確保を行わないように確保済みのバッファを使用
		//	std::array<nox::char32, 5096> buffer = { 0 };
		//	nox::util::Format(buffer, message.data(), std::forward<Args>(args)...);

		//	nox::debug::detail::TraceDirect(log_category, category, buffer.data(), isNewLine, source_location);
		//}

		template<class... Args>
		void	TraceDirectArgs(LogCategory log_category, const std::u16string_view category, bool isNewLine, const std::source_location& source_location, const std::u16string_view message, Args&&...args)
		{
			//	動的メモリ確保を行わないように確保済みのバッファを使用
			std::array<nox::char16, 5096> buffer = { 0 };
			nox::util::Format(buffer, message.data(), std::forward<Args>(args)...);

			nox::debug::detail::TraceDirect(log_category, category, buffer.data(), isNewLine, source_location);
		}

	}

	template<std::derived_from<log_id::LogId> LogId> 
		requires(std::is_same_v<std::u16string_view, decltype(LogId()())>)
	inline	void	LogTrace(LogCategory log_category, const std::u32string_view message, const std::source_location source_location = std::source_location::current())
	{
		nox::debug::detail::TraceDirect(log_category, LogId()(), message, true, source_location);
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
		requires(std::is_polymorphic_v<LogId> == false && std::is_same_v<std::u16string_view, decltype(LogId()())>)
	inline	void	LogTraceArgs(LogCategory log_category, const std::source_location& source_location, const std::u16string_view message, Args&&... args)
	{
		constexpr std::u16string_view log_tag = LogId()();
		nox::debug::detail::TraceDirectArgs(log_category, log_tag, true, source_location, message, std::forward<Args>(args)...);
	}

	/// @brief 削除予定
	/// @param log_category 
	/// @param message 
	/// @param source_location 
	inline	void	LogTrace(LogCategory log_category, const std::u32string_view message, const std::source_location source_location = std::source_location::current())
	{
		nox::debug::LogTrace<log_id::Invalid>(log_category, message, source_location);
	}
}

#if NOX_DEBUG
#define	NOX_INFO_LINE_OLD(...) ::nox::debug::LogTrace(::nox::debug::LogCategory::Info, __VA_ARGS__)
#define NOX_WARNING_LINE_OLD(...) ::nox::debug::LogTrace(::nox::debug::LogCategory::Warning, __VA_ARGS__)
#define NOX_ERROR_LINE_OLD(...) ::nox::debug::LogTrace(::nox::debug::LogCategory::Error, __VA_ARGS__)

/// @brief		ログ出力 レベル：Info
/// @details	フォーマット処理を動的メモリ確保を行わないように確保済みのバッファを使用する
#define NOX_INFO_LINE(LodId, ...) ::nox::debug::LogTraceArgs<LodId>(::nox::debug::LogCategory::Info, ::std::source_location::current(), __VA_ARGS__)
#define NOX_WARNING_LINE(LodId, ...) ::nox::debug::LogTraceArgs<LodId>(::nox::debug::LogCategory::Warning, ::std::source_location::current(), __VA_ARGS__)
#define NOX_ERROR_LINE(LodId, ...) ::nox::debug::LogTraceArgs<LodId>(::nox::debug::LogCategory::Error, ::std::source_location::current(), __VA_ARGS__)
#else
#define	NOX_INFO_LINE_OLD(...)
#define NOX_WARNING_LINE_OLD(...)
#define NOX_ERROR_LINE_OLD(...)

#define NOX_INFO_LINE(LodId, message)
#define NOX_WARNING_LINE(LodId, message)
#define NOX_ERROR_LINE(LodId, message)

#endif // NOX_DEBUG
