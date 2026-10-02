// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	updater_graph.cpp
/// @brief	updater_graph
#include "pch.h"
#include "updater_graph.h"

#include "log_id.h"

namespace nox
{
	namespace
	{
		/// @brief 2つのノードが同一Serviceを取り合っているか。
		/// @details 型情報のアドレスで同一性を見る。片方でも書き込みがあれば衝突。
		[[nodiscard]] bool conflicts_service_access(
			const std::span<const nox::ServiceAccess> a,
			const std::span<const nox::ServiceAccess> b)noexcept
		{
			for (const nox::ServiceAccess& left : a)
			{
				for (const nox::ServiceAccess& right : b)
				{
					if (left.type != right.type)
					{
						continue;
					}
					if (left.write || right.write)
					{
						return true;
					}
				}
			}
			return false;
		}

		/// @brief Serviceの実体を型で探す。フェーズ関数の引数は新しいService(記述子を持つもの)に限る。
		/// @details 旧Serviceは World::RegisterService でいつでも足せるため、ここでは解決の対象にしない
		///          (フェーズ関数の引数に取れないことは nox::detail::IsServicePhaseParameter が保証している)。
		[[nodiscard]] void* find_service_instance(
			const std::span<const nox::ServiceInstance> services,
			const nox::reflection::Type* const type)noexcept
		{
			for (const nox::ServiceInstance& service : services)
			{
				if ((service.descriptor != nullptr) && (service.type == type))
				{
					return service.instance;
				}
			}
			return nullptr;
		}

		/// @brief Serviceのフェーズを、ハンドルの型情報で探す。見つからなければノード数を返す。
		[[nodiscard]] nox::uint32 find_service_phase_node(
			const std::span<const nox::UpdaterNode> nodes,
			const nox::reflection::Type* const phase_key)noexcept
		{
			for (nox::uint32 index = 0u; index < nodes.size(); ++index)
			{
				const nox::UpdaterNode& node = nodes[index];
				if ((node.kind == nox::UpdaterNodeKind::ServicePhaseMethod) && (node.service_method->phase_key == phase_key))
				{
					return index;
				}
			}
			return static_cast<nox::uint32>(nodes.size());
		}

		/// @brief nox::SortUpdaterNodeOrder の作業領域に書く、ノードごとの状態。
		inline constexpr nox::uint8 kOrderUnvisited = 0u;
		inline constexpr nox::uint8 kOrderVisiting = 1u;
		inline constexpr nox::uint8 kOrderPlaced = 2u;

		/// @brief ノードを並べる。まだ並んでいない先行ノードがあれば、元の番号の小さい順に先に並べる。
		/// @details 再帰の深さは指定が連なる長さで決まる(ノード数ではない)。
		/// @return 循環に当たらなければ true。当たった指定は無視して並べ続ける。
		bool place_with_predecessors(
			const nox::uint32 node_index,
			const std::span<const nox::UpdaterNodeOrderEdge> order_edges,
			const std::span<nox::uint32> dest_order,
			const std::span<nox::uint8> state,
			nox::uint32& position)noexcept
		{
			if (state[node_index] == kOrderPlaced)
			{
				return true;
			}
			if (state[node_index] == kOrderVisiting)
			{
				//	自分の先行ノードを辿って自分へ戻ってきた。この指定は守れないので無視する。
				return false;
			}

			state[node_index] = kOrderVisiting;

			//	大半のノードには指定が無い。先行ノードが1つも無ければ候補の走査ごと省く。
			bool has_predecessor = false;
			for (const nox::UpdaterNodeOrderEdge& edge : order_edges)
			{
				if (edge.to == node_index)
				{
					has_predecessor = true;
					break;
				}
			}

			bool acyclic = true;
			const nox::uint32 node_count = static_cast<nox::uint32>(dest_order.size());
			for (nox::uint32 predecessor = 0u; has_predecessor && (predecessor < node_count); ++predecessor)
			{
				bool is_predecessor = false;
				for (const nox::UpdaterNodeOrderEdge& edge : order_edges)
				{
					if ((edge.from == predecessor) && (edge.to == node_index))
					{
						is_predecessor = true;
						break;
					}
				}
				if (is_predecessor == false)
				{
					continue;
				}

				if (place_with_predecessors(predecessor, order_edges, dest_order, state, position) == false)
				{
					acyclic = false;
				}
			}

			state[node_index] = kOrderPlaced;
			dest_order[position] = node_index;
			++position;
			return acyclic;
		}

		/// @brief ノードが遅延構造変更を出しうるか(= nox::EntityCommands& を宣言しているか)。
		/// @details Serviceのフェーズ関数は EntityCommands を受け取れないので、常に出さない。
		[[nodiscard]] bool emits_structural_change(const nox::UpdaterNode& node)noexcept
		{
			switch (node.kind)
			{
			case nox::UpdaterNodeKind::EntitySystem:
				return node.system->GetDescriptor().emits_structural_change;
			case nox::UpdaterNodeKind::EntityLogicMethod:
				return node.method->emits_structural_change;
			case nox::UpdaterNodeKind::ServicePhaseMethod:
			default:
				return false;
			}
		}

#if !NOX_MASTER
		[[nodiscard]]
		constexpr std::u8string_view to_updater_phase_name(const nox::SystemPhaseType phase_type) noexcept
		{
			switch (phase_type)
			{
			case nox::SystemPhaseType::Init: return u8"Init";
			case nox::SystemPhaseType::Start: return u8"Start";
			case nox::SystemPhaseType::Update: return u8"Update";
			case nox::SystemPhaseType::Terminate: return u8"Terminate";
			default: return u8"Unknown";
			}
		}

		/// @brief ログ1行分を組み立てる固定長バッファ。BuildRuntimeDependencyGraphTextと同じ流儀。
		struct UpdaterGraphTextBuilder
		{
			std::array<nox::char8, 512> buffer{};
			size_t length = 0;

			void Clear()noexcept
			{
				length = 0;
				buffer[0] = u8'\0';
			}

			void Append(const std::u8string_view value)noexcept
			{
				const size_t writable_length = std::min(value.length(), buffer.size() - length - 1);
				std::ranges::copy_n(value.data(), writable_length, buffer.data() + length);
				length += writable_length;
				buffer[length] = u8'\0';
			}

			void Append(const std::string_view value)noexcept
			{
				const size_t writable_length = std::min(value.length(), buffer.size() - length - 1);
				for (size_t i = 0; i < writable_length; ++i)
				{
					buffer[length + i] = static_cast<nox::char8>(value[i]);
				}
				length += writable_length;
				buffer[length] = u8'\0';
			}

			[[nodiscard]] std::u8string_view GetView()const noexcept
			{
				return std::u8string_view(buffer.data(), length);
			}
		};

		/// @brief ComponentMaskを型名の並びとして書き出す。
		void append_component_mask(UpdaterGraphTextBuilder& builder, const nox::ComponentMask& mask)noexcept
		{
			bool first = true;
			builder.Append(u8"[");
			mask.ForEachIndex([&builder, &first](const nox::ComponentTypeIndex type_index)noexcept
				{
					if (first == false)
					{
						builder.Append(u8",");
					}
					first = false;

					const nox::ComponentTypeInfo* const info = nox::detail::TryGetComponentTypeInfo(type_index);
					builder.Append(info != nullptr ? info->name : std::string_view("?"));
				});
			builder.Append(u8"]");
		}

		void append_service_accesses(
			UpdaterGraphTextBuilder& builder,
			const std::span<const nox::ServiceAccess> accesses)noexcept
		{
			builder.Append(u8"[");
			for (size_t i = 0; i < accesses.size(); ++i)
			{
				if (i != 0)
				{
					builder.Append(u8",");
				}
				builder.Append(accesses[i].write ? u8"w:" : u8"r:");
				builder.Append(accesses[i].type != nullptr ? accesses[i].type->GetTypeName() : std::string_view("?"));
			}
			builder.Append(u8"]");
		}

		/// @brief ノードの表示名。EntitySystemは型名、EntityLogic / Serviceは「型名::メソッド名」。
		void append_node_name(UpdaterGraphTextBuilder& builder, const nox::UpdaterNode& node)noexcept
		{
			switch (node.kind)
			{
			case nox::UpdaterNodeKind::EntitySystem:
				builder.Append(node.system->GetDescriptor().name);
				return;

			case nox::UpdaterNodeKind::EntityLogicMethod:
				builder.Append(node.storage->GetDescriptor().name);
				builder.Append(u8"::");
				builder.Append(node.method->name);
				return;

			case nox::UpdaterNodeKind::ServicePhaseMethod:
				builder.Append(node.service_method->service_name);
				builder.Append(u8"::");
				builder.Append(node.service_method->name);
				return;

			default:
				builder.Append(u8"?");
				return;
			}
		}

		/// @brief 衝突理由を1つだけ書き出す(先に見つかったもの)。
		void append_conflict_reason(
			UpdaterGraphTextBuilder& builder,
			const nox::UpdaterNodeAccess& a,
			const nox::UpdaterNodeAccess& b)noexcept
		{
			if ((a.group_index != nox::k_invalid_updater_group_index) && (a.group_index == b.group_index))
			{
				builder.Append(u8"logic-state");
				return;
			}

			bool found = false;
			a.write_mask.ForEachIndex([&builder, &b, &found](const nox::ComponentTypeIndex type_index)noexcept
				{
					if (found || (b.read_write_mask.Test(type_index) == false))
					{
						return;
					}
					found = true;
					const nox::ComponentTypeInfo* const info = nox::detail::TryGetComponentTypeInfo(type_index);
					builder.Append(u8"component:");
					builder.Append(info != nullptr ? info->name : std::string_view("?"));
				});
			if (found)
			{
				return;
			}

			b.write_mask.ForEachIndex([&builder, &a, &found](const nox::ComponentTypeIndex type_index)noexcept
				{
					if (found || (a.read_write_mask.Test(type_index) == false))
					{
						return;
					}
					found = true;
					const nox::ComponentTypeInfo* const info = nox::detail::TryGetComponentTypeInfo(type_index);
					builder.Append(u8"component:");
					builder.Append(info != nullptr ? info->name : std::string_view("?"));
				});
			if (found)
			{
				return;
			}

			for (const nox::ServiceAccess& left : a.service_accesses)
			{
				for (const nox::ServiceAccess& right : b.service_accesses)
				{
					if ((left.type == right.type) && (left.write || right.write))
					{
						builder.Append(u8"service:");
						builder.Append(left.type != nullptr ? left.type->GetTypeName() : std::string_view("?"));
						return;
					}
				}
			}

			builder.Append(u8"unknown");
		}
#endif // !NOX_MASTER
	}
}

bool nox::ConflictsUpdaterNodeAccess(
	const nox::UpdaterNodeAccess& a,
	const nox::UpdaterNodeAccess& b)noexcept
{
	//	同一EntityLogic型のメソッド同士はメンバ変数を共有するため、宣言が重ならなくても直列化する。
	if ((a.group_index != nox::k_invalid_updater_group_index) && (a.group_index == b.group_index))
	{
		return true;
	}

	//	read同士は衝突しない。片方の書き込みが相手の読み書きに触れたときだけ衝突する。
	if (a.write_mask.Intersects(b.read_write_mask) || b.write_mask.Intersects(a.read_write_mask))
	{
		return true;
	}

	return conflicts_service_access(a.service_accesses, b.service_accesses);
}

bool nox::SortUpdaterNodeOrder(
	const std::span<const nox::UpdaterNodeOrderEdge> order_edges,
	const std::span<nox::uint32> dest_order,
	const std::span<nox::uint8> scratch_state)noexcept
{
	const nox::uint32 node_count = static_cast<nox::uint32>(dest_order.size());
	NOX_ASSERT(scratch_state.size() >= node_count, u8"並べ替えの作業領域が足りません");
	if (scratch_state.size() < node_count)
	{
		for (nox::uint32 index = 0u; index < node_count; ++index)
		{
			dest_order[index] = index;
		}
		return false;
	}

	std::fill_n(scratch_state.data(), node_count, kOrderUnvisited);

	//	元の登録順に訪ね、先行ノードを前倒ししながら並べる。
	//	構築時に一度だけ走る処理なので、ノード数 × 指定の数の走査で足りる。確保は走らない。
	nox::uint32 position = 0u;
	bool acyclic = true;
	for (nox::uint32 node_index = 0u; node_index < node_count; ++node_index)
	{
		if (place_with_predecessors(node_index, order_edges, dest_order, scratch_state, position) == false)
		{
			acyclic = false;
		}
	}

	return acyclic;
}

nox::uint32 nox::BuildUpdaterLayerIndices(
	const std::span<const nox::UpdaterNodeAccess> accesses,
	const std::span<nox::uint32> dest_layer_indices)noexcept
{
	return nox::BuildUpdaterLayerIndices(accesses, std::span<const nox::UpdaterNodeOrderEdge>(), dest_layer_indices);
}

nox::uint32 nox::BuildUpdaterLayerIndices(
	const std::span<const nox::UpdaterNodeAccess> accesses,
	const std::span<const nox::UpdaterNodeOrderEdge> order_edges,
	const std::span<nox::uint32> dest_layer_indices)noexcept
{
	NOX_ASSERT(dest_layer_indices.size() >= accesses.size(), u8"レイヤー番号の出力先が足りません");
	if (accesses.empty())
	{
		return 0u;
	}

	//	衝突辺は必ず「登録順の小さい方 → 大きい方」に張られるので、登録順がトポロジカル順そのものになる。
	//	よってBuildExecuteNodeListの最長経路レイヤリングは、前方への一度の走査に畳める。
	//	明示的な順序の指定も、並べ替えた後なら同じく小さい方から大きい方へ向いている。
	nox::uint32 layer_count = 0u;
	for (nox::uint32 index = 0u; index < accesses.size(); ++index)
	{
		nox::uint32 layer_index = 0u;
		for (nox::uint32 earlier_index = 0u; earlier_index < index; ++earlier_index)
		{
			if (nox::ConflictsUpdaterNodeAccess(accesses[earlier_index], accesses[index]) == false)
			{
				continue;
			}
			layer_index = std::max(layer_index, dest_layer_indices[earlier_index] + 1u);
		}

		//	順序の指定は、衝突しないノード同士でも別のレイヤーへ分ける理由になる。
		for (const nox::UpdaterNodeOrderEdge& edge : order_edges)
		{
			if (edge.to != index)
			{
				continue;
			}

			NOX_ASSERT(edge.from < index, u8"順序の指定は並べ替えた後の番号で from < to にしてください");
			if (edge.from >= index)
			{
				continue;
			}
			layer_index = std::max(layer_index, dest_layer_indices[edge.from] + 1u);
		}

		dest_layer_indices[index] = layer_index;
		layer_count = std::max(layer_count, layer_index + 1u);
	}

	return layer_count;
}

nox::UpdaterGraph::UpdaterGraph() :
	phase_nodes_{},
	phase_layer_offsets_{},
	phase_command_buffer_counts_{},
	phase_service_arguments_{}
{
}

nox::UpdaterGraph::~UpdaterGraph() = default;

void nox::UpdaterGraph::Rebuild(
	const std::span<nox::legacy::EntitySystemBase* const> systems,
	const std::span<nox::EntityLogicStorage* const> storages,
	const std::span<const nox::ServiceInstance> services)
{
	for (nox::uint8 phase_index = 0u; phase_index < nox::util::ToUnderlying(nox::SystemPhaseType::_Max); ++phase_index)
	{
		RebuildPhase(static_cast<nox::SystemPhaseType>(phase_index), systems, storages, services);
	}
}

void nox::UpdaterGraph::RebuildPhase(
	const nox::SystemPhaseType phase_type,
	const std::span<nox::legacy::EntitySystemBase* const> systems,
	const std::span<nox::EntityLogicStorage* const> storages,
	const std::span<const nox::ServiceInstance> services)
{
	const nox::uint32 phase_index = nox::util::ToUnderlying(phase_type);
	nox::Vector<nox::UpdaterNode>& dest_nodes = phase_nodes_[phase_index];
	nox::Vector<nox::uint32>& dest_offsets = phase_layer_offsets_[phase_index];
	nox::Vector<void*>& dest_arguments = phase_service_arguments_[phase_index];
	dest_nodes.clear();
	dest_offsets.clear();
	dest_arguments.clear();
	phase_command_buffer_counts_[phase_index] = 0u;

	//	既定の登録順で集める。Init / Start / Update は「Service → System → EntityLogic」、
	//	Terminate だけは「System → EntityLogic → Service」。
	//	Service を先に回すのは、時間や入力の更新のような毎フレームの準備を System / EntityLogic が読むため。
	//	Terminate で後に回すのは、System / EntityLogic の終了処理がまだ Service を使えるようにするため。
	//	どちらも衝突したときにどちらが先かを決めるだけで、衝突しなければ同じレイヤーで並列に走る。
	nox::Vector<nox::UpdaterNode> nodes;
	//	Serviceのフェーズ関数のノードが、解決済みの引数(dest_arguments)のどこから始まるか。nodes と同じ並び。
	nox::Vector<nox::uint32> argument_offsets;

	const auto append_service_nodes = [&]()
		{
			for (const nox::ServiceInstance& service : services)
			{
				//	旧Service(RegisterServiceで登録したもの)はフェーズを持たない。
				if (service.descriptor == nullptr)
				{
					continue;
				}

				for (const nox::ServicePhaseMethodDescriptor& method : service.descriptor->get_phase_methods())
				{
					if (method.phase != phase_type)
					{
						continue;
					}

					//	引数の Service はここで一度だけ解決する。実行時には探さない。
					const nox::uint32 argument_offset = static_cast<nox::uint32>(dest_arguments.size());
					bool resolved = true;
					for (const nox::ServicePhaseParameter& parameter : method.get_parameters())
					{
						void* const argument = find_service_instance(services, parameter.type);
						if ((argument == nullptr) && parameter.required)
						{
							NOX_ASSERT(false, u8"Serviceのフェーズ関数が参照で受けるServiceがWorldにありません: {0}::{1}",
								method.service_name, method.name);
							resolved = false;
						}
						dest_arguments.push_back(argument);
					}

					if (resolved == false)
					{
						dest_arguments.resize(argument_offset);
						continue;
					}

					nox::UpdaterNode node{};
					//	ComponentDataには触れない。Serviceへの読み書き(自分自身を含む)だけが宣言になる。
					node.access.service_accesses = method.get_service_accesses();
					node.access.group_index = nox::k_invalid_updater_group_index;
					node.kind = nox::UpdaterNodeKind::ServicePhaseMethod;
					node.service_instance = service.instance;
					node.service_method = &method;
					nodes.push_back(node);
					argument_offsets.push_back(argument_offset);
				}
			}
		};

	const bool services_last = (phase_type == nox::SystemPhaseType::Terminate);
	if (services_last == false)
	{
		append_service_nodes();
	}

	for (nox::legacy::EntitySystemBase* const system : systems)
	{
		const nox::legacy::EntitySystemTypeDescriptor& descriptor = system->GetDescriptor();
		if (descriptor.phase != phase_type)
		{
			continue;
		}

		nox::UpdaterNode node{};
		node.access.read_write_mask = descriptor.make_read_write_mask();
		node.access.write_mask = descriptor.make_write_mask();
		node.access.service_accesses = descriptor.get_service_accesses();
		node.access.group_index = nox::k_invalid_updater_group_index;
		node.kind = nox::UpdaterNodeKind::EntitySystem;
		node.system = system;
		nodes.push_back(node);
		argument_offsets.push_back(0u);
	}

	for (nox::uint32 storage_index = 0u; storage_index < storages.size(); ++storage_index)
	{
		nox::EntityLogicStorage* const storage = storages[storage_index];
		for (const nox::EntityLogicMethodDescriptor& method : storage->GetDescriptor().get_methods())
		{
			if (method.phase != phase_type)
			{
				continue;
			}

			nox::UpdaterNode node{};
			node.access.read_write_mask = method.make_read_write_mask();
			node.access.write_mask = method.make_write_mask();
			node.access.service_accesses = method.get_service_accesses();
			//	同一EntityLogic型のメソッド同士を必ず衝突させるためのグループ。
			node.access.group_index = storage_index;
			node.kind = nox::UpdaterNodeKind::EntityLogicMethod;
			node.storage = storage;
			node.method = &method;
			nodes.push_back(node);
			argument_offsets.push_back(0u);
		}
	}

	if (services_last)
	{
		append_service_nodes();
	}

	if (nodes.empty())
	{
		return;
	}

	const nox::uint32 node_count = static_cast<nox::uint32>(nodes.size());
	const std::span<const nox::UpdaterNode> collected_nodes(nodes.data(), nodes.size());

	//	明示的な順序の指定(Serviceのフェーズの After / Before)を、既定の登録順の番号で集める。
	nox::Vector<nox::UpdaterNodeOrderEdge> order_edges;
	for (nox::uint32 index = 0u; index < node_count; ++index)
	{
		const nox::UpdaterNode& node = nodes[index];
		if (node.kind != nox::UpdaterNodeKind::ServicePhaseMethod)
		{
			continue;
		}

		for (const nox::reflection::Type* const phase_key : node.service_method->get_after_phases())
		{
			const nox::uint32 from = find_service_phase_node(collected_nodes, phase_key);
			if (from == node_count)
			{
				//	指定先の Service がこの World に無い。フェーズの食い違いはコンパイル時に弾いている。
				NOX_WARNING_LINE(nox::log_id::CoreCommon, u8"順序の指定先のフェーズがありません: {0}::{1} After {2}",
					node.service_method->service_name, node.service_method->name, phase_key->GetTypeName());
				continue;
			}
			order_edges.push_back(nox::UpdaterNodeOrderEdge{ .from = from, .to = index });
		}

		for (const nox::reflection::Type* const phase_key : node.service_method->get_before_phases())
		{
			const nox::uint32 to = find_service_phase_node(collected_nodes, phase_key);
			if (to == node_count)
			{
				NOX_WARNING_LINE(nox::log_id::CoreCommon, u8"順序の指定先のフェーズがありません: {0}::{1} Before {2}",
					node.service_method->service_name, node.service_method->name, phase_key->GetTypeName());
				continue;
			}
			order_edges.push_back(nox::UpdaterNodeOrderEdge{ .from = index, .to = to });
		}
	}

	//	指定を満たすよう登録順を並べ替える。指定が無ければ既定の登録順のまま。
	nox::Vector<nox::uint32> order(node_count, 0u);
	if (order_edges.empty())
	{
		for (nox::uint32 index = 0u; index < node_count; ++index)
		{
			order[index] = index;
		}
	}
	else
	{
		nox::Vector<nox::uint8> scratch_state(node_count, nox::uint8{ 0u });
		[[maybe_unused]] const bool sorted = nox::SortUpdaterNodeOrder(
			std::span<const nox::UpdaterNodeOrderEdge>(order_edges.data(), order_edges.size()),
			std::span<nox::uint32>(order.data(), order.size()),
			std::span<nox::uint8>(scratch_state.data(), scratch_state.size()));
		NOX_ASSERT(sorted, u8"Serviceのフェーズの順序の指定が循環しています。守れない指定はログに出ています");
	}

	nox::Vector<nox::uint32> position_of(node_count, 0u);
	for (nox::uint32 position = 0u; position < node_count; ++position)
	{
		position_of[order[position]] = position;
	}

	//	指定を並べ替えた後の番号へ写す。循環して守れなかった指定(後ろ向きになったもの)は捨てる。
	nox::Vector<nox::UpdaterNodeOrderEdge> ordered_edges;
	ordered_edges.reserve(order_edges.size());
	for (const nox::UpdaterNodeOrderEdge& edge : order_edges)
	{
		const nox::uint32 from = position_of[edge.from];
		const nox::uint32 to = position_of[edge.to];
		if (from < to)
		{
			ordered_edges.push_back(nox::UpdaterNodeOrderEdge{ .from = from, .to = to });
			continue;
		}

		NOX_ERROR_LINE(nox::log_id::CoreCommon, u8"順序の指定を守れません(循環): {0}::{1} -> {2}",
			nodes[edge.from].service_method->service_name,
			nodes[edge.from].service_method->name,
			nodes[edge.to].service_method->name);
	}

	//	並べ替えた順に、登録順の番号とコマンドバッファ番号を振る。
	//	コマンドバッファ番号は「遅延構造変更を出しうるノードだけ」に、この登録順で詰めて振る。
	//	昇順に振るので、番号順の再生とノード登録順の再生は同じ並びになる。
	//	引数の配列はここまでで伸び切っているので、ノードへ渡すポインタはもう動かない。
	nox::uint32 command_buffer_index = 0u;
	nox::Vector<nox::UpdaterNode> ordered_nodes;
	ordered_nodes.reserve(node_count);
	for (nox::uint32 position = 0u; position < node_count; ++position)
	{
		const nox::uint32 source_index = order[position];
		nox::UpdaterNode node = nodes[source_index];
		node.order_index = position;
		node.command_buffer_index = emits_structural_change(node)
			? command_buffer_index++
			: nox::k_invalid_updater_command_buffer_index;
		if (node.kind == nox::UpdaterNodeKind::ServicePhaseMethod)
		{
			node.service_arguments = dest_arguments.data() + argument_offsets[source_index];
		}
		ordered_nodes.push_back(node);
	}

	phase_command_buffer_counts_[phase_index] = command_buffer_index;

	nox::Vector<nox::UpdaterNodeAccess> accesses;
	accesses.reserve(node_count);
	for (const nox::UpdaterNode& node : ordered_nodes)
	{
		accesses.push_back(node.access);
	}

	nox::Vector<nox::uint32> layer_indices(node_count, 0u);
	const nox::uint32 layer_count = nox::BuildUpdaterLayerIndices(
		std::span<const nox::UpdaterNodeAccess>(accesses.data(), accesses.size()),
		std::span<const nox::UpdaterNodeOrderEdge>(ordered_edges.data(), ordered_edges.size()),
		std::span<nox::uint32>(layer_indices.data(), layer_indices.size()));

	//	(レイヤー, 登録順)で整列する。登録順の走査を外に出さないので安定。
	dest_nodes.reserve(node_count);
	dest_offsets.reserve(static_cast<size_t>(layer_count) + 1u);
	for (nox::uint32 layer_index = 0u; layer_index < layer_count; ++layer_index)
	{
		dest_offsets.push_back(static_cast<nox::uint32>(dest_nodes.size()));
		for (nox::uint32 index = 0u; index < node_count; ++index)
		{
			if (layer_indices[index] != layer_index)
			{
				continue;
			}

			nox::UpdaterNode node = ordered_nodes[index];
			node.layer_index = layer_index;
			dest_nodes.push_back(node);
		}
	}
	//	末尾番兵。GetLayerNodesが分岐なしで範囲を作れる。
	dest_offsets.push_back(static_cast<nox::uint32>(dest_nodes.size()));
}

std::span<const nox::UpdaterNode> nox::UpdaterGraph::GetNodes(const nox::SystemPhaseType phase_type)const noexcept
{
	const nox::Vector<nox::UpdaterNode>& nodes = phase_nodes_[nox::util::ToUnderlying(phase_type)];
	return std::span<const nox::UpdaterNode>(nodes.data(), nodes.size());
}

nox::uint32 nox::UpdaterGraph::GetLayerCount(const nox::SystemPhaseType phase_type)const noexcept
{
	const nox::Vector<nox::uint32>& offsets = phase_layer_offsets_[nox::util::ToUnderlying(phase_type)];
	return offsets.empty() ? 0u : static_cast<nox::uint32>(offsets.size() - 1u);
}

nox::uint32 nox::UpdaterGraph::GetCommandBufferCount(const nox::SystemPhaseType phase_type)const noexcept
{
	return phase_command_buffer_counts_[nox::util::ToUnderlying(phase_type)];
}

std::span<const nox::UpdaterNode> nox::UpdaterGraph::GetLayerNodes(
	const nox::SystemPhaseType phase_type,
	const nox::uint32 layer_index)const noexcept
{
	if (layer_index >= GetLayerCount(phase_type))
	{
		return std::span<const nox::UpdaterNode>();
	}

	const nox::Vector<nox::UpdaterNode>& nodes = phase_nodes_[nox::util::ToUnderlying(phase_type)];
	const nox::Vector<nox::uint32>& offsets = phase_layer_offsets_[nox::util::ToUnderlying(phase_type)];
	const nox::uint32 begin = offsets[layer_index];
	const nox::uint32 end = offsets[layer_index + 1u];
	return std::span<const nox::UpdaterNode>(nodes.data() + begin, static_cast<size_t>(end - begin));
}

#if !NOX_MASTER
void nox::UpdaterGraph::Trace()const
{
	UpdaterGraphTextBuilder builder;

	for (nox::uint8 phase_index = 0u; phase_index < nox::util::ToUnderlying(nox::SystemPhaseType::_Max); ++phase_index)
	{
		const nox::SystemPhaseType phase_type = static_cast<nox::SystemPhaseType>(phase_index);
		const std::span<const nox::UpdaterNode> nodes = GetNodes(phase_type);
		if (nodes.empty())
		{
			continue;
		}

		//	引数の数は既存のログ行と揃えてある。新しい引数個数で NOX_INFO_LINE を実体化すると、
		//	kernel/string_format.h 側の既存警告(-Wmissing-braces)がその実体化ぶんだけ増えるため。
		NOX_INFO_LINE(nox::log_id::CoreCommon, u8"UpdaterGraph Phase: {0} (nodes={1}, layers={2})",
			to_updater_phase_name(phase_type),
			static_cast<nox::uint32>(nodes.size()),
			GetLayerCount(phase_type));
		NOX_INFO_LINE(nox::log_id::CoreCommon, u8"  Phase {0}: コマンドバッファ={1}本 / ノード={2}",
			to_updater_phase_name(phase_type),
			GetCommandBufferCount(phase_type),
			static_cast<nox::uint32>(nodes.size()));

		for (const nox::UpdaterNode& node : nodes)
		{
			builder.Clear();
			append_node_name(builder, node);
			builder.Append(u8" rw=");
			append_component_mask(builder, node.access.read_write_mask);
			builder.Append(u8" w=");
			append_component_mask(builder, node.access.write_mask);
			builder.Append(u8" services=");
			append_service_accesses(builder, node.access.service_accesses);

			NOX_INFO_LINE(nox::log_id::CoreCommon, u8"  Layer {0}: n{1} {2}",
				node.layer_index,
				node.order_index,
				builder.GetView());
		}

		//	衝突辺(=直列化の理由)。登録順の小さい方から大きい方へ張られる。
		for (const nox::UpdaterNode& from : nodes)
		{
			for (const nox::UpdaterNode& to : nodes)
			{
				if (from.order_index >= to.order_index)
				{
					continue;
				}
				if (nox::ConflictsUpdaterNodeAccess(from.access, to.access) == false)
				{
					continue;
				}

				builder.Clear();
				append_conflict_reason(builder, from.access, to.access);
				NOX_INFO_LINE(nox::log_id::CoreCommon, u8"  EDGE n{0} -> n{1} ({2})",
					from.order_index,
					to.order_index,
					builder.GetView());
			}
		}

		//	明示的な順序の指定(Serviceのフェーズの After / Before)。衝突が無くても直列化の理由になる。
		const nox::uint32 node_count = static_cast<nox::uint32>(nodes.size());
		for (const nox::UpdaterNode& node : nodes)
		{
			if (node.kind != nox::UpdaterNodeKind::ServicePhaseMethod)
			{
				continue;
			}

			for (const nox::reflection::Type* const phase_key : node.service_method->get_after_phases())
			{
				const nox::uint32 from_index = find_service_phase_node(nodes, phase_key);
				if (from_index == node_count)
				{
					continue;
				}

				builder.Clear();
				builder.Append(u8"order");
				NOX_INFO_LINE(nox::log_id::CoreCommon, u8"  EDGE n{0} -> n{1} ({2})",
					nodes[from_index].order_index,
					node.order_index,
					builder.GetView());
			}

			for (const nox::reflection::Type* const phase_key : node.service_method->get_before_phases())
			{
				const nox::uint32 to_index = find_service_phase_node(nodes, phase_key);
				if (to_index == node_count)
				{
					continue;
				}

				builder.Clear();
				builder.Append(u8"order");
				NOX_INFO_LINE(nox::log_id::CoreCommon, u8"  EDGE n{0} -> n{1} ({2})",
					node.order_index,
					nodes[to_index].order_index,
					builder.GetView());
			}
		}
	}
}
#endif // !NOX_MASTER
