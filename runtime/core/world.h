//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	world.h
///	@brief	world
#pragma once
#include	"entity.h"
#include	"component_id.h"

namespace nox
{
	/// @brief	Worldはプロセスに複数存在可能
	///			GameWorld, EditorWorld, MeshPreviewWorldなど。各worldは任意のmoduleを持つことができる。
	class World final
	{
		static constexpr nox::uint32 kInvalidArchetypeId = std::numeric_limits<nox::uint32>::max();
	private:
		/// @brief Componentの物理配置を表すArchetype
		class Archetype;

		struct EntityRecord final
		{
			constexpr EntityRecord()noexcept :
				generation(0u),
				archetype_id(kInvalidArchetypeId),
				row(0u)
			{}

			nox::uint32 generation;
			/// @brief 所属するArchetype。破棄済みならkInvalidArchetypeId
			nox::uint32 archetype_id;
			/// @brief Archetype内の行番号(全Chunkを通した通し番号)
			nox::uint32 row;
		};

		struct Node
		{

		};
	public:
		World();
		~World();

		World(const World&) = delete;
		World& operator=(const World&) = delete;
		World(World&&) = delete;
		World& operator=(World&&) = delete;

		void Init();
		void Update();

		nox::Entity CreateEntity();
		void Delete(nox::Entity entity);
		bool IsAlive(nox::Entity entity)const noexcept;

		/// @brief Componentを追加し、そのComponentを返す。既に持っていれば既存のものを返す
		/// @return entityが破棄済みならnullptr
		void* AddComponent(const nox::Entity entity, const nox::reflection::Type& type);
		template<class T>
		inline T* AddComponent(const nox::Entity entity)
		{
			return static_cast<T*>(AddComponent(entity, nox::reflection::Typeof<T>()));
		}

		/// @brief Componentを外す。持っていなければ何もしない
		void RemoveComponent(const nox::Entity entity, const nox::reflection::Type& type);
		template<class T>
		inline void RemoveComponent(const nox::Entity entity)
		{
			RemoveComponent(entity, nox::reflection::Typeof<T>());
		}

		/// @return 持っていない、またはentityが破棄済みならnullptr
		void* GetComponent(const nox::Entity entity, const nox::reflection::Type& type);
		template<class T>
		inline T* GetComponent(const nox::Entity entity)
		{
			return static_cast<T*>(GetComponent(entity, nox::reflection::Typeof<T>()));
		}

		/// @brief runtimeで一度だけ呼ぶ初期化
		static void GlobalInitialize();
		/// @brief runtimeで一度だけ呼ぶ終了処理
		static void GlobalTerminate();
	private:
		/// @brief src_idのArchetypeにcomponent_idを足した(引いた)Archetypeを返す。無ければ作ってエッジを張る
		nox::uint32 GetOrCreateNextArchetype(const nox::uint32 src_id, const nox::uint16 component_id);
		/// @brief Componentの構成が一致するArchetypeを返す。無ければ作る
		/// @param component_id_list 昇順に並んだComponent ID
		nox::uint32 FindOrCreateArchetype(const std::span<const nox::uint16> component_id_list);
		/// @brief entityを別のArchetypeへ移す。両方が持つComponentだけを移し、移り先にしかないComponentは未初期化のまま
		/// @return 移り先の行番号
		nox::uint32 MoveEntity(const nox::Entity entity, const nox::uint32 dst_id)noexcept;

	private:
		nox::Vector<EntityRecord> entity_records_;
		nox::Vector<nox::uint32> free_entities_;

		/// @brief Archetype IDで引く。0番は空のArchetype
		nox::Vector<nox::World::Archetype*> archetypes_;
		/// @brief TODO: Archetypeの破棄は未実装。破棄するときはエッジも外すこと
		nox::Vector<nox::uint32> free_archetype_indices_;

		/// @brief 空のArchetypeのバッファ
		alignas(16) std::array<std::byte, 256> empty_archetype_buffer_;
	};
}
