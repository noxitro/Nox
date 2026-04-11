//	Copyright (c) 2025 NOX ENGINE All rights reserved.

///	@file	remote_host.cpp
///	@brief	remote_host
#include	"pch.h"
#include	"editor_remote_server.h"

#if NOX_DEVELOP
#include	"editor_remote_response.h"
#include	"editor_remote_query.h"
#include	"net/dev_net_api.h"
#include	"net/dev_net_log_id.h"
#include	"../application.h"

namespace nox::dev::editor_remote
{
	
}

nox::dev::editor_remote::EditorRemoteServer::EditorRemoteServer():
	query_id_counter_(0),
	instance_id_counter_(0),
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

nox::dev::editor_remote::EditorRemoteServer::~EditorRemoteServer()
{

}

void	nox::dev::editor_remote::EditorRemoteServer::SendQuery(nox::dev::editor_remote::Query& query, std::function<void(const nox::dev::editor_remote::Response&)> callback)
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

void	nox::dev::editor_remote::EditorRemoteServer::SendBuffer(std::span<const nox::uint8> buffer)
{
	if (main_client_.socket != nox::dev::net::k_raw_invalid_socket)
	{
		this->Send(main_client_.socket, static_cast<const void*>(buffer.data()), static_cast<nox::int32>(buffer.size()));
	}
}

void	nox::dev::editor_remote::EditorRemoteServer::Update()
{
	if (main_client_.socket != nox::dev::net::k_raw_invalid_socket)
	{
		UpdateReceive();
	}
}

void nox::dev::editor_remote::EditorRemoteServer::UpdateReceive()
{
	if (reader_.GetReceivedSize() <= 0)
	{
		return;
	}

	alignas(alignof(std::max_align_t)) std::array<nox::uint8, 1024> entity_buffer{ 0 };
	alignas(alignof(std::max_align_t)) std::array<nox::uint8, 1024> receive_buffer{ 0 };
	std::array<nox::char8, 256> entity_name_buffer;

	NOX_LOCAL_SCOPE(nox::os::ScopedLock(mutex_writer_));

	//	1パケット分が読み取れる限りループ
	while (reader_.CanReadBody())
	{
		//	ヘッダ部分を空読み
		reader_.SkipHeader();

		//	entity名を読み取り
		this->reader_.Read(entity_name_buffer);
		const std::u8string_view entity_full_name(entity_name_buffer.data());

		const nox::reflection::ClassInfo*const class_info = nox::reflection::FindClassInfo(entity_full_name);
		NOX_ASSERT(class_info != nullptr, u"不明なEntity:{0}", entity_full_name);

		nox::dev::editor_remote::EditorRemoteEntity*const entity = static_cast<nox::dev::editor_remote::EditorRemoteEntity*>(class_info->GetType().CreateObject(entity_buffer));
		NOX_ASSERT(entity != nullptr, u"EditorIpcEntityの生成に失敗:{0}", entity_full_name);

		NOX_LOCAL_SCOPE(nox::util::ScopeExit([&entity]() {
			std::destroy_at(entity);
			}));

		//	query
		if (class_info->IsSubclassOf<nox::dev::editor_remote::Query>())
		{
			nox::dev::editor_remote::Query& query = static_cast<nox::dev::editor_remote::Query&>(*entity);
			query.Deserialize(reader_);

			//	レスポンス生成
			{
				nox::PlacementObject<nox::dev::editor_remote::Response> response = query.Execute(receive_buffer);
				if (response != nullptr)
				{
					response->Serialize(query.GetId(), writer_);
					writer_.Flush();
				}
			}
		}
		else if (class_info->IsSubclassOf<nox::dev::editor_remote::Response>())
		{
			nox::dev::editor_remote::Response& response = static_cast<nox::dev::editor_remote::Response&>(*entity);

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

void nox::dev::editor_remote::EditorRemoteServer::OnReceive()
{
	//	受信バッファ 未初期化でOK
	std::array<nox::uint8, 2048> receive_buffer;
	//	送受信サイズ
	nox::int32 received_size = 0;	

	//	受信できるだけする
	while (nox::Application::Instance().IsKill() == false)
	{
		if (nox::Application::Instance().IsKill())
		{
			return;
		}

		const nox::int32 receive_size = nox::dev::net::Receive(main_client_.socket, static_cast<char*>(static_cast<void*>(receive_buffer.data())), static_cast<nox::int32>(receive_buffer.size()));
		if (receive_size > 0)
		{
			received_size += receive_size;
		}
		else if (receive_size == 0)
		{
			//	接続が切断された
			break;
		}
		else
		{
			//	エラー
			const int err = ::WSAGetLastError();
			switch (err)
			{
				case WSAEWOULDBLOCK:
				//	受信できるデータがない
				break;
				default:
					
					NOX_ERROR_LINE(nox::dev::net::log_id::DevNet, u8"受信エラー err={0}", err);
					break;
			}
			break;
		}
	}

	//	読み取りストリームに受信バッファを追加
	if (received_size > 0)
	{
		reader_.AddReceiveBuffer(std::span(receive_buffer.data(), received_size));
	}
}

void	nox::dev::editor_remote::EditorRemoteServer::OnConnected(const nox::dev::net::ConnectionContext& context)
{
	main_client_ = context;
}

void	nox::dev::editor_remote::EditorRemoteServer::OnDisconnected(const nox::dev::net::ConnectionContext& context)
{
	main_client_.socket = nox::dev::net::k_raw_invalid_socket;
}

void	nox::dev::editor_remote::EditorRemoteServer::RegisterRemoteInstance(nox::Object& object, nox::int64 instance_id)
{
	if (instance_id == 0)
	{
		instance_id = nox::os::atomic::Increment(instance_id_counter_);
	}
}

nox::int64 nox::dev::editor_remote::EditorRemoteServer::FindRemoteInstanceId(const nox::Object& object)const noexcept
{
	const auto it = remote_instance_id_dict_.find(&object);
	if (it != remote_instance_id_dict_.end())
	{
		return it->second;
	}
	return 0;
}

nox::Object* nox::dev::editor_remote::EditorRemoteServer::FindRemoteInstance(nox::int64 instance_id)const noexcept
{
	const auto it = remote_instance_dict_.find(instance_id);
	if (it != remote_instance_dict_.end())
	{
		return &it->second.get();
	}
	return nullptr;
}
#endif // NOX_DEVELOP