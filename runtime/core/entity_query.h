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
				if (ResolveServices(world, bases, k_indices) == false)
				{
					return;
				}

				for (nox::Archetype* const archetype : query.GetMatchedArchetypes())
				{
					const nox::uint32 chunk_count = archetype->GetChunkCount();
					for (nox::uint32 chunk_index = 0u; chunk_index < chunk_count; ++chunk_index)
					{
						const nox::uint32 entity_count = archetype->GetChunkEntityCount(chunk_index);
						if (entity_count == 0u)
						{
							continue;
						}
						if (ResolveColumns(*archetype, chunk_index, bases, k_indices) == false)
						{
							NOX_ASSERT(false, u8"Queryにマッチしたarchetypeで列の解決に失敗しました");
							continue;
						}

						const nox::EntityId* const entities = archetype->GetEntityArray(chunk_index);
						for (nox::uint32 row = 0u; row < entity_count; ++row)
						{
							InvokeRow(owner, method, bases, entities[row], row, k_indices);
						}
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
				if (ResolveServices(world, bases, k_indices) == false)
				{
					return;
				}
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
