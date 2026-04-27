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

	private:
		LogLevel level_ {};
	};

}
#endif	//	NOX_DEVELOP
