// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	entity_command_buffer.h
/// @brief	Deferred, fixed-capacity entity structural changes.
#pragma once
#include <array>
#include <atomic>
#include <thread>
#include "entity.h"

namespace nox
{
	enum class EntityCommandType : nox::uint8
	{
		Destroy,
	};

	struct EntityCommand
	{
		nox::EntityCommandType type;
		nox::uint64 entity_raw;
	};

	template<nox::uint32 _Capacity>
		requires (_Capacity > 0u)
	class EntityCommandBuffer final
	{
	public:
		static constexpr nox::uint32 Capacity = _Capacity;

		[[nodiscard]]
		inline bool TryDestroy(const nox::EntityId entity) noexcept
		{
			if (TryEnterProducer() == false)
			{
				return false;
			}

			nox::uint32 index = length_.load(std::memory_order_relaxed);
			while (index < Capacity)
			{
				if (length_.compare_exchange_weak(
					index,
					index + 1u,
					std::memory_order_relaxed,
					std::memory_order_relaxed))
				{
					commands_[index] = nox::EntityCommand{
						.type = nox::EntityCommandType::Destroy,
						.entity_raw = entity.raw,
					};
					ready_[index].store(true, std::memory_order_release);
					LeaveProducer();
					return true;
				}
			}
			LeaveProducer();
			return false;
		}

		/// @brief Playbackを開始し、新しい構造変更を拒否して進行中の記録完了を待機する。
		inline void BeginPlayback()noexcept
		{
			nox::uint32 state = producer_state_.load(std::memory_order_acquire);
			while ((state & k_playback_bit) == 0u)
			{
				if (producer_state_.compare_exchange_weak(
					state,
					state | k_playback_bit,
					std::memory_order_acq_rel,
					std::memory_order_acquire))
				{
					break;
				}
			}

			while ((producer_state_.load(std::memory_order_acquire) & k_producer_count_mask) != 0u)
			{
				std::this_thread::yield();
			}
		}

		[[nodiscard]]
		inline bool TryGet(const nox::uint32 index, nox::EntityCommand& out)const noexcept
		{
			if (index >= GetLength() || ready_[index].load(std::memory_order_acquire) == false)
			{
				return false;
			}

			out = commands_[index];
			return true;
		}

		[[nodiscard]]
		inline nox::uint32 GetLength()const noexcept
		{
			return length_.load(std::memory_order_acquire);
		}

		inline void Clear()noexcept
		{
			const nox::uint32 length = GetLength();
			for (nox::uint32 index = 0u; index < length; ++index)
			{
				ready_[index].store(false, std::memory_order_relaxed);
			}
			length_.store(0u, std::memory_order_release);
			producer_state_.store(0u, std::memory_order_release);
		}

	private:
		static constexpr nox::uint32 k_playback_bit = 0x80000000u;
		static constexpr nox::uint32 k_producer_count_mask = ~k_playback_bit;

		[[nodiscard]]
		inline bool TryEnterProducer()noexcept
		{
			nox::uint32 state = producer_state_.load(std::memory_order_acquire);
			while ((state & k_playback_bit) == 0u)
			{
				if (producer_state_.compare_exchange_weak(
					state,
					state + 1u,
					std::memory_order_acquire,
					std::memory_order_acquire))
				{
					return true;
				}
			}
			return false;
		}

		inline void LeaveProducer()noexcept
		{
			producer_state_.fetch_sub(1u, std::memory_order_release);
		}

	private:
		std::array<nox::EntityCommand, Capacity> commands_{};
		std::array<std::atomic_bool, Capacity> ready_{};
		std::atomic<nox::uint32> length_{ 0u };
		std::atomic<nox::uint32> producer_state_{ 0u };
	};
}
