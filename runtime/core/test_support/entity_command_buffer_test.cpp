// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	entity_command_buffer_test.cpp
/// @brief	entity_command_buffer_test
#include "pch.h"
#include "test.h"

#include "../entity_command_buffer.h"
#include "../../kernel/assertion.h"

void nox::test::TestEntityCommandBuffer()
{
	nox::EntityCommandBuffer<2> commands;
	const nox::EntityId entity{
		.generation = 1u,
		.index = 42u,
	};

	NOX_ASSERT(commands.TryDestroy(entity), u"EntityCommandBufferへの記録に失敗しました");
	nox::EntityCommand recorded{};
	NOX_ASSERT(commands.TryGet(0u, recorded), u"EntityCommandBufferの記録取得に失敗しました");
	NOX_ASSERT(commands.GetLength() == 1u, u"EntityCommandBufferの記録数が不正です");
	NOX_ASSERT(recorded.type == nox::EntityCommandType::Destroy, u"EntityCommandBufferのコマンド種別が不正です");
	NOX_ASSERT(recorded.entity_raw == entity.raw, u"EntityCommandBufferのEntityIdが不正です");

	commands.Clear();
	NOX_ASSERT(commands.GetLength() == 0u, u"EntityCommandBufferのClear結果が不正です");

	constexpr nox::uint32 command_count = 64u;
	constexpr nox::uint32 producer_count = 4u;
	constexpr nox::uint32 commands_per_producer = command_count / producer_count;
	nox::EntityCommandBuffer<command_count> concurrent_commands;
	std::atomic<nox::uint32> recorded_count{ 0u };
	std::array<std::thread, producer_count> producers;
	for (nox::uint32 producer_index = 0u; producer_index < producers.size(); ++producer_index)
	{
		producers[producer_index] = std::thread([&concurrent_commands, &recorded_count, producer_index]()
			{
				for (nox::uint32 command_index = 0u; command_index < commands_per_producer; ++command_index)
				{
					const nox::EntityId concurrent_entity{
						(static_cast<nox::uint64>(producer_index) << 32u) | command_index
					};
					if (concurrent_commands.TryDestroy(concurrent_entity))
					{
						recorded_count.fetch_add(1u, std::memory_order_relaxed);
					}
				}
			});
	}
	for (std::thread& producer : producers)
	{
		producer.join();
	}

	concurrent_commands.BeginPlayback();
	NOX_ASSERT(recorded_count.load(std::memory_order_relaxed) == command_count, u"並列EntityCommandBufferの記録数が不正です");
	for (nox::uint32 command_index = 0u; command_index < command_count; ++command_index)
	{
		nox::EntityCommand concurrent_command{};
		NOX_ASSERT(concurrent_commands.TryGet(command_index, concurrent_command), u"並列EntityCommandBufferのコマンド取得に失敗しました");
		NOX_ASSERT(concurrent_command.type == nox::EntityCommandType::Destroy, u"並列EntityCommandBufferのコマンド種別が不正です");
	}
	concurrent_commands.Clear();
}
