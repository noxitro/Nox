// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	entity_commands.h
/// @brief	フェーズ実行中のSystem / EntityLogicがWorldへ出せる操作の窓口。
/// @details EntitySystem::OnUpdate や EntityLogic の更新メソッドの引数に
///          nox::EntityCommands& を並べると、そのフェーズ用の実体が束縛される。
///          Worldへの非所有の薄いビューなので、コピーも保存もせずその場で使い切る。
#pragma once
#include	"entity.h"
#include	"component_type.h"

namespace nox
{
	class World;

	namespace detail
	{
		//	entity_commands.h が world.h に依存しないための橋渡し。
		//	entity_access.h -> entity_commands.h -> world.h という循環を作らないために、
		//	宣言だけをここへ置き、定義は entity_commands.cpp に置く。

		/// @brief フェーズ実行中にEntityIdだけを即時に払い出す。
		[[nodiscard]] nox::EntityId CreateEntityDeferredOfWorld(nox::World& world)noexcept;
		void QueueDestroyEntityOfWorld(nox::World& world, nox::EntityId entity)noexcept;
		void QueueAddComponentOfWorld(
			nox::World& world,
			nox::EntityId entity,
			const nox::ComponentTypeInfo& type_info,
			const void* source)noexcept;
		void QueueRemoveComponentOfWorld(
			nox::World& world,
			nox::EntityId entity,
			const nox::ComponentTypeInfo& type_info)noexcept;
		[[nodiscard]] bool IsAliveOfWorld(const nox::World& world, nox::EntityId entity)noexcept;
	}

	/// @brief フェーズ実行中に許される操作だけを露出したWorldのビュー。
	/// @details 構造変更2系統のうち「遅延系」の入口。ここから出た構造変更(Destroy / Add / Remove)は
	///          必ずコマンドバッファへ積まれ、フェーズ終端のPlaybackポイントでまとめて反映される。
	///          Archetypeの再配置が列挙中に起きないことが、型の形で保証される
	///          (このビューには即時に反映する手段が存在しない)。
	///
	///          Createだけは例外的にその場でEntityIdを返す。EntityIdの払い出しは
	///          EntityRecord 1件を触るだけでArchetypeにも他entityの行にも触れないため、
	///          列挙中のポインタ・行番号を壊さないからである。これにより
	///          「弾を生成してそのまま初期化する」を同一フェーズ内で書ける
	///          (講演でいう「遅延生成と即時操作のズレ」の緩和)。
	///          ただし生成直後のentityはComponentDataをまだ持たない。Addで積んだ初期値が
	///          読めるようになるのは次のフェーズからである。
	///
	///          コンストラクタはpublicだが、これは守るための記述量に見合わないという判断であり、
	///          エンジン(nox::detail::EntityInvoker)以外が作る想定はない。
	class EntityCommands final
	{
	public:
		inline explicit EntityCommands(nox::World& world)noexcept :
			world_(&world)
		{
		}

		//	その場で使い切る前提の一時オブジェクト。保存させないためコピーを禁じる。
		EntityCommands(const EntityCommands&) = delete;
		EntityCommands& operator=(const EntityCommands&) = delete;

		/// @brief 新しいentityを生成し、そのIdをその場で返す。
		/// @details 返るIdは即座に有効(IsAliveがtrue)。ComponentDataはAddで積む。
		[[nodiscard]] inline nox::EntityId Create()noexcept
		{
			return nox::detail::CreateEntityDeferredOfWorld(*world_);
		}

		/// @brief entityの破棄を予約する。実際の破棄はフェーズ終端でまとめて行われる。
		void Destroy(nox::EntityId entity)noexcept;

		/// @brief ComponentDataの追加を、初期値つきで予約する。
		/// @details 初期値はコマンドバッファへ複製されるので、呼び出し側の一時オブジェクトで構わない。
		///          反映はフェーズ終端。同一フェーズ内では読み出せない。
		template<class T>
			requires(nox::IsComponentDataType<T>())
		inline void Add(const nox::EntityId entity, const T& value)noexcept
		{
			nox::detail::QueueAddComponentOfWorld(*world_, entity, nox::ComponentTypeOf<T>(), &value);
		}

		/// @brief ComponentDataの追加を、ゼロ初期化で予約する。
		template<class T>
			requires(nox::IsComponentDataType<T>())
		inline void Add(const nox::EntityId entity)noexcept
		{
			nox::detail::QueueAddComponentOfWorld(*world_, entity, nox::ComponentTypeOf<T>(), nullptr);
		}

		/// @brief ComponentDataの削除を予約する。反映はフェーズ終端。
		template<class T>
			requires(nox::IsComponentDataType<T>())
		inline void Remove(const nox::EntityId entity)noexcept
		{
			nox::detail::QueueRemoveComponentOfWorld(*world_, entity, nox::ComponentTypeOf<T>());
		}

		[[nodiscard]] bool IsAlive(nox::EntityId entity)const noexcept;

	private:
		nox::World* world_;
	};
}
