//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	world.cpp
///	@brief	world
#include	"pch.h"
#include	"world.h"
#include	<bit>
#include	<bitset>

#include	"world_registery.h"
#include	"node_descriptor.h"
#include	"engine_module.h"

namespace nox
{
	namespace
	{
		/// @brief archetypeが持てる最大コンポーネント数
		inline constexpr nox::uint16 kArchetypeComponentMaxCount = 255;

		class ComponentTypeSet final
		{
			static constexpr nox::uint64 kGoldenRatio64 = 0x9e3779b97f4a7c15ull;
		public:
			static constexpr nox::uint8 Capacity = 255;
		public:
			inline constexpr ComponentTypeSet(std::span<const nox::uint16> component_id_list)noexcept:
				component_indices_([](std::span<const nox::uint16> component_id_list)constexpr noexcept->auto {
				std::array<nox::uint16, Capacity> result{};
				for (std::size_t i = 0; i < component_id_list.size(); ++i)
				{
					result[i] = component_id_list[i];
				}
				return result;
					}(component_id_list)),
				summary_count_(static_cast<nox::uint8>(component_id_list.size()))
			{

			}

			bool Insert([[maybe_unused]] const nox::uint16 component_id)noexcept
			{
				//	TODO: 未実装
				return false;
			}

			bool Insert([[maybe_unused]] const std::span<const nox::uint16> component_id_list)noexcept
			{
				//	TODO: 未実装
				return false;
			}

			bool Contains([[maybe_unused]] const nox::uint16 component_id)const noexcept
			{
				//	TODO: 未実装
				return false;
			}

		private:
			std::array<nox::uint16, Capacity> component_indices_;
			/// @brief	集合に入っている型bit boolmfilter
			std::bitset<64> summary_bits_;
			nox::uint8 summary_count_;
		};
	}
}

class nox::World::Archetype final
{
	/// @brief Archetype内の1列の情報
	struct Column final
	{
		/// @brief Chunk先頭からのオフセット
		nox::uint32 offset;
		/// @brief 1要素のバイト数
		nox::uint32 size;
	};

	/// @brief このArchetypeからの接続先情報
	struct Edge final
	{
		/// @brief 遷移先のArchetypeのID
		ArchetypeId archetype_id : nox::World::kArchetypeIdBitWidth;
		/// @brief 追加/削除されたcomponentのid
		nox::ComponentId component_id;
	};

	/// @brief 1chunkのサイズ
	inline static constexpr nox::uint32 kChunkByteSize = 16u * 1024u;
	/// @brief Chunkの先頭のアラインメント。Componentのアラインメントの上限(16)に合わせる
	inline static constexpr nox::uint32 kChunkAlignment = 16u;
	inline static constexpr nox::uint32 kInvalidColumn = std::numeric_limits<nox::uint32>::max();
public:
	Archetype()noexcept = delete;
	inline explicit Archetype(const std::span<const nox::ComponentId> component_id_list)noexcept:
		archetype_id_(kInvalidArchetypeId),
		component_id_list_(std::from_range, component_id_list),
		entity_count_(0u)
	{
		NOX_ASSERT(std::ranges::is_sorted(component_id_list), u8"Component IDは昇順で渡してください");
		NOX_ASSERT(component_id_list.size() <= kArchetypeComponentMaxCount, u8"1Archetypeが持てるComponentの数を超えました");

		//	1行あたりのバイト数と、列ごとのアラインメント調整で出る余白の上限
		nox::uint32 row_stride = static_cast<nox::uint32>(sizeof(nox::Entity));
		nox::uint32 padding_budget = 0u;
		for (const nox::ComponentId component_id : component_id_list)
		{
			const nox::reflection::Type& type = nox::detail::GetComponentType(component_id);
			row_stride += type.GetTypeSize();
			padding_budget += type.GetAlignmentOf();
		}

		NOX_ASSERT(row_stride + padding_budget <= kChunkByteSize, u8"Componentの合計サイズがChunkに収まりません");
		chunk_capacity_ = (kChunkByteSize - padding_budget) / row_stride;

		columns_.reserve(component_id_list.size());
		nox::uint32 offset = chunk_capacity_ * static_cast<nox::uint32>(sizeof(nox::Entity));
		for (const nox::ComponentId component_id : component_id_list)
		{
			const nox::reflection::Type& type = nox::detail::GetComponentType(component_id);
			offset = nox::memory::AlignUp(offset, type.GetAlignmentOf());
			columns_.emplace_back(Column{ .offset = offset, .size = type.GetTypeSize() });
			offset += chunk_capacity_ * type.GetTypeSize();
		}
		NOX_ASSERT(offset <= kChunkByteSize, u8"Chunkのレイアウト計算が破綻しています");
	}
	Archetype(const Archetype&)noexcept = delete;
	Archetype(Archetype&&)noexcept = delete;
	~Archetype()
	{
		for (std::byte* const chunk : chunks_)
		{
			delete chunk;
		}
	}

	inline constexpr std::span<const nox::ComponentId> GetComponentIdList()const noexcept
	{
		return component_id_list_;
	}

	inline constexpr bool ContainsComponent(const nox::ComponentId component_id)const noexcept
	{
		return std::ranges::binary_search(component_id_list_, component_id);
	}

	[[nodiscard]] inline constexpr nox::uint32 GetEntityCount()const noexcept
	{
		return entity_count_;
	}

	[[nodiscard]] inline constexpr nox::uint32 FindColumn(const nox::ComponentId component_id)const noexcept
	{
		const auto it = std::ranges::lower_bound(component_id_list_, component_id);
		if (it == component_id_list_.end() || *it != component_id)
		{
			return kInvalidColumn;
		}
		return static_cast<nox::uint32>(std::ranges::distance(component_id_list_.begin(), it));
	}

	[[nodiscard]] inline nox::uint32 GetColumnSize(const nox::uint32 column)const noexcept
	{
		return columns_[column].size;
	}

	template<class Self> requires std::same_as<std::remove_const_t<Self>, Archetype>
	[[nodiscard]] inline constexpr auto GetComponentPtr(this Self& self, const nox::uint32 column, const nox::uint32 row)noexcept
		-> std::conditional_t<std::is_const_v<Self>, const std::byte*, std::byte*>
	{
		
		NOX_ASSERT(row < self.entity_count_, u8"行が範囲外です");
		const Column& info = self.columns_[column];

		//	1chunkの先頭アドレス + chunk内のオフセット + row * 1要素のバイト数
		std::byte* const chunk = self.chunks_[row / self.chunk_capacity_];
		return chunk + info.offset + static_cast<std::size_t>(row % self.chunk_capacity_) * info.size;
	}

	template<class Self> requires std::same_as<std::remove_const_t<Self>, Archetype>
	[[nodiscard]] inline constexpr auto GetComponentPtr(this Self& self, nox::ComponentId component_id, const nox::uint32 row)noexcept
		-> std::conditional_t<std::is_const_v<Self>, const std::byte*, std::byte*>
	{
		const nox::uint32 column = self.FindColumn(component_id);
		if (column == kInvalidColumn)
		{
			return nullptr;
		}
		return self.GetComponentPtr(column, row);
	}

	template<class Self> requires std::same_as<std::remove_const_t<Self>, Archetype>
	[[nodiscard]] inline constexpr auto GetEntity(this Self& self, const nox::uint32 row)noexcept
		-> std::conditional_t<std::is_const_v<Self>, const nox::Entity&, nox::Entity&>
	{
		NOX_ASSERT(row < self.entity_count_, u8"行が範囲外です");
		return reinterpret_cast<nox::Entity*>(self.chunks_[row / self.chunk_capacity_])[row % self.chunk_capacity_];
	}

	inline ArchetypeId FindEdgeArchetypeId(const nox::ComponentId component_id)noexcept
	{
		for (nox::World::Archetype::Edge& edge : edges_)
		{
			if (edge.component_id == component_id)
			{
				return edge.archetype_id;
			}
		}
		return nox::World::kInvalidArchetypeId;
	}

	inline void AddEdge(const ArchetypeId archetype_id, const nox::ComponentId component_id)noexcept
	{
		NOX_ASSERT(FindEdgeArchetypeId(component_id) == nox::World::kInvalidArchetypeId, u8"同じComponentのエッジを二重に登録しようとしました");
		edges_.emplace_back(nox::World::Archetype::Edge{ .archetype_id = archetype_id, .component_id = component_id });
	}

	[[nodiscard]] inline nox::uint32 AddEntity(const nox::Entity entity)noexcept
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
	
private:
	ArchetypeId archetype_id_ : nox::World::kArchetypeIdBitWidth;
	const nox::Vector<nox::ComponentId> component_id_list_;
	nox::Vector<Column> columns_;

	/// @brief このArchetypeに接続しているArchetypeのIDリスト
	nox::Vector<typename nox::World::Archetype::Edge> edges_;

	nox::uint32 entity_count_;
	/// @brief 1chunkに入る行数
	nox::uint32 chunk_capacity_;

	/// @brief バッファ
	nox::Vector<std::byte*> chunks_;
};

struct nox::World::Detail final
{
	Detail()noexcept = delete;

	static inline nox::World::Archetype* AllocArchetype(const std::span<const nox::ComponentId> component_id_list)
	{
		return new nox::World::Archetype(component_id_list);
	}

	static inline nox::uint32 MoveEntity(nox::World& self, const nox::Entity entity, const ArchetypeId archetype_id)
	{
		nox::World::EntityRecord& record = self.entity_records_[entity.index];
		NOX_ASSERT(record.archetype_id != archetype_id, u8"同じArchetypeに移動しようとしました");

		nox::World::Archetype& src_archetype = *self.archetypes_[nox::util::ToUnderlying(record.archetype_id)];
		nox::World::Archetype& dst_archetype = *self.archetypes_[nox::util::ToUnderlying(archetype_id)];
		const nox::uint32 src_row = record.row;
		const nox::uint32 dst_row = static_cast<nox::uint32>(dst_archetype.GetComponentIdList().size());
	}
};

nox::World::World():
	empty_archetype_buffer_{ std::byte{} }
{
	static_assert(sizeof(nox::World::Archetype) <= sizeof(decltype(empty_archetype_buffer_)), "空のArchetypeのバッファが小さすぎます");

	nox::World::Archetype*const empty_archetype = std::construct_at(reinterpret_cast<nox::World::Archetype*>(empty_archetype_buffer_.data()), std::span<const nox::ComponentId>{});
	archetypes_.emplace_back(empty_archetype);
}

nox::World::~World()
{
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

void nox::World::Init(const std::span<const nox::reflection::Type* const> module_types)
{
	return;
}

void nox::World::Update()
{

}

nox::Entity nox::World::CreateEntity()
{
	if (free_entities_.empty())
	{
		const nox::uint32 entity_index = static_cast<nox::uint32>(entity_records_.size());
		entity_records_.emplace_back(nox::World::EntityRecord{ 
			.generation = 0u, 
			.archetype_id = nox::World::kEmptyArchetypeId,
			.row = 0u }
			);
		return nox::Entity{ .generation = 0u, .index = entity_index };
	}
	else
	{
		const nox::uint32 entity_index = free_entities_.back();
		free_entities_.pop_back();

		const nox::World::EntityRecord& record = entity_records_[entity_index];
		return nox::Entity{ .generation = record.generation, .index = entity_index };
	}
}

void nox::World::Delete(const nox::Entity entity)
{
	if (IsAlive(entity) == false)
	{
		return;
	}

	nox::World::EntityRecord& record = entity_records_.at(entity.index);
	++record.generation;
	free_entities_.emplace_back(entity.index);
}

bool nox::World::IsAlive(const nox::Entity entity)const noexcept
{
	NOX_ASSERT(entity.index < entity_records_.size(), u8"entityが範囲外です");
	return entity_records_.at(entity.index).generation == entity.generation;
}

void* nox::World::AddComponent(const nox::Entity entity, const nox::reflection::Type& type)
{
	if (IsAlive(entity) == false)
	{
		return nullptr;
	}

	nox::World::EntityRecord& record = entity_records_.at(entity.index);
	nox::World::Archetype& archetype = *archetypes_[nox::util::ToUnderlying(record.archetype_id)];
	const auto component_id = nox::detail::GetComponentId(type);
	nox::World::Archetype* new_archetype = nullptr;

	{
		auto new_archetype_id = archetype.FindEdgeArchetypeId(component_id);
		if (new_archetype_id == nox::World::kInvalidArchetypeId)
		{
			nox::Vector<nox::ComponentId> new_component_id_list;
			new_component_id_list.reserve(archetype.GetComponentIdList().size() + 1u);
			new_component_id_list.append_range(archetype.GetComponentIdList());
			new_component_id_list.emplace_back(component_id);

			new_archetype = nox::World::Detail::AllocArchetype(new_component_id_list);
			new_archetype_id = ArchetypeId{ static_cast<nox::uint32>(archetypes_.size()) };
			archetypes_.emplace_back(new_archetype);
			archetype.AddEdge(new_archetype_id, component_id);
		}
		else
		{
			new_archetype = archetypes_[nox::util::ToUnderlying(new_archetype_id)];
		}
	}
	


	return nullptr;
}

void* nox::World::GetComponent(const nox::Entity entity, const nox::reflection::Type& type)noexcept
{
	return const_cast<void*>(std::as_const(*this).GetComponent(entity, type));
}

const void* nox::World::GetComponent(const nox::Entity entity, const nox::reflection::Type& type)const noexcept
{
	if (IsAlive(entity) == false)
	{
		return nullptr;
	}
	const nox::World::EntityRecord& record = entity_records_[entity.index];
	const Archetype& archetype = *archetypes_[nox::util::ToUnderlying(record.archetype_id)];

	return archetype.GetComponentPtr(nox::detail::GetComponentId(type), record.row);
}