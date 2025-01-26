///	@file	log_trace.h
///	@brief	log_trace
#pragma once

#include	"advanced_type.h"
#include	"convert_string.h"
#include	"nox_string_view.h"

/// @brief		ログID構造体の定義名前空間
namespace nox::log_tag
{
	/// @brief ログID
	/// @details ログIDを定義する構造体を継承することで、ログIDを定義できます
	struct LogId
	{
	};

	/// @brief kernel用
	struct Kernel : LogId
	{
		inline constexpr std::u32string_view operator()() const noexcept { return U"Kernel"; }
	};
}

namespace nox::dev
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
		void	TraceDirect(LogCategory log_category, const StringView category, const StringView message, bool isNewLine, const std::source_location& source_location);
	}

	template<std::derived_from<log_tag::LogId> LogId>
	inline	void	LogTrace(LogCategory log_category, const StringView message, const std::source_location source_location = std::source_location::current())
	{
		nox::dev::detail::TraceDirect(log_category, LogId()(), message, true, source_location);
	}

	/// @brief 削除予定
	/// @param log_category 
	/// @param message 
	/// @param source_location 
	inline	void	LogTrace(LogCategory log_category, const StringView message, const std::source_location source_location = std::source_location::current())
	{
		nox::dev::LogTrace<log_tag::Kernel>(log_category, message, source_location);
	}
}

#if NOX_DEBUG
#define	NOX_INFO_LINE_OLD(...) ::nox::dev::LogTrace(::nox::dev::LogCategory::Info, __VA_ARGS__)
#define NOX_WARNING_LINE_OLD(...) ::nox::dev::LogTrace(::nox::dev::LogCategory::Warning, __VA_ARGS__)
#define NOX_ERROR_LINE_OLD(...) ::nox::dev::LogTrace(::nox::dev::LogCategory::Error, __VA_ARGS__)

#define NOX_INFO_LINE(LodId, message) ::nox::dev::LogTrace<LodId>(::nox::dev::LogCategory::Info, message)
#define NOX_WARNING_LINE(LodId, message) ::nox::dev::LogTrace<LodId>(::nox::dev::LogCategory::Warning, message)
#define NOX_ERROR_LINE(LodId, message) ::nox::dev::LogTrace<LodId>(::nox::dev::LogCategory::Error, message)
#else
#define	NOX_INFO_LINE_OLD(...)
#define NOX_WARNING_LINE_OLD(...)
#define NOX_ERROR_LINE_OLD(...)

#define NOX_INFO_LINE(LodId, message)
#define NOX_WARNING_LINE(LodId, message)
#define NOX_ERROR_LINE(LodId, message)

#endif // NOX_DEBUG
