//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	world.h
///	@brief	world
#pragma once
#include	"entity.h"
#include	"component_id.h"
#include	"updater_graph.h"
#include	"service_container.h"

namespace nox
{
	/// @brief	Worldはプロセスに複数存在可能
	///			GameWorld, EditorWorld, MeshPreviewWorldなど。各worldは任意のmoduleを持つことができる。
	class World final
	{
	private:
		/// @brief	Archetypeの識別値
		///	@details	ArchetypeIdは31bitで表現する
		enum class ArchetypeId : nox::uint32 {};

		static constexpr nox::uint8 kArchetypeIdBitWidth = 31u;
		/// @brief 空のArchetypeのID。Componentが1つもないEntityが所属する。
		static constexpr ArchetypeId kEmptyArchetypeId = ArchetypeId{ 0u };
		/// @brief 無効なArchetypeのID。存在しないArchetypeを表す
		static constexpr ArchetypeId kInvalidArchetypeId = ArchetypeId{ std::numeric_limits<nox::uint32>::max() };
	
		struct Detail;

		/// @brief Componentの物理配置を表すArchetype
		class Archetype;

		struct EntityRecord final
		{
			/// @brief 世代番号(破棄/再利用で更新され、stale handleの検出に使う)
			nox::uint32 generation;
			/// @brief 所属するArchetypeのID
			ArchetypeId archetype_id : kArchetypeIdBitWidth;
			/// @brief Archetype内のcomponentsのrow番号
			nox::uint32 row;
		};
	public:
		World();
		~World();

		void Init();
		void Init(const std::span<const nox::reflection::Type*const> module_types);
		template<class... ModuleTypes>
		void Init()
		{
			constexpr std::array<const nox::reflection::Type*, sizeof...(ModuleTypes)> module_types{ nox::reflection::Typeof<ModuleTypes>()... };
			Init(module_types);
		}

		void Update();

		nox::Entity CreateEntity();
		void Delete(nox::Entity entity);
		bool IsAlive(nox::Entity entity)const noexcept;

		/// @brief componentを追加する。既に同じ型のcomponentが存在する場合は何もせずnullptrを返す。
		/// @param entity 
		/// @param type 
		/// @return 
		void* AddComponent(const nox::Entity entity, const nox::reflection::Type& type);
		template<class T>
		inline T* AddComponent(const nox::Entity entity)
		{
			return static_cast<T*>(AddComponent(entity, nox::reflection::Typeof<T>()));
		}

		template<class T>
		inline T* AddComponent(const nox::Entity entity, T&& component)
		{
			T*const ptr = AddComponent<T>(entity);
			if (ptr)
			{
				*ptr = std::forward<T>(component);
			}
			return ptr;
		}

		void* GetComponent(const nox::Entity entity, const nox::reflection::Type& type)noexcept;
		const void* GetComponent(const nox::Entity entity, const nox::reflection::Type& type)const noexcept;

		/// @brief runtimeで一度だけ呼ぶ初期化
		static void GlobalInitialize();
		/// @brief runtimeで一度だけ呼ぶ終了処理
		static void GlobalTerminate();
	private:

	private:
		nox::Vector<EntityRecord> entity_records_;
		nox::Vector<nox::uint32> free_entities_;

		nox::Vector<nox::World::Archetype*> archetypes_;
		nox::Vector<nox::uint32> free_archetype_indices_;

		/// @brief EmptyArchetypeのバッファ
		std::array<std::byte, 256> empty_archetype_buffer_;

		nox::UpdaterGraph updater_graph_;
		nox::ServiceContainer service_container_;
	};
}