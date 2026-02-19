//	Copyright (c) 2025 NOX ENGINE All rights reserved.

///	@file	remote_host.cpp
///	@brief	remote_host
#include	"stdafx.h"
#include	"editor_ipc_server.h"

#if NOX_DEVELOP
#include	"editor_ipc_response.h"
#include	"editor_ipc_query.h"
#include	"net/dev_net_api.h"

namespace nox::dev::editor_ipc
{
	
}

nox::dev::editor_ipc::EditorIpcServer::EditorIpcServer():
	query_id_counter_(0),
	response_dict_{},
	main_client_{},
	writer_(*this),
	reader_(*this)
{
	this->Startup(InitializeContext{
		.max_connection = 1,
		.port = 86,
		});
}

nox::dev::editor_ipc::EditorIpcServer::~EditorIpcServer()
{

}

void	nox::dev::editor_ipc::EditorIpcServer::SendQuery(nox::dev::editor_ipc::Query& query, std::function<void(const nox::dev::editor_ipc::Response&)> callback)
{
	const nox::uint32 query_id = nox::os::atomic::Increment(query_id_counter_);

	NOX_LOCAL_SCOPE(nox::os::ScopedLock(mutex_writer_));
	query.Serialize(query_id, writer_);

	if (callback != nullptr)
	{
		response_dict_.emplace(query_id, callback);
	}

	writer_.Flush();
}

void	nox::dev::editor_ipc::EditorIpcServer::SendBuffer(std::span<const nox::uint8> buffer)
{
	if (main_client_.socket != nox::dev::net::k_raw_invalid_socket)
	{
		this->Send(main_client_.socket, static_cast<const void*>(buffer.data()), static_cast<nox::int32>(buffer.size()));
	}
}

void	nox::dev::editor_ipc::EditorIpcServer::Update()
{
	if (main_client_.socket != nox::dev::net::k_raw_invalid_socket)
	{
		UpdateReceive();
	}
}

void nox::dev::editor_ipc::EditorIpcServer::UpdateReceive()
{
	if (reader_.GetReceivedSize() <= 0)
	{
		return;
	}

	alignas(alignof(nox::dev::editor_ipc::EditorIpcEntity)) std::array<nox::uint8, 1024> entity_buffer{ 0 };
	alignas(alignof(nox::dev::editor_ipc::Response)) std::array<nox::uint8, 1024> receive_buffer{ 0 };
	std::array<nox::char8, 256> entity_name_buffer;

	NOX_LOCAL_SCOPE(nox::os::ScopedLock(mutex_writer_));

	while (reader_.GetReceivedSize() > 0)
	{
		//	entity名を読み取り
		
		const std::u8string_view entity_full_name = this->reader_.ReadString(entity_name_buffer);

		const nox::reflection::ClassInfo*const class_info = nox::reflection::FindClassInfo(entity_full_name);
		NOX_ASSERT(class_info != nullptr, u"不明なEntity:{0}", entity_full_name);

		nox::dev::editor_ipc::EditorIpcEntity*const entity = static_cast<nox::dev::editor_ipc::EditorIpcEntity*>(class_info->GetType().CreateObject(entity_buffer));
		NOX_ASSERT(entity != nullptr, u"EditorIpcEntityの生成に失敗:{0}", entity_full_name);

		//	query
		if (class_info->IsBaseOf<nox::dev::editor_ipc::Query>())
		{
			nox::dev::editor_ipc::Query& query = static_cast<nox::dev::editor_ipc::Query&>(*entity);
			query.Deserialize(reader_);

			//	レスポンス生成
			{
				nox::PlacementObject<nox::dev::editor_ipc::Response> response = query.Execute(receive_buffer);
				if (response != nullptr)
				{
					response->Serialize(query.GetId(), writer_);
					writer_.Flush();
				}
			}
		}
		else if (class_info->IsBaseOf<nox::dev::editor_ipc::Response>())
		{
			nox::dev::editor_ipc::Response& response = static_cast<nox::dev::editor_ipc::Response&>(*entity);

			response.Deserialize(reader_);

			const nox::uint32 query_id = response.GetId();
			//	callback実行
			const auto it = response_dict_.find(query_id);
			if (it != response_dict_.end())
			{
				std::invoke(it->second, response);
				response_dict_.erase(it);
			}
		}
		else
		{
			NOX_ASSERT(false, u"不明な型:{0}", entity_full_name);
		}
	}
}

void nox::dev::editor_ipc::EditorIpcServer::OnReceive()
{
	//	受信バッファ 未初期化でOK
	std::array<nox::uint8, 1024> receive_buffer;
	//	送受信サイズ
	nox::int32 received_size = 0;	

	//	受信できるだけする
	while (true)
	{
		const nox::int32 receive_size = nox::dev::net::Receive(main_client_.socket, static_cast<char*>(static_cast<void*>(receive_buffer.data())), static_cast<nox::int32>(receive_buffer.size()));
		//	切断
		if (receive_size == 0)
		{
			break;
		}
		else
		{

		}
	}

	//	読み取りストリームに受信バッファを追加
	reader_.AddReceiveBuffer(std::span(receive_buffer.data(), received_size));
}

void	nox::dev::editor_ipc::EditorIpcServer::OnConnected(const nox::dev::net::ConnectionContext& context)
{
	main_client_ = context;
}

void	nox::dev::editor_ipc::EditorIpcServer::OnDisconnected(const nox::dev::net::ConnectionContext& context)
{
	main_client_.socket = nox::dev::net::k_raw_invalid_socket;
}
#endif // NOX_DEVELOP