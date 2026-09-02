// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	entity_commands.h
/// @brief	フェーズ実行中のSystem / EntityLogicがWorldへ出せる操作の窓口。
/// @details EntitySystem::OnUpdate や EntityLogic の更新メソッドの引数に
///          nox::EntityCommands& を並べると、そのフェーズ用の実体が束縛される。
///          Worldへの非所有の薄いビューなので、コピーも保存もせずその場で使い切る。
#pragma once
#include	"entity.h"

namespace nox
{
	class World;

	/// @brief フェーズ実行中に許される操作だけを露出したWorldのビュー。
	/// @details CreateEntity / AddComponent は意図的に持たない。フェーズ実行中の構造変更は
	///          Archetypeの再配置を引き起こし、列挙中のポインタと行番号を壊すため設計上禁止している。
	///          破棄はコマンドバッファに積まれ、フェーズ終端でまとめて反映される(Destroy)。
	///          そのため「構造を変える操作は必ず遅延キューを経由する」ことが型の形で保証される。
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

		/// @brief entityの破棄を予約する。実際の破棄はフェーズ終端でまとめて行われる。
		void Destroy(nox::EntityId entity)noexcept;

		[[nodiscard]] bool IsAlive(nox::EntityId entity)const noexcept;

	private:
		nox::World* world_;
	};
}
