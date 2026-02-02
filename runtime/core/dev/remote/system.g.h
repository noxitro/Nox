#pragma once
//	do not edit
//	written from RuntimeRemoteCodeGenerator

#if	NOX_DEVELOP
#include	"../editor_ipc_query.h"
#include	"../editor_ipc_response.h"

namespace nox::dev::editor_ipc
{
	class ResourceConvertQuery : public nox::dev::editor_ipc::Query
	{
		NOX_DECLARE_OBJECT(nox::dev::editor_ipc::ResourceConvertQuery, nox::dev::editor_ipc::Query);
	public:
		ResourceConvertQuery(){}
		void OnSerialize(nox::dev::editor_ipc::SocketStreamWriter& writer)override;
		void OnDeserialize(nox::dev::editor_ipc::SocketStreamReader& reader)override;
		nox::PlacementObject<nox::dev::editor_ipc::Response> Execute(std::span<nox::uint8> storage)const override;

		inline std::u8string_view GetNativePath()
		{
			return nativePath_;
		}
		inline void SetNativePath(std::u8string_view value)
		{
			nativePath_ = value;
		}

	private:
		nox::U8FixedString<256> nativePath_;
	};
}
#endif	//	NOX_DEVELOP
