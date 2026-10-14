//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	socket_scheduler.cpp
///	@brief	socket_scheduler
#include	"pch.h"
#include	"socket_scheduler.h"

#include	"server.h"
#include	"client.h"
#include	"../../world_legacy.h"
#include	"dev_net_log_id.h"


namespace nox
{

}

namespace nox::dev::net
{
	namespace
	{
	}


}

std::span<const nox::SystemBase::PhaseRegister> nox::dev::net::SocketScheduler::GetPhaseRegisterList()const noexcept
{
	static constexpr auto table = std::array{
		PhaseRegister(k_phase_init),
		PhaseRegister(k_phase_terminate)
	};

	return table;
}

nox::dev::net::SocketScheduler::SocketScheduler()
{
}

nox::dev::net::SocketScheduler::~SocketScheduler()
{
}

void	nox::dev::net::SocketScheduler::Initialize(nox::legacy::World& world)
{
	const nox::os::network::Error error = nox::os::network::Initialize();
	NOX_ASSERT(error == nox::os::network::Error::None, nox::util::Format(u"network::Initialize failed. error={0}", static_cast<nox::int32>(nox::util::ToUnderlying(error))));

	thread_.SetThreadName(u"SocketScheduler");
	thread_.SetThreadPriority(nox::ThreadPriority::Lowest);
	thread_.Dispatch([this, &world]() {
		this->UpdateTask(world);
		});
}

void	nox::dev::net::SocketScheduler::Finalize([[maybe_unused]] nox::legacy::World& world)
{
	thread_.Wait();

	nox::os::network::Finalize();

}

void	nox::dev::net::SocketScheduler::UpdateTask(nox::legacy::World& world)
{
	while (world.IsKill() == false)
	{
		//	保留リストから本リストへ移動
		if (pending_server_list_.empty() == false)
		{
			NOX_LOCAL_SCOPE(nox::ScopedLock(mutex_server_list_));

			for (const auto& [server_ref, is_register] : pending_server_list_)
			{
				if (is_register)
				{
					server_list_.emplace_back(server_ref);
				}
				else
				{
					auto it = std::ranges::find_if(server_list_,
						[&server_ref](const std::reference_wrapper<Server>& r)noexcept
						{
							return std::addressof(r.get()) == std::addressof(server_ref.get());
						});
					if (it != server_list_.end())
					{
						server_list_.erase(it);
					}
				}
			}

			pending_server_list_.clear();
		}

		//	サーバーリストがない場合は1ms待機
		if (server_list_.empty())
		{
			nox::os::Sleep(10);
			continue;
		}

		//	リッスンsocketの新規接続を監視
		{
			//	サーバーは数個なので、kMaxPollEntries 件ずつスタック上で調べる
			std::array<nox::os::network::PollEntry, nox::os::network::kMaxPollEntries> entries{};
			for (std::size_t begin = 0; begin < server_list_.size(); begin += entries.size())
			{
				const std::size_t count = std::min(entries.size(), server_list_.size() - begin);
				for (std::size_t i = 0; i < count; ++i)
				{
					//	ソケット登録
					entries[i].socket = server_list_[begin + i].get().GetSocket();
					entries[i].requested = nox::os::network::PollEvent::Readable;
				}

				//	1ms 待つ
				const auto poll_result = nox::os::network::Poll(std::span(entries.data(), count), 1);
				if (poll_result.has_value() == false)
				{
					if (poll_result.error() != nox::os::network::Error::Interrupted)
					{
						NOX_ERROR_LINE(nox::dev::net::log_id::DevNet, u"poll() failed. error={0}", static_cast<nox::int32>(nox::util::ToUnderlying(poll_result.error())));
					}
					continue;
				}

				for (std::size_t i = 0; i < count; ++i)
				{
					nox::dev::net::Server& server = server_list_[begin + i].get();

					//	新規接続（リッスンソケットにイベントがあった場合のみ）
					if (nox::util::IsBitAnd(entries[i].returned, nox::os::network::PollEvent::Readable))
					{
						server.Connection();
					}

					//	受け付け処理
					server.Update(world);
				}
			}
		}
	}
}

void	nox::dev::net::SocketScheduler::DoConnectionServerClient()
{
}

void	nox::dev::net::SocketScheduler::RegisterEntity(Server& entity)
{
	NOX_LOCAL_SCOPE(nox::ScopedLock(mutex_server_list_));
	pending_server_list_.emplace_back(std::make_tuple(std::ref(entity), true));
}

void	nox::dev::net::SocketScheduler::RegisterEntity(Client&)
{
}

void	nox::dev::net::SocketScheduler::UnregisterEntity(nox::dev::net::Server& entity)
{
	NOX_LOCAL_SCOPE(nox::ScopedLock(mutex_server_list_));

	auto it = std::ranges::find_if(server_list_,
		[&entity](const std::reference_wrapper<Server>& r)noexcept
		{
			return std::addressof(r.get()) == std::addressof(entity);
		});

	if (it != server_list_.end())
	{
		pending_server_list_.emplace_back(std::make_tuple(std::ref(entity), false));
	}
}

void	nox::dev::net::SocketScheduler::UnregisterEntity(Client&)
{

}
