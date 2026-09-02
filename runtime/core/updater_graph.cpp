// Copyright (C) 2026 NOX ENGINE All rights reserved.

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

		/// @brief ノードの表示名。EntitySystemは型名、EntityLogicは「型名::メソッド名」。
		void append_node_name(UpdaterGraphTextBuilder& builder, const nox::UpdaterNode& node)noexcept
		{
			if (node.kind == nox::UpdaterNodeKind::EntitySystem)
			{
				builder.Append(node.system->GetDescriptor().name);
				return;
			}

			builder.Append(node.storage->GetDescriptor().name);
			builder.Append(u8"::");
			builder.Append(node.method->name);
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

nox::uint32 nox::BuildUpdaterLayerIndices(
	const std::span<const nox::UpdaterNodeAccess> accesses,
	const std::span<nox::uint32> dest_layer_indices)noexcept
{
	NOX_ASSERT(dest_layer_indices.size() >= accesses.size(), u8"レイヤー番号の出力先が足りません");
	if (accesses.empty())
	{
		return 0u;
	}

	//	衝突辺は必ず「登録順の小さい方 → 大きい方」に張られるので、登録順がトポロジカル順そのものになる。
	//	よってBuildExecuteNodeListの最長経路レイヤリングは、前方への一度の走査に畳める。
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

		dest_layer_indices[index] = layer_index;
		layer_count = std::max(layer_count, layer_index + 1u);
	}

	return layer_count;
}

nox::UpdaterGraph::UpdaterGraph() :
	phase_nodes_{},
	phase_layer_offsets_{}
{
}

nox::UpdaterGraph::~UpdaterGraph() = default;

void nox::UpdaterGraph::Rebuild(
	const std::span<nox::EntitySystemBase* const> systems,
	const std::span<nox::EntityLogicStorage* const> storages)
{
	for (nox::uint8 phase_index = 0u; phase_index < nox::util::ToUnderlying(nox::SystemPhaseType::_Max); ++phase_index)
	{
		RebuildPhase(static_cast<nox::SystemPhaseType>(phase_index), systems, storages);
	}
}

void nox::UpdaterGraph::RebuildPhase(
	const nox::SystemPhaseType phase_type,
	const std::span<nox::EntitySystemBase* const> systems,
	const std::span<nox::EntityLogicStorage* const> storages)
{
	const nox::uint32 phase_index = nox::util::ToUnderlying(phase_type);
	nox::Vector<nox::UpdaterNode>& dest_nodes = phase_nodes_[phase_index];
	nox::Vector<nox::uint32>& dest_offsets = phase_layer_offsets_[phase_index];
	dest_nodes.clear();
	dest_offsets.clear();

	//	登録順 = systemsの並び → storagesの並び → メソッド表の並び。
	nox::Vector<nox::UpdaterNode> nodes;
	for (nox::EntitySystemBase* const system : systems)
	{
		const nox::EntitySystemTypeDescriptor& descriptor = system->GetDescriptor();
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
		node.order_index = static_cast<nox::uint32>(nodes.size());
		nodes.push_back(node);
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
			node.order_index = static_cast<nox::uint32>(nodes.size());
			nodes.push_back(node);
		}
	}

	if (nodes.empty())
	{
		return;
	}

	nox::Vector<nox::UpdaterNodeAccess> accesses;
	accesses.reserve(nodes.size());
	for (const nox::UpdaterNode& node : nodes)
	{
		accesses.push_back(node.access);
	}

	nox::Vector<nox::uint32> layer_indices(nodes.size(), 0u);
	const nox::uint32 layer_count = nox::BuildUpdaterLayerIndices(
		std::span<const nox::UpdaterNodeAccess>(accesses.data(), accesses.size()),
		std::span<nox::uint32>(layer_indices.data(), layer_indices.size()));

	//	(レイヤー, 登録順)で整列する。登録順の走査を外に出さないので安定。
	dest_nodes.reserve(nodes.size());
	dest_offsets.reserve(static_cast<size_t>(layer_count) + 1u);
	for (nox::uint32 layer_index = 0u; layer_index < layer_count; ++layer_index)
	{
		dest_offsets.push_back(static_cast<nox::uint32>(dest_nodes.size()));
		for (nox::uint32 index = 0u; index < nodes.size(); ++index)
		{
			if (layer_indices[index] != layer_index)
			{
				continue;
			}

			nox::UpdaterNode node = nodes[index];
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

		NOX_INFO_LINE(nox::log_id::CoreCommon, u8"UpdaterGraph Phase: {0} (nodes={1}, layers={2})",
			to_updater_phase_name(phase_type),
			static_cast<nox::uint32>(nodes.size()),
			GetLayerCount(phase_type));

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
	}
}
#endif // !NOX_MASTER
