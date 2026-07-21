//	Copyright (c) 2025 NOX ENGINE All rights reserved.

///	@file	remote_host.cpp
///	@brief	remote_host
#include	"pch.h"
#include	"editor_remote_server.h"

#if NOX_DEVELOP
#include	"net/socket_scheduler.h"
#include	"editor_remote_response.h"
#include	"editor_remote_query.h"
#include	"net/dev_net_api.h"
#include	"net/dev_net_log_id.h"
#include	"../world.h"
#include	"../log_service.h"
#include	"../object.h"
#include	"remote/remote_system.g.h"

namespace nox::dev::editor_remote
{
	struct EditorRemoteServer::Impl
	{
		nox::LogService log_service;
		nox::Vector<nox::IntrusivePtr<nox::Object>> editor_owned_remote_instances;

		inline Impl() noexcept
		{
			nox::debug::AttachLogHandler([&](const nox::debug::LogHandlerArgs& args) {
				log_service.LogHandler(args);
				});
		}

		inline ~Impl() noexcept
		{
			nox::debug::AttachLogHandler(nullptr);
		}
	};
}

nox::dev::editor_remote::EditorRemoteServer::EditorRemoteServer():
	event_handler_(*this),
	server_(event_handler_),
	query_id_counter_(0),
	instance_id_counter_(0),
	response_dict_{},
	writer_(*this),
   reader_(*this),
	socket_scheduler_(nullptr),
	main_client_{},
	impl_(new Impl())
{
	
}

nox::dev::editor_remote::EditorRemoteServer::~EditorRemoteServer()
{
	Shutdown();
	delete impl_;
	impl_ = nullptr;
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
		const auto send_result = server_.Send(main_client_.socket, static_cast<const void*>(buffer.data()), static_cast<nox::int32>(buffer.size()));
		if (send_result.has_value() == false)
		{
			NOX_ERROR_LINE(nox::dev::net::log_id::DevNet, u8"送信エラー size={0}", buffer.size());
		}
	}
}

void	nox::dev::editor_remote::EditorRemoteServer::Start(nox::World& world)
{
    const bool started = server_.Startup(nox::dev::net::Server::InitializeContext{
		.max_connection = 1,
		.port = 86,
		});

	if (started && socket_scheduler_ == nullptr)
	{
		socket_scheduler_ = world.FindSystem<nox::dev::net::SocketScheduler>();
		NOX_ASSERT(socket_scheduler_ != nullptr, u"SocketSchedulerが登録されていません");
		if (socket_scheduler_ != nullptr)
		{
			socket_scheduler_->RegisterEntity(server_);
		}
	}
}

void nox::dev::editor_remote::EditorRemoteServer::Shutdown()
{
	if (socket_scheduler_ != nullptr)
	{
		socket_scheduler_->UnregisterEntity(server_);
		socket_scheduler_ = nullptr;
	}

	server_.Shutdown();
}

void nox::dev::editor_remote::EditorRemoteServer::Terminate([[maybe_unused]] nox::World& world)
{
	Shutdown();
}

void	nox::dev::editor_remote::EditorRemoteServer::Update(nox::World& world)
{
	if (main_client_.socket != nox::dev::net::k_raw_invalid_socket)
	{
		UpdateReceive(world);
	}
}

void nox::dev::editor_remote::EditorRemoteServer::UpdateReceive(nox::World& world)
{
	if (reader_.GetReceivedSize() <= 0)
	{
		return;
	}

	alignas(alignof(std::max_align_t)) std::array<nox::uint8, 1024> entity_buffer{ 0 };
	alignas(alignof(std::max_align_t)) std::array<nox::uint8, 4096> receive_buffer{ 0 };
		std::array<nox::char8, 256> entity_name_buffer{};

    NOX_LOCAL_SCOPE(nox::os::ScopedLock(mutex_reader_));

	//	1パケット分が読み取れる限りループ
	while (reader_.CanReadBody())
	{
		//	ヘッダ部分を空読み
		reader_.SkipHeader();

		//	entity名を読み取り
		std::u8string_view entity_full_name = this->reader_.ReadString(entity_name_buffer);
		if (entity_full_name.empty() == false && entity_full_name.back() == u8'\0')
		{
			entity_full_name.remove_suffix(1);
		}

		const nox::reflection::ClassInfo*const class_info = nox::reflection::FindClassInfo(entity_full_name);
		NOX_ASSERT(class_info != nullptr, u"不明なEntity:{0}", entity_full_name);

		bool is_heap_entity = false;
		nox::dev::editor_remote::EditorRemoteEntity* entity = static_cast<nox::dev::editor_remote::EditorRemoteEntity*>(class_info->GetType().CreateObject(entity_buffer));
		if (entity == nullptr)
		{
			entity = static_cast<nox::dev::editor_remote::EditorRemoteEntity*>(class_info->GetType().CreateObject());
			is_heap_entity = true;
		}
		NOX_ASSERT(entity != nullptr, u"EditorIpcEntityの生成に失敗:{0}", entity_full_name);

		NOX_LOCAL_SCOPE(nox::util::ScopeExit([entity, is_heap_entity]() {
			if (is_heap_entity)
			{
				delete entity;
			}
			else
			{
				std::destroy_at(entity);
			}
			}));

		//	query
		if (class_info->IsSubclassOf<nox::dev::editor_remote::Query>())
		{
			nox::dev::editor_remote::Query& query = static_cast<nox::dev::editor_remote::Query&>(*entity);
			query.Deserialize(reader_);

			//	レスポンス生成
			{
				nox::PlacementObject<nox::dev::editor_remote::Response> response = query.Execute(world, receive_buffer);
				if (response != nullptr)
				{
                    NOX_LOCAL_SCOPE(nox::os::ScopedLock(mutex_writer_));
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

void nox::dev::editor_remote::EditorRemoteServer::OnServerReceive(nox::World& world)
{
	//	受信バッファ 未初期化でOK
	std::array<nox::uint8, 2048> receive_buffer;

	if (world.IsKill())
	{
		return;
	}

	const nox::int32 receive_size = nox::dev::net::Receive(main_client_.socket, static_cast<char*>(static_cast<void*>(receive_buffer.data())), static_cast<nox::int32>(receive_buffer.size()));
	if (receive_size > 0)
	{
		{
			NOX_LOCAL_SCOPE(nox::os::ScopedLock(mutex_reader_));
			reader_.AddReceiveBuffer(std::span(receive_buffer.data(), static_cast<std::size_t>(receive_size)));
		}
		UpdateReceive(world);
	}
	else if (receive_size < 0)
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
	}
}

void	nox::dev::editor_remote::EditorRemoteServer::OnServerConnected(const nox::dev::net::ConnectionContext& context)
{
	main_client_ = context;

	//	log_serviceへサーバーを登録
	impl_->log_service.AttachServer(*this);
}

void	nox::dev::editor_remote::EditorRemoteServer::OnServerDisconnected([[maybe_unused]] const nox::dev::net::ConnectionContext& context)
{
	main_client_.socket = nox::dev::net::k_raw_invalid_socket;

	impl_->log_service.DetachServer();
}

void	nox::dev::editor_remote::EditorRemoteServer::RegisterRemoteInstance(nox::Object& object, nox::int64 instance_id)
{
   const auto registered_it = remote_instance_id_dict_.find(&object);
	if (registered_it != remote_instance_id_dict_.end())
	{
       return;
	}

	if (instance_id == 0)
	{
       instance_id = -nox::os::atomic::Increment(instance_id_counter_);
	}

	remote_instance_dict_.emplace(instance_id, std::ref(object));
	remote_instance_id_dict_.emplace(&object, instance_id);
}

void nox::dev::editor_remote::EditorRemoteServer::RegisterEditorOwnedRemoteInstance(nox::Object& object, nox::int64 instance_id)
{
	RegisterRemoteInstance(object, instance_id);

	for (const nox::IntrusivePtr<nox::Object>& instance : impl_->editor_owned_remote_instances)
	{
		if (instance.Get() == &object)
		{
			return;
		}
	}

	nox::IntrusivePtr<nox::Object> keep_alive;
	keep_alive.Reset(&object);
	impl_->editor_owned_remote_instances.emplace_back(std::move(keep_alive));
}

bool nox::dev::editor_remote::EditorRemoteServer::UnregisterRemoteInstance(nox::int64 instance_id)
{
	const auto remote_instance_it = remote_instance_dict_.find(instance_id);
	if (remote_instance_it == remote_instance_dict_.end())
	{
		return false;
	}

	nox::Object& object = remote_instance_it->second.get();
	remote_instance_id_dict_.erase(&object);
	remote_instance_dict_.erase(remote_instance_it);

	for (auto it = impl_->editor_owned_remote_instances.begin(); it != impl_->editor_owned_remote_instances.end();)
	{
		if (it->Get() == &object)
		{
			it = impl_->editor_owned_remote_instances.erase(it);
		}
		else
		{
			++it;
		}
	}

	return true;
}

bool nox::dev::editor_remote::EditorRemoteServer::NotifyRemoteInstanceDestroyed(nox::Object& object)
{
	const nox::int64 instance_id = FindRemoteInstanceId(object);
	if (instance_id == 0)
	{
		return false;
	}

	nox::dev::editor_remote::RuntimeObjectDestroyedQuery query;
	query.SetRemoteInstanceId(instance_id);
	SendQuery(query);
	UnregisterRemoteInstance(instance_id);
	return instance_id > 0;
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

void nox::dev::editor_remote::EditorRemoteServer::CollectRemoteInstances(std::function<void(nox::int64, const nox::Object&)> evaluate)const
{
	for (const auto& pair : remote_instance_dict_)
	{
		evaluate(pair.first, pair.second.get());
	}
}

std::span<const nox::SystemBase::PhaseRegister> nox::dev::editor_remote::EditorRemoteServer::GetPhaseRegisterList()const noexcept
{
	static constexpr auto table = std::to_array({
		PhaseRegister(k_phase_init, nox::dev::net::SocketScheduler::k_phase_init),
		PhaseRegister(k_phase_update, 
			{nox::dev::net::SocketScheduler::k_phase_socket_update}
			),
		PhaseRegister(k_phase_terminate,
			{},
			{nox::dev::net::SocketScheduler::k_phase_terminate}
			)
	});

	return table;
}
#endif // NOX_DEVELOP
