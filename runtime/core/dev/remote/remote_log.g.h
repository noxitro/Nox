#pragma once
//	do not edit
//	written from RuntimeRemoteCodeGenerator

#if	NOX_DEVELOP
#include	"../editor_remote_query.h"
#include	"../editor_remote_response.h"

namespace nox::dev::editor_remote
{
	/// @brief ログ送信
	class SendLog final : public nox::dev::editor_remote::Query
	{
		NOX_DECLARE_OBJECT(nox::dev::editor_remote::SendLog, nox::dev::editor_remote::Query);
	public:
		enum class LogLevel : nox::uint8
		{
			Info = 0,
			Warn = 1,
			Error = 2,
			Fatal = 3
		};
	public:
		SendLog(){}
		void OnSerialize(nox::dev::editor_remote::SocketStreamWriter& writer)override;
		void OnDeserialize(nox::dev::editor_remote::SocketStreamReader& reader)override;
		nox::PlacementObject<nox::dev::editor_remote::Response> Execute(nox::Application&, std::span<nox::uint8> storage)const override;

		inline LogLevel GetLevel()const noexcept
		{
			return level_;
		}

		inline void SetLevel(LogLevel value)
		{
			level_ = value;
		}

		inline std::u8string_view GetMsg()const noexcept
		{
			return msg_;
		}

		inline void SetMsg(std::u8string_view value)
		{
			msg_ = value;
		}

		inline std::u8string_view GetCallStack()const noexcept
		{
			return call_stack_;
		}

		inline void SetCallStack(std::u8string_view value)
		{
			call_stack_ = value;
		}

		inline std::u8string_view GetChannel()const noexcept
		{
			return channel_;
		}

		inline void SetChannel(std::u8string_view value)
		{
			channel_ = value;
		}

	private:
		LogLevel level_ {};
		std::u8string_view msg_ {};
		std::u8string_view call_stack_ {};
		std::u8string_view channel_ {};
	};

}
#endif	//	NOX_DEVELOP
