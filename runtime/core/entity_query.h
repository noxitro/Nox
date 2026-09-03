// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	entity_query.h
/// @brief	宣言したComponentDataを持つentityの列挙と、引数リストへのバインド。
/// @details ForEachの1行あたりのコストは「列先頭 + 行インデックス」の加算のみ。
///          仮想関数もstd::functionも介さないため、呼び出し全体がインライン展開される。
#pragma once
#include	"archetype.h"
#include	"entity_access.h"

namespace nox
{
	class World;

	/// @brief Queryがマッチした「Archetype内の1Chunk」への参照。
	/// @details Chunk同士は完全に独立したメモリなので、これがそのまま並列実行の分割単位になる。
	///          POD。所有もしないしコピーも自明なので、スタック上の固定長配列へそのまま並べられる。
	struct EntityChunkRef final
	{
		nox::Archetype* archetype;
		nox::uint32 chunk_index;
	};

	/// @brief 必須ComponentDataを満たすArchetypeの集合。
	/// @details 照合はArchetypeが増えたときにだけ走る。毎フレームのコストはマッチ済み配列の走査だけ。
	class EntityQuery final
	{
	public:
		inline EntityQuery()noexcept :
			required_mask_(),
			matched_archetypes_()
		{
		}

		inline void Reset(const nox::ComponentMask& required_mask)
		{
			required_mask_ = required_mask;
			matched_archetypes_.clear();
		}

		/// @brief Archetypeが新規作成されたときに一度だけ呼ばれる。
		inline void TryAddArchetype(nox::Archetype& archetype)
		{
			if (archetype.GetMask().Contains(required_mask_))
			{
				matched_archetypes_.push_back(&archetype);
			}
		}

		[[nodiscard]] inline const nox::ComponentMask& GetRequiredMask()const noexcept { return required_mask_; }

		[[nodiscard]] inline std::span<nox::Archetype* const> GetMatchedArchetypes()const noexcept
		{
			return std::span(matched_archetypes_.data(), matched_archetypes_.size());
		}

		/// @brief 走査対象になるChunk(entityが1行以上あるもの)の総数。
		/// @details 空Chunkは数えない。ジョブ1つ分の仕事が無いものを配っても往復コストが乗るだけのため。
		[[nodiscard]] inline nox::uint32 GetTotalChunkCount()const noexcept
		{
			nox::uint32 total = 0u;
			for (const nox::Archetype* const archetype : matched_archetypes_)
			{
				const nox::uint32 chunk_count = archetype->GetChunkCount();
				for (nox::uint32 chunk_index = 0u; chunk_index < chunk_count; ++chunk_index)
				{
					total += (archetype->GetChunkEntityCount(chunk_index) != 0u) ? 1u : 0u;
				}
			}
			return total;
		}

		/// @brief 空でないChunk参照を、GetTotalChunkCount順で[start, start + out.size())の範囲だけ書き出す。
		/// @details 呼び出し側が用意したspanへ書くだけで確保は一切走らない。
		///          出力容量より多くのChunkがある場合は、startをずらして複数回に分けて呼ぶ。
		/// @return 実際に書き出した数。
		[[nodiscard]] inline nox::uint32 FillChunkRefs(
			const nox::uint32 start,
			const std::span<nox::EntityChunkRef> out)const noexcept
		{
			nox::uint32 scanned = 0u;
			nox::uint32 written = 0u;
			for (nox::Archetype* const archetype : matched_archetypes_)
			{
				const nox::uint32 chunk_count = archetype->GetChunkCount();
				for (nox::uint32 chunk_index = 0u; chunk_index < chunk_count; ++chunk_index)
				{
					if (archetype->GetChunkEntityCount(chunk_index) == 0u)
					{
						continue;
					}
					if (scanned++ < start)
					{
						continue;
					}
					if (written >= out.size())
					{
						return written;
					}
					out[written++] = nox::EntityChunkRef{ .archetype = archetype, .chunk_index = chunk_index };
				}
			}
			return written;
		}

	private:
		nox::ComponentMask required_mask_;
		nox::Vector<nox::Archetype*> matched_archetypes_;
	};

	namespace detail
	{
		/// @brief 列の先頭アドレスと行から実引数を作る。
		template<class Parameter>
		[[nodiscard]] inline Parameter BindEntityArgument(
			void* const base,
			const nox::EntityId entity,
			const nox::uint32 row)noexcept
		{
			using Traits = nox::EntityParameterTraits<Parameter>;
			if constexpr (Traits::k_kind == nox::EntityParameterKind::Entity)
			{
				return entity;
			}
			else if constexpr (nox::detail::IsComponentParameterKind(Traits::k_kind))
			{
				return *(static_cast<typename Traits::RawType*>(base) + row);
			}
			else if constexpr (Traits::k_kind == nox::EntityParameterKind::Commands)
			{
				//	呼び出し1回につき1つ、スタック上に作った実体を全行で共有する。
				return *static_cast<nox::EntityCommands*>(base);
			}
			else if constexpr (std::is_reference_v<Parameter>)
			{
				//	Service&。解決に失敗した場合はここへ来る前に呼び出しが打ち切られている。
				return *static_cast<std::remove_reference_t<Parameter>*>(base);
			}
			else
			{
				//	Service*。未登録ならnullptrがそのまま渡る(呼び出された側で判定できる)。
				return static_cast<Parameter>(base);
			}
		}

		/// @brief Service引数を解決する。Chunkに依存しないのでForEachごとに1回だけ呼ぶ。
		/// @return 実行を続行してよいか。参照で受けるServiceが未登録の場合のみfalse。
		template<class Parameter>
		[[nodiscard]] inline bool ResolveEntityServiceBase(nox::World& world, void*& out_base)noexcept
		{
			using Traits = nox::EntityParameterTraits<Parameter>;
			if constexpr (nox::detail::IsServiceParameterKind(Traits::k_kind))
			{
				out_base = nox::detail::TryGetServiceOfWorld(world, nox::reflection::Typeof<typename Traits::RawType>());
				if constexpr (std::is_reference_v<Parameter>)
				{
					//	参照にnullは渡せない。未登録は宣言と実態の食い違いなので実行ごと打ち切る。
					NOX_ASSERT(out_base != nullptr, u8"参照で宣言されたServiceがWorldに登録されていません");
					return out_base != nullptr;
				}
				else
				{
					return true;
				}
			}
			else
			{
				out_base = nullptr;
				return true;
			}
		}

		/// @brief nox::EntityCommands引数へ、呼び出し単位の実体を束縛する。
		/// @details Chunkにも行にも依存しないため、Serviceと同じく列挙ごとに1回だけ書き込む。
		template<class Parameter>
		inline void BindEntityCommandsBase(nox::EntityCommands& commands, void*& out_base)noexcept
		{
			if constexpr (nox::EntityParameterTraits<Parameter>::k_kind == nox::EntityParameterKind::Commands)
			{
				out_base = &commands;
			}
		}

		/// @brief ComponentData引数の列先頭を解決する。宣言外の型はQueryが弾くのでnullptrにならない。
		template<class Parameter>
		[[nodiscard]] inline bool ResolveEntityColumnBase(
			nox::Archetype& archetype,
			const nox::uint32 chunk_index,
			void*& out_base)noexcept
		{
			using Traits = nox::EntityParameterTraits<Parameter>;
			if constexpr (nox::detail::IsComponentParameterKind(Traits::k_kind))
			{
				out_base = archetype.TryGetComponentArray(chunk_index, nox::ComponentTypeIndexOf<typename Traits::RawType>());
				return out_base != nullptr;
			}
			else
			{
				return true;
			}
		}

		/// @brief 引数リストに対する列挙の実装。
		template<class... Parameters>
		struct EntityInvoker final
		{
			static constexpr size_t k_parameter_count = sizeof...(Parameters);
			using BaseArray = std::array<void*, (k_parameter_count == 0u ? 1u : k_parameter_count)>;
			using ParameterTuple = std::tuple<Parameters...>;

			template<size_t Index>
			using ParameterAt = std::tuple_element_t<Index, ParameterTuple>;

			template<size_t... Indices>
			[[nodiscard]] static bool ResolveServices(nox::World& world, BaseArray& bases, std::index_sequence<Indices...>)noexcept
			{
				return (nox::detail::ResolveEntityServiceBase<ParameterAt<Indices>>(world, bases[Indices]) && ... && true);
			}

			template<size_t... Indices>
			static void BindCommands(nox::EntityCommands& commands, BaseArray& bases, std::index_sequence<Indices...>)noexcept
			{
				(nox::detail::BindEntityCommandsBase<ParameterAt<Indices>>(commands, bases[Indices]), ...);
			}

			template<size_t... Indices>
			[[nodiscard]] static bool ResolveColumns(
				nox::Archetype& archetype,
				const nox::uint32 chunk_index,
				BaseArray& bases,
				std::index_sequence<Indices...>)noexcept
			{
				return (nox::detail::ResolveEntityColumnBase<ParameterAt<Indices>>(archetype, chunk_index, bases[Indices]) && ... && true);
			}

			template<class Owner, class MethodPointerType, size_t... Indices>
			static void InvokeRow(
				Owner& owner,
				MethodPointerType method,
				const BaseArray& bases,
				const nox::EntityId entity,
				const nox::uint32 row,
				std::index_sequence<Indices...>)
			{
				(owner.*method)(nox::detail::BindEntityArgument<ParameterAt<Indices>>(bases[Indices], entity, row)...);
			}

			/// @brief Chunk1つ分の行ループ。Service/EntityCommandsは解決済みでbasesに載っている前提。
			/// @details 直列経路とChunk並列経路のどちらもここを通る。走らせるコードを1本に保つための分離。
			template<class Owner, class MethodPointerType>
			static void InvokeChunkRows(
				BaseArray& bases,
				nox::Archetype& archetype,
				const nox::uint32 chunk_index,
				Owner& owner,
				MethodPointerType method)
			{
				constexpr auto k_indices = std::make_index_sequence<k_parameter_count>{};
				const nox::uint32 entity_count = archetype.GetChunkEntityCount(chunk_index);
				if (entity_count == 0u)
				{
					return;
				}
				if (ResolveColumns(archetype, chunk_index, bases, k_indices) == false)
				{
					NOX_ASSERT(false, u8"Queryにマッチしたarchetypeで列の解決に失敗しました");
					return;
				}

				const nox::EntityId* const entities = archetype.GetEntityArray(chunk_index);
				for (nox::uint32 row = 0u; row < entity_count; ++row)
				{
					InvokeRow(owner, method, bases, entities[row], row, k_indices);
				}
			}

			/// @brief Chunkを1つだけ処理する。Chunk並列実行の1ジョブ分に相当する。
			/// @details Service解決とEntityCommandsの束縛はChunkに依存しないので、ジョブごとに独立して行う。
			///          EntityCommandsはWorldへのポインタ1つのビューで、破棄予約はロックフリーの
			///          コマンドバッファへ積まれるため、複数スレッドから同時に使っても安全。
			template<class Owner, class MethodPointerType>
			static void ForEachEntityInChunk(
				nox::World& world,
				nox::Archetype& archetype,
				const nox::uint32 chunk_index,
				Owner& owner,
				MethodPointerType method)
			{
				constexpr auto k_indices = std::make_index_sequence<k_parameter_count>{};
				BaseArray bases{};
				nox::EntityCommands commands(world);
				if (ResolveServices(world, bases, k_indices) == false)
				{
					return;
				}
				BindCommands(commands, bases, k_indices);
				InvokeChunkRows(bases, archetype, chunk_index, owner, method);
			}

			/// @brief Queryにマッチした全entityに対してmethodを呼ぶ。
			template<class Owner, class MethodPointerType>
			static void ForEachEntity(
				nox::World& world,
				const nox::EntityQuery& query,
				Owner& owner,
				MethodPointerType method)
			{
				constexpr auto k_indices = std::make_index_sequence<k_parameter_count>{};
				BaseArray bases{};
				//	Worldへの薄いビュー。ポインタ1つ分なので確保も解放も走らない。
				nox::EntityCommands commands(world);
				if (ResolveServices(world, bases, k_indices) == false)
				{
					return;
				}
				BindCommands(commands, bases, k_indices);

				for (nox::Archetype* const archetype : query.GetMatchedArchetypes())
				{
					const nox::uint32 chunk_count = archetype->GetChunkCount();
					for (nox::uint32 chunk_index = 0u; chunk_index < chunk_count; ++chunk_index)
					{
						InvokeChunkRows(bases, *archetype, chunk_index, owner, method);
					}
				}
			}

			/// @brief 1つのentityに対してのみmethodを呼ぶ(EntityLogic用)。
			template<class Owner, class MethodPointerType>
			static void InvokeSingle(
				nox::World& world,
				nox::Archetype& archetype,
				const nox::ArchetypeLocation location,
				const nox::EntityId entity,
				Owner& owner,
				MethodPointerType method)
			{
				constexpr auto k_indices = std::make_index_sequence<k_parameter_count>{};
				BaseArray bases{};
				nox::EntityCommands commands(world);
				if (ResolveServices(world, bases, k_indices) == false)
				{
					return;
				}
				BindCommands(commands, bases, k_indices);
				if (ResolveColumns(archetype, location.chunk_index, bases, k_indices) == false)
				{
					NOX_ASSERT(false, u8"EntityLogicが宣言したComponentDataの列を解決できませんでした");
					return;
				}
				InvokeRow(owner, method, bases, entity, location.row, k_indices);
			}
		};

		/// @brief EntitySignatureからEntityInvokerを取り出す。
		template<class Signature>
		struct EntityInvokerOfSignature;

		template<class... Parameters>
		struct EntityInvokerOfSignature<nox::EntitySignature<Parameters...>>
		{
			using Type = nox::detail::EntityInvoker<Parameters...>;
		};

		template<class Signature>
		using EntityInvokerOf = typename nox::detail::EntityInvokerOfSignature<Signature>::Type;
	}
}
