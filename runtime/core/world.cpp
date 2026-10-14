//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	world.cpp
///	@brief	world
#include	"pch.h"
#include	"world.h"

#include	"world_registery.h"
#include	"node_descriptor.h"
#include	"engine_module.h"

namespace nox
{
	namespace
	{
		/// @brief 空のArchetypeのID。Componentが1つもないEntityが所属する。
		inline constexpr nox::uint32 kEmptyArchetypeId = 0u;
		/// @brief 1Archetypeが持てるComponentの数の上限
		inline constexpr nox::uint16 kArchetypeComponentMaxCount = 255u;

		/// @brief 1Chunkのバイト数
		inline constexpr nox::uint32 kChunkByteSize = 16u * 1024u;
		/// @brief Chunkの先頭のアラインメント。Componentのアラインメントの上限(16)に合わせる
		inline constexpr nox::uint32 kChunkAlignment = 16u;

		[[nodiscard]] inline constexpr nox::uint32 AlignUp(const nox::uint32 value, const nox::uint32 alignment)noexcept
		{
			return (value + alignment - 1u) & ~(alignment - 1u);
		}
	}
}

/// @details	Componentの構成ごとに、Entityを固定長のChunkへ詰めて並べる(SoA)。
///				Chunkの中は [Entityの列][Component 0の列][Component 1の列]... の順。
///				行番号は全Chunkを通した通し番号で、row / chunk_capacity_ がChunk、row % chunk_capacity_ がChunk内の位置。
///				Componentはtrivially copyableであることを前提に、移動と詰め直しをmemcpyで行う。
class nox::World::Archetype final
{
	/// @brief Componentの列の配置
	struct Column final
	{
		/// @brief Chunk先頭からのオフセット
		nox::uint32 offset;
		/// @brief 1要素のバイト数
		nox::uint32 size;
	};

public:
	static constexpr nox::uint32 kInvalidColumn = std::numeric_limits<nox::uint32>::max();

public:
	/// @param component_id_list 昇順に並んだComponent ID
	explicit Archetype(const std::span<const nox::uint16> component_id_list)noexcept:
		component_id_list_(std::from_range, component_id_list),
		chunk_capacity_(0u),
		entity_count_(0u)
	{
		NOX_ASSERT(std::ranges::is_sorted(component_id_list), u8"Component IDは昇順で渡してください");
		NOX_ASSERT(component_id_list.size() <= kArchetypeComponentMaxCount, u8"1Archetypeが持てるComponentの数を超えました");

		//	1行あたりのバイト数と、列ごとのアラインメント調整で出る余白の上限
		nox::uint32 row_stride = static_cast<nox::uint32>(sizeof(nox::Entity));
		nox::uint32 padding_budget = 0u;
		for (const nox::uint16 component_id : component_id_list)
		{
			const nox::reflection::Type& type = nox::detail::GetComponentType(component_id);
			NOX_ASSERT(type.IsTypeAttributeFlag(nox::reflection::TypeAttributeFlag::TrivialCopyable), u8"Componentはtrivially copyableである必要があります: {0}", type.GetTypeName());
			row_stride += type.GetTypeSize();
			padding_budget += type.GetAlignmentOf();
		}

		NOX_ASSERT(row_stride + padding_budget <= kChunkByteSize, u8"Componentの合計サイズがChunkに収まりません");
		chunk_capacity_ = (kChunkByteSize - padding_budget) / row_stride;

		columns_.reserve(component_id_list.size());
		nox::uint32 offset = chunk_capacity_ * static_cast<nox::uint32>(sizeof(nox::Entity));
		for (const nox::uint16 component_id : component_id_list)
		{
			const nox::reflection::Type& type = nox::detail::GetComponentType(component_id);
			offset = AlignUp(offset, type.GetAlignmentOf());
			columns_.emplace_back(Column{ .offset = offset, .size = type.GetTypeSize() });
			offset += chunk_capacity_ * type.GetTypeSize();
		}
		NOX_ASSERT(offset <= kChunkByteSize, u8"Chunkのレイアウト計算が破綻しています");
	}

	~Archetype()
	{
		for (std::byte* const chunk : chunks_)
		{
			nox::memory::Deallocate(chunk, kChunkAlignment);
		}
	}

	Archetype(const Archetype&) = delete;
	Archetype& operator=(const Archetype&) = delete;
	Archetype(Archetype&&) = delete;
	Archetype& operator=(Archetype&&) = delete;

	[[nodiscard]] inline std::span<const nox::uint16> GetComponentIdList()const noexcept
	{
		return component_id_list_;
	}

	[[nodiscard]] inline nox::uint32 GetEntityCount()const noexcept
	{
		return entity_count_;
	}

	/// @brief Componentの列番号を返す
	/// @return 持っていなければkInvalidColumn
	[[nodiscard]] inline nox::uint32 FindColumn(const nox::uint16 component_id)const noexcept
	{
		const auto it = std::ranges::lower_bound(component_id_list_, component_id);
		if (it == component_id_list_.end() || *it != component_id)
		{
			return kInvalidColumn;
		}
		return static_cast<nox::uint32>(std::ranges::distance(component_id_list_.begin(), it));
	}

	[[nodiscard]] inline bool Contains(const nox::uint16 component_id)const noexcept
	{
		return std::ranges::binary_search(component_id_list_, component_id);
	}

	[[nodiscard]] inline nox::uint32 GetColumnSize(const nox::uint32 column)const noexcept
	{
		return columns_[column].size;
	}

	[[nodiscard]] inline std::byte* GetComponentPtr(const nox::uint32 column, const nox::uint32 row)noexcept
	{
		NOX_ASSERT(row < entity_count_, u8"行が範囲外です");
		const Column& info = columns_[column];
		return chunks_[row / chunk_capacity_] + info.offset + static_cast<std::size_t>(row % chunk_capacity_) * info.size;
	}

	[[nodiscard]] inline nox::Entity& GetEntity(const nox::uint32 row)noexcept
	{
		NOX_ASSERT(row < entity_count_, u8"行が範囲外です");
		return reinterpret_cast<nox::Entity*>(chunks_[row / chunk_capacity_])[row % chunk_capacity_];
	}

	/// @brief 末尾に行を足す。Componentは未初期化
	/// @return 足した行の番号
	[[nodiscard]] nox::uint32 PushRow(const nox::Entity entity)noexcept
	{
		//	削除は末尾との入れ替えで詰めるので、空きは常に末尾のChunkにしかない
		if (entity_count_ == static_cast<nox::uint32>(chunks_.size()) * chunk_capacity_)
		{
			chunks_.emplace_back(static_cast<std::byte*>(nox::memory::Allocate(kChunkByteSize, kChunkAlignment, nox::memory::InstanceType::Other)));
		}

		const nox::uint32 row = entity_count_++;
		std::construct_at(&GetEntity(row), entity);
		return row;
	}

	/// @brief 行を外す。末尾の行を穴へ移して詰める(確保も全体のずらしもしない)
	/// @return 穴へ移ってきたEntity。外した行が末尾だった場合はstd::nullopt
	[[nodiscard]] std::optional<nox::Entity> SwapRemoveRow(const nox::uint32 row)noexcept
	{
		NOX_ASSERT(row < entity_count_, u8"行が範囲外です");
		const nox::uint32 last = entity_count_ - 1u;

		std::optional<nox::Entity> moved = std::nullopt;
		if (row != last)
		{
			for (nox::uint32 column = 0u; column < static_cast<nox::uint32>(columns_.size()); ++column)
			{
				std::memcpy(GetComponentPtr(column, row), GetComponentPtr(column, last), columns_[column].size);
			}
			moved = GetEntity(last);
			GetEntity(row) = *moved;
		}

		//	空いたChunkは解放せずに残す。増減を繰り返すときに確保し直さないため
		--entity_count_;
		return moved;
	}

	/// @brief component_idを足す(引く)と移る先のArchetypeを探す
	/// @details 向きはContains(component_id)で決まるので、追加と削除を区別しない
	/// @return 見つからなければkInvalidArchetypeId
	[[nodiscard]] inline nox::uint32 FindEdge(const nox::uint16 component_id)const noexcept
	{
		const auto it = std::ranges::find(edge_component_ids_, component_id);
		if (it == edge_component_ids_.end())
		{
			return kInvalidArchetypeId;
		}
		return edge_archetype_ids_[static_cast<std::size_t>(std::ranges::distance(edge_component_ids_.begin(), it))];
	}

	inline void AddEdge(const nox::uint16 component_id, const nox::uint32 archetype_id)noexcept
	{
		NOX_ASSERT(FindEdge(component_id) == kInvalidArchetypeId, u8"同じComponentのエッジを二重に登録しようとしました");
		edge_component_ids_.emplace_back(component_id);
		edge_archetype_ids_.emplace_back(archetype_id);
	}

private:
	/// @brief 昇順に並んだComponent ID
	const nox::Vector<nox::uint16> component_id_list_;
	/// @brief component_id_list_と同じ添字の列の配置
	nox::Vector<Column> columns_;

	nox::Vector<std::byte*> chunks_;
	/// @brief 1Chunkに入る行数
	nox::uint32 chunk_capacity_;
	nox::uint32 entity_count_;

	/// @brief エッジの検索キー。このComponentを足す(引く)と移る
	nox::Vector<nox::uint16> edge_component_ids_;
	/// @brief エッジの行き先のArchetype ID。edge_component_ids_と同じ添字
	nox::Vector<nox::uint32> edge_archetype_ids_;
};

nox::World::World():
	empty_archetype_buffer_{ std::byte{} }
{
	static_assert(sizeof(nox::World::Archetype) <= sizeof(decltype(empty_archetype_buffer_)), "空のArchetypeのバッファが小さすぎます");
	static_assert(alignof(nox::World::Archetype) <= 16u, "空のArchetypeのバッファのアラインメントが足りません");

	nox::World::Archetype* const empty_archetype = std::construct_at(reinterpret_cast<nox::World::Archetype*>(empty_archetype_buffer_.data()), std::span<const nox::uint16>{});
	archetypes_.emplace_back(empty_archetype);
}

nox::World::~World()
{
	for (nox::World::Archetype* const archetype : archetypes_ | std::views::drop(1))
	{
		delete archetype;
	}
	std::destroy_at(reinterpret_cast<nox::World::Archetype*>(empty_archetype_buffer_.data()));
}

void nox::World::Init()
{
	nox::StackAllocVector<const nox::NodeDescriptor*, 512> node_descriptors;
	nox::WorldRegistery registery(node_descriptors.GetContainer());

	//	TODO:全moduleを収集してるので、今後直す
	nox::reflection::ForeachDerivedClassInfoList(
		nox::reflection::Typeof<nox::EngineModule>(),
		[&node_descriptors ,&registery](const nox::reflection::ClassInfo& class_info)
		{
			//alignas(nox::EngineModule) std::array<std::byte, 256> buffer{};

			auto* const module = static_cast<nox::EngineModule*>(class_info.GetType().CreateObject());
			NOX_ASSERT(module != nullptr, u8"EngineModuleの生成に失敗しました: {0}", class_info.GetFullName());

			module->Register(registery);
			//	updatergraphへ追加

			delete module;

			node_descriptors.GetContainer().clear();
		});
}

void nox::World::Update()
{

}

nox::Entity nox::World::CreateEntity()
{
	nox::uint32 entity_index = 0u;
	if (free_entities_.empty())
	{
		entity_index = static_cast<nox::uint32>(entity_records_.size());
		entity_records_.emplace_back(nox::World::EntityRecord{});
	}
	else
	{
		entity_index = free_entities_.back();
		free_entities_.pop_back();
	}

	nox::World::EntityRecord& record = entity_records_[entity_index];
	const nox::Entity entity{ .generation = record.generation, .index = entity_index };

	//	Componentを持たないEntityも空のArchetypeに置く。AddComponentを常に「Archetypeの移動」として扱えるようにするため
	record.archetype_id = kEmptyArchetypeId;
	record.row = archetypes_[kEmptyArchetypeId]->PushRow(entity);
	return entity;
}

void nox::World::Delete(const nox::Entity entity)
{
	if (IsAlive(entity) == false)
	{
		return;
	}

	nox::World::EntityRecord& record = entity_records_[entity.index];
	if (const std::optional<nox::Entity> moved = archetypes_[record.archetype_id]->SwapRemoveRow(record.row))
	{
		entity_records_[moved->index].row = record.row;
	}

	record.archetype_id = kInvalidArchetypeId;
	record.row = 0u;
	++record.generation;
	free_entities_.emplace_back(entity.index);
}

bool nox::World::IsAlive(const nox::Entity entity)const noexcept
{
	NOX_ASSERT(entity.index < entity_records_.size(), u8"entityが範囲外です");
	if (entity.index >= entity_records_.size())
	{
		return false;
	}
	return entity_records_[entity.index].generation == entity.generation;
}

void* nox::World::AddComponent(const nox::Entity entity, const nox::reflection::Type& type)
{
	if (IsAlive(entity) == false)
	{
		return nullptr;
	}

	const nox::uint16 component_id = nox::detail::GetComponentId(type);
	const nox::uint32 src_id = entity_records_[entity.index].archetype_id;

	//	既に持っていれば移動しない
	if (nox::World::Archetype& src = *archetypes_[src_id]; src.Contains(component_id))
	{
		return src.GetComponentPtr(src.FindColumn(component_id), entity_records_[entity.index].row);
	}

	const nox::uint32 dst_id = GetOrCreateNextArchetype(src_id, component_id);
	const nox::uint32 dst_row = MoveEntity(entity, dst_id);

	//	移り先にしかないComponentは未初期化なので、ここで作る
	nox::World::Archetype& dst = *archetypes_[dst_id];
	std::byte* const storage = dst.GetComponentPtr(dst.FindColumn(component_id), dst_row);
	return type.CreateObject(std::span<std::uint8_t>(reinterpret_cast<std::uint8_t*>(storage), type.GetTypeSize()));
}

void nox::World::RemoveComponent(const nox::Entity entity, const nox::reflection::Type& type)
{
	if (IsAlive(entity) == false)
	{
		return;
	}

	const nox::uint16 component_id = nox::detail::GetComponentId(type);
	const nox::uint32 src_id = entity_records_[entity.index].archetype_id;
	if (archetypes_[src_id]->Contains(component_id) == false)
	{
		return;
	}

	//	外すComponentは移り先に列が無いので、移さずに捨てる(trivially destructibleなので破棄処理は要らない)
	const nox::uint32 dst_id = GetOrCreateNextArchetype(src_id, component_id);
	MoveEntity(entity, dst_id);
}

void* nox::World::GetComponent(const nox::Entity entity, const nox::reflection::Type& type)
{
	if (IsAlive(entity) == false)
	{
		return nullptr;
	}

	const nox::World::EntityRecord& record = entity_records_[entity.index];
	nox::World::Archetype& archetype = *archetypes_[record.archetype_id];
	const nox::uint32 column = archetype.FindColumn(nox::detail::GetComponentId(type));
	if (column == nox::World::Archetype::kInvalidColumn)
	{
		return nullptr;
	}
	return archetype.GetComponentPtr(column, record.row);
}

nox::uint32 nox::World::GetOrCreateNextArchetype(const nox::uint32 src_id, const nox::uint16 component_id)
{
	//	2回目以降はここで済む
	if (const nox::uint32 dst_id = archetypes_[src_id]->FindEdge(component_id); dst_id != kInvalidArchetypeId)
	{
		return dst_id;
	}

	//	初回だけ: 移り先のComponentの一覧を作る(昇順を保つ)
	const nox::World::Archetype& src = *archetypes_[src_id];
	const std::span<const nox::uint16> src_ids = src.GetComponentIdList();

	std::array<nox::uint16, kArchetypeComponentMaxCount> buffer{};
	auto out = buffer.begin();
	if (src.Contains(component_id))
	{
		out = std::ranges::remove_copy(src_ids, out, component_id).out;
	}
	else
	{
		NOX_ASSERT(src_ids.size() < kArchetypeComponentMaxCount, u8"1Archetypeが持てるComponentの数を超えました");
		const auto insert_position = std::ranges::lower_bound(src_ids, component_id);
		out = std::ranges::copy(src_ids.begin(), insert_position, out).out;
		*out++ = component_id;
		out = std::ranges::copy(insert_position, src_ids.end(), out).out;
	}

	const nox::uint32 dst_id = FindOrCreateArchetype(std::span<const nox::uint16>(buffer.begin(), out));

	//	エッジは両向きに張る(src + C → dst なら dst − C → src)
	archetypes_[src_id]->AddEdge(component_id, dst_id);
	archetypes_[dst_id]->AddEdge(component_id, src_id);
	return dst_id;
}

nox::uint32 nox::World::FindOrCreateArchetype(const std::span<const nox::uint16> component_id_list)
{
	//	エッジが無いときだけ通る経路なので線形探索にしている。種類が増えて重くなったら構成のハッシュで引く
	for (const auto [archetype_id, archetype] : std::views::enumerate(archetypes_))
	{
		if (archetype != nullptr && std::ranges::equal(archetype->GetComponentIdList(), component_id_list))
		{
			return static_cast<nox::uint32>(archetype_id);
		}
	}

	archetypes_.emplace_back(new nox::World::Archetype(component_id_list));
	return static_cast<nox::uint32>(archetypes_.size() - 1u);
}

nox::uint32 nox::World::MoveEntity(const nox::Entity entity, const nox::uint32 dst_id)noexcept
{
	nox::World::EntityRecord& record = entity_records_[entity.index];
	NOX_ASSERT(record.archetype_id != dst_id, u8"同じArchetypeへは移せません");

	nox::World::Archetype& src = *archetypes_[record.archetype_id];
	nox::World::Archetype& dst = *archetypes_[dst_id];
	const nox::uint32 src_row = record.row;
	const nox::uint32 dst_row = dst.PushRow(entity);

	//	両方が持つComponentだけを移す。どちらのIDも昇順なので、1回の突き合わせで済む
	const std::span<const nox::uint16> src_ids = src.GetComponentIdList();
	const std::span<const nox::uint16> dst_ids = dst.GetComponentIdList();
	for (std::size_t src_column = 0u, dst_column = 0u; src_column < src_ids.size() && dst_column < dst_ids.size();)
	{
		if (src_ids[src_column] == dst_ids[dst_column])
		{
			const nox::uint32 s = static_cast<nox::uint32>(src_column);
			const nox::uint32 d = static_cast<nox::uint32>(dst_column);
			std::memcpy(dst.GetComponentPtr(d, dst_row), src.GetComponentPtr(s, src_row), src.GetColumnSize(s));
			++src_column;
			++dst_column;
		}
		else if (src_ids[src_column] < dst_ids[dst_column])
		{
			++src_column;
		}
		else
		{
			++dst_column;
		}
	}

	//	移り元から外す。末尾から穴へ移ってきたEntityの行番号も直す
	if (const std::optional<nox::Entity> moved = src.SwapRemoveRow(src_row))
	{
		entity_records_[moved->index].row = src_row;
	}

	record.archetype_id = dst_id;
	record.row = dst_row;
	return dst_row;
}
