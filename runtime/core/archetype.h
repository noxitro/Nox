// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	archetype.h
/// @brief	ComponentDataの組み合わせごとに連続配置するSoAストレージ。
/// @details Archetype = ComponentDataの組み合わせ。同一Archetypeのentityは固定長のChunkに詰められ、
///          EntityIdの列と各ComponentDataの列が同じ並び順で置かれる。列の先頭さえ取れれば
///          あとは行インデックスだけで全ての列にアクセスできる。
#pragma once
#include	"component_type.h"
#include	"entity.h"

namespace nox
{
	/// @brief 1Chunkのバイト数。L2に載る粒度で、ジョブ分割の単位も兼ねる。
	inline constexpr nox::uint32 k_archetype_chunk_byte_size = 16u * 1024u;

	/// @brief 1Archetypeが持てるComponentData型数の上限。
	inline constexpr nox::uint32 k_max_component_type_per_archetype = 32u;

	inline constexpr nox::uint32 k_invalid_archetype_chunk_index = std::numeric_limits<nox::uint32>::max();

	/// @brief Archetype内でのentityの位置。
	struct ArchetypeLocation final
	{
		nox::uint32 chunk_index;
		nox::uint32 row;

		[[nodiscard]] inline constexpr bool IsValid()const noexcept
		{
			return chunk_index != nox::k_invalid_archetype_chunk_index;
		}

		[[nodiscard]] static inline constexpr nox::ArchetypeLocation Invalid()noexcept
		{
			return nox::ArchetypeLocation{ nox::k_invalid_archetype_chunk_index, 0u };
		}
	};

	/// @brief 単一のComponentData組み合わせに対応するストレージ。
	/// @details ChunkはComponentDataが増えたときにだけ確保される。行の追加・削除自体はアロケーションなし。
	class Archetype final
	{
	public:
		/// @brief Chunk1つ分のメタデータ。実データはblock先頭からのオフセットで参照する。
		struct Chunk final
		{
			nox::uint8* block;
			nox::uint32 count;
		};

	public:
		/// @param component_types ComponentTypeIndexの昇順である必要はない(内部でソートする)。
		Archetype(const nox::ComponentMask& mask, std::span<const nox::ComponentTypeInfo* const> component_types);
		~Archetype();

		Archetype(const Archetype&) = delete;
		Archetype& operator=(const Archetype&) = delete;
		Archetype(Archetype&&) = delete;
		Archetype& operator=(Archetype&&) = delete;

		/// @brief 末尾にentityを1行追加する。ComponentDataはゼロ初期化される。
		[[nodiscard]] nox::ArchetypeLocation AddEntity(nox::EntityId entity);

		/// @brief 指定行をswap-removeで詰める。
		/// @return 詰めるために移動してきたentity。移動が発生しなかった場合はraw==0。
		nox::EntityId RemoveEntity(nox::ArchetypeLocation location)noexcept;

		/// @brief 指定Chunkにおける指定ComponentDataの列の先頭。宣言外の型を渡すとnullptr。
		[[nodiscard]] void* TryGetComponentArray(nox::uint32 chunk_index, nox::ComponentTypeIndex type_index)noexcept;

		/// @brief 指定Chunkにおけるentity列の先頭。
		[[nodiscard]] nox::EntityId* GetEntityArray(nox::uint32 chunk_index)noexcept;

		[[nodiscard]] inline const nox::ComponentMask& GetMask()const noexcept { return mask_; }
		[[nodiscard]] inline nox::uint32 GetChunkCapacity()const noexcept { return chunk_capacity_; }
		[[nodiscard]] inline nox::uint32 GetChunkCount()const noexcept { return static_cast<nox::uint32>(chunks_.size()); }
		[[nodiscard]] inline nox::uint32 GetEntityCount()const noexcept { return entity_count_; }
		[[nodiscard]] nox::uint32 GetChunkEntityCount(nox::uint32 chunk_index)const noexcept;

		[[nodiscard]] inline std::span<const nox::ComponentTypeIndex> GetTypeIndices()const noexcept
		{
			return std::span(type_indices_.data(), type_count_);
		}

		/// @brief 他のArchetypeへ1行分のComponentDataを、双方が持つ型についてのみコピーする。
		void CopySharedComponents(
			nox::ArchetypeLocation source_location,
			nox::Archetype& destination,
			nox::ArchetypeLocation destination_location)noexcept;

	private:
		/// @brief type_indices_内の位置。持っていなければ-1。
		[[nodiscard]] nox::int32 FindTypeSlot(nox::ComponentTypeIndex type_index)const noexcept;

		void PushChunk();

	private:
		nox::ComponentMask mask_;
		std::array<nox::ComponentTypeIndex, nox::k_max_component_type_per_archetype> type_indices_;
		/// @brief type_indices_と並行。Chunk先頭からの列オフセット。
		std::array<nox::uint32, nox::k_max_component_type_per_archetype> column_offsets_;
		/// @brief type_indices_と並行。1要素のバイト数。
		std::array<nox::uint32, nox::k_max_component_type_per_archetype> component_sizes_;
		nox::Vector<nox::Archetype::Chunk> chunks_;
		nox::uint32 type_count_;
		nox::uint32 chunk_capacity_;
		nox::uint32 entity_count_;
	};
}
