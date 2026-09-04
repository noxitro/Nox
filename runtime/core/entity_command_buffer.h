// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	entity_command_buffer.h
/// @brief	Deferred, fixed-capacity entity structural changes.
/// @details 構造変更2系統のうちの「遅延系」。記録側はジョブセーフ(ロックフリー)で、
///          反映はフェーズ終端の排他的なPlaybackポイントでまとめて行われる。
///
///          確保は一切走らない。コマンド列も、ComponentDataの初期値を運ぶペイロード領域も
///          テンプレート引数で決まる固定長の配列で、溢れたら記録が false を返すだけ。
#pragma once
#include <array>
#include <atomic>
#include <bit>
#include <cstddef>
#include <cstring>
#include <thread>
#include "entity.h"
#include "component_type.h"

namespace nox
{
	enum class EntityCommandType : nox::uint8
	{
		Destroy,
		AddComponent,
		RemoveComponent,
	};

	struct EntityCommand
	{
		nox::EntityCommandType type;
		nox::uint64 entity_raw;
		/// @brief AddComponent / RemoveComponent の対象型。Destroyではnullptr。
		const nox::ComponentTypeInfo* type_info;
		/// @brief AddComponentが運ぶ初期値の、ペイロード領域内でのオフセット。
		nox::uint32 payload_offset;
		/// @brief 初期値のバイト数。0なら「初期値なし(ゼロ初期化のまま)」。
		nox::uint32 payload_size;
	};

	/// @brief 遅延構造変更のコマンドバッファ。
	/// @tparam _Capacity      記録できるコマンド数の上限。
	/// @tparam _PayloadBytes  AddComponentが運ぶ初期値の総バイト数の上限。
	template<nox::uint32 _Capacity, nox::uint32 _PayloadBytes = _Capacity * 64u>
		requires (_Capacity > 0u)
	class EntityCommandBuffer final
	{
	public:
		static constexpr nox::uint32 Capacity = _Capacity;
		static constexpr nox::uint32 PayloadCapacity = _PayloadBytes;

		[[nodiscard]]
		inline bool TryDestroy(const nox::EntityId entity) noexcept
		{
			return TryRecord(nox::EntityCommand{
				.type = nox::EntityCommandType::Destroy,
				.entity_raw = entity.raw,
				.type_info = nullptr,
				.payload_offset = 0u,
				.payload_size = 0u,
				});
		}

		/// @brief ComponentDataの追加を予約する。sourceがnullptrでなければ初期値を複製して運ぶ。
		/// @details ComponentDataは常にtrivially copyable(nox::IsComponentDataTypeの要件)なので、
		///          単純なmemcpyで運べる。コマンド枠かペイロード枠のどちらかが尽きたらfalse。
		[[nodiscard]]
		inline bool TryAddComponent(
			const nox::EntityId entity,
			const nox::ComponentTypeInfo& type_info,
			const void* const source) noexcept
		{
			//	ペイロードの確保と書き込みもproducerスコープの内側で行う。
			//	外に出すと、BeginPlaybackの「進行中の記録完了を待つ」保証が
			//	コマンド枠だけにかかり、初期値の書き込みには及ばなくなる。
			if (TryEnterProducer() == false)
			{
				return false;
			}

			nox::uint32 payload_offset = 0u;
			nox::uint32 payload_size = 0u;
			if (source != nullptr && type_info.size != 0u)
			{
				if (TryAllocatePayload(type_info.size, type_info.alignment, payload_offset) == false)
				{
					LeaveProducer();
					return false;
				}
				std::memcpy(GetPayloadBytes() + payload_offset, source, type_info.size);
				payload_size = type_info.size;
			}

			const bool recorded = TryRecordLocked(nox::EntityCommand{
				.type = nox::EntityCommandType::AddComponent,
				.entity_raw = entity.raw,
				.type_info = &type_info,
				.payload_offset = payload_offset,
				.payload_size = payload_size,
				});
			LeaveProducer();
			return recorded;
		}

		[[nodiscard]]
		inline bool TryRemoveComponent(
			const nox::EntityId entity,
			const nox::ComponentTypeInfo& type_info) noexcept
		{
			return TryRecord(nox::EntityCommand{
				.type = nox::EntityCommandType::RemoveComponent,
				.entity_raw = entity.raw,
				.type_info = &type_info,
				.payload_offset = 0u,
				.payload_size = 0u,
				});
		}

		/// @brief コマンドが運ぶ初期値の先頭。運んでいない場合はnullptr。
		[[nodiscard]]
		inline const void* TryGetPayload(const nox::EntityCommand& command)const noexcept
		{
			if (command.payload_size == 0u)
			{
				return nullptr;
			}
			return GetPayloadBytes() + command.payload_offset;
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

		/// @brief 使用済みのペイロードバイト数。溢れの検証と診断のために公開している。
		[[nodiscard]]
		inline nox::uint32 GetPayloadLength()const noexcept
		{
			return payload_used_.load(std::memory_order_acquire);
		}

		/// @brief これまでに到達した最大コマンド数(high-water mark)。
		/// @details 溢れると abort する設計なので、出荷前に容量を根拠づけるための安全弁。
		///          Clearでは戻さない。World寿命を通じた最大値を保持する。
		[[nodiscard]]
		inline nox::uint32 GetPeakLength()const noexcept
		{
			return peak_length_.load(std::memory_order_acquire);
		}

		/// @brief これまでに到達した最大ペイロードバイト数(high-water mark)。
		[[nodiscard]]
		inline nox::uint32 GetPeakPayloadLength()const noexcept
		{
			return peak_payload_used_.load(std::memory_order_acquire);
		}

		inline void Clear()noexcept
		{
			const nox::uint32 length = GetLength();
			const nox::uint32 payload_used = GetPayloadLength();

			//	Clearはフェーズ終端の排他区間からしか呼ばれないので、ここでまとめて最大値を更新する。
			//	記録のたびにCASで更新するより、ホットパスに何も足さずに済む。
			if (length > peak_length_.load(std::memory_order_relaxed))
			{
				peak_length_.store(length, std::memory_order_relaxed);
			}
			if (payload_used > peak_payload_used_.load(std::memory_order_relaxed))
			{
				peak_payload_used_.store(payload_used, std::memory_order_relaxed);
			}

			for (nox::uint32 index = 0u; index < length; ++index)
			{
				ready_[index].store(false, std::memory_order_relaxed);
			}
			length_.store(0u, std::memory_order_release);
			payload_used_.store(0u, std::memory_order_release);
			producer_state_.store(0u, std::memory_order_release);
		}

	private:
		static constexpr nox::uint32 k_playback_bit = 0x80000000u;
		static constexpr nox::uint32 k_producer_count_mask = ~k_playback_bit;
		/// @brief ペイロード領域を std::max_align_t 何個分で持つか(切り上げ)。
		static constexpr nox::uint32 k_payload_word_size = static_cast<nox::uint32>(sizeof(std::max_align_t));
		static constexpr nox::uint32 k_payload_word_count =
			(PayloadCapacity + k_payload_word_size - 1u) / k_payload_word_size;

		/// @brief コマンド1件を記録する。枠が尽きているかPlayback中ならfalse。
		[[nodiscard]]
		inline bool TryRecord(const nox::EntityCommand& command)noexcept
		{
			if (TryEnterProducer() == false)
			{
				return false;
			}

			const bool recorded = TryRecordLocked(command);
			LeaveProducer();
			return recorded;
		}

		/// @brief producerスコープに入っている前提で、コマンド1件を記録する。
		[[nodiscard]]
		inline bool TryRecordLocked(const nox::EntityCommand& command)noexcept
		{
			nox::uint32 index = length_.load(std::memory_order_relaxed);
			while (index < Capacity)
			{
				if (length_.compare_exchange_weak(
					index,
					index + 1u,
					std::memory_order_relaxed,
					std::memory_order_relaxed))
				{
					commands_[index] = command;
					ready_[index].store(true, std::memory_order_release);
					return true;
				}
			}
			return false;
		}

		/// @brief ペイロード領域をアラインして切り出す。バンプするだけなので解放はClearの一括のみ。
		[[nodiscard]]
		inline bool TryAllocatePayload(
			const nox::uint32 size,
			const nox::uint32 alignment,
			nox::uint32& out_offset)noexcept
		{
			const nox::uint32 mask = (alignment > 1u) ? (alignment - 1u) : 0u;
			nox::uint32 offset = payload_used_.load(std::memory_order_relaxed);
			for (;;)
			{
				const nox::uint32 aligned = (offset + mask) & ~mask;
				//	PayloadCapacity からの引き算で判定して、加算のオーバーフローを避ける。
				if (aligned > PayloadCapacity || size > (PayloadCapacity - aligned))
				{
					return false;
				}

				if (payload_used_.compare_exchange_weak(
					offset,
					aligned + size,
					std::memory_order_relaxed,
					std::memory_order_relaxed))
				{
					out_offset = aligned;
					return true;
				}
			}
		}

		/// @brief ペイロード領域をバイト列として見る。
		/// @details 記憶域を std::max_align_t の配列で持っているので、先頭は必ず最大基本アラインメント。
		///          そこからのオフセットを型のアラインメントへ切り上げれば、実アドレスも整う。
		[[nodiscard]]
		inline nox::uint8* GetPayloadBytes()noexcept
		{
			return reinterpret_cast<nox::uint8*>(payload_.data());
		}

		[[nodiscard]]
		inline const nox::uint8* GetPayloadBytes()const noexcept
		{
			return reinterpret_cast<const nox::uint8*>(payload_.data());
		}

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
		/// @brief ComponentDataの初期値を運ぶ固定長領域。
		/// @details std::max_align_t の配列として持つことで、alignas を書かずに
		///          最大基本アラインメントを得ている(alignas を書くとMSVCがC4324で鳴く)。
		std::array<std::max_align_t, k_payload_word_count> payload_{};
		std::atomic<nox::uint32> length_{ 0u };
		std::atomic<nox::uint32> payload_used_{ 0u };
		std::atomic<nox::uint32> producer_state_{ 0u };
		/// @brief 容量の妥当性を出荷前に確かめるためのhigh-water mark。Clearで更新し、リセットはしない。
		std::atomic<nox::uint32> peak_length_{ 0u };
		std::atomic<nox::uint32> peak_payload_used_{ 0u };
	};
}
