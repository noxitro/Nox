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

		/// @brief 引数リストの宣言(ComponentData / Service)だけで衝突するか。インスタンス状態の共有は見ない。
		[[nodiscard]] bool conflicts_declared_access(
			const nox::UpdaterNodeAccess& a,
			const nox::UpdaterNodeAccess& b)noexcept
		{
			//	read同士は衝突しない。片方の書き込みが相手の読み書きに触れたときだけ衝突する。
			if (a.write_mask.Intersects(b.read_write_mask) || b.write_mask.Intersects(a.read_write_mask))
			{
				return true;
			}

			return conflicts_service_access(a.service_accesses, b.service_accesses);
		}

		/// @brief ノードの型が宣言した RunAfter。EntityLogic / Serviceは型単位の宣言を全メソッドで共有する。
		/// @details Taskは型ではないので宣言を持たない(空)。
		[[nodiscard]] std::span<const std::string_view> get_updater_node_run_after(const nox::UpdaterNode& node)noexcept
		{
			switch (node.kind)
			{
			case nox::UpdaterNodeKind::EntitySystem:
				return node.system->GetDescriptor().run_after;
			case nox::UpdaterNodeKind::EntityLogicMethod:
				return node.storage->GetDescriptor().run_after;
			case nox::UpdaterNodeKind::ServiceMethod:
				return node.service_type->run_after;
			case nox::UpdaterNodeKind::Task:
				break;
			}
			return std::span<const std::string_view>();
		}

		/// @brief ノードの型が宣言した RunBefore。
		[[nodiscard]] std::span<const std::string_view> get_updater_node_run_before(const nox::UpdaterNode& node)noexcept
		{
			switch (node.kind)
			{
			case nox::UpdaterNodeKind::EntitySystem:
				return node.system->GetDescriptor().run_before;
			case nox::UpdaterNodeKind::EntityLogicMethod:
				return node.storage->GetDescriptor().run_before;
			case nox::UpdaterNodeKind::ServiceMethod:
				return node.service_type->run_before;
			case nox::UpdaterNodeKind::Task:
				break;
			}
			return std::span<const std::string_view>();
		}

		/// @brief フェーズの集合を表すビット。
		[[nodiscard]] constexpr nox::uint32 to_updater_phase_bit(const nox::SystemPhaseType phase_type)noexcept
		{
			return 1u << nox::util::ToUnderlying(phase_type);
		}

		/// @brief 構築中のノード1つ分の作業領域。構築が終われば捨てる。
		struct UpdaterBuildNode final
		{
			nox::UpdaterNode node;
			/// @brief 全順序のキー(型名, メソッド名)。
			std::string_view type_name;
			std::string_view method_name;
			/// @brief 登録順。型名もメソッド名も同じノード(同じ型の二重登録)の間でだけ使う。
			nox::uint32 registration_index = 0u;
			bool emits_structural_change = false;
		};

		/// @brief 全順序のタイブレーク: 型名 → メソッド名 → (同名の二重登録のときだけ)登録順。
		[[nodiscard]] bool is_less_updater_build_node(const UpdaterBuildNode& a, const UpdaterBuildNode& b)noexcept
		{
			const int type_compare = a.type_name.compare(b.type_name);
			if (type_compare != 0)
			{
				return type_compare < 0;
			}

			const int method_compare = a.method_name.compare(b.method_name);
			if (method_compare != 0)
			{
				return method_compare < 0;
			}

			return a.registration_index < b.registration_index;
		}

		/// @brief ノード番号の半開区間 [first, last)。
		struct UpdaterNodeRange final
		{
			nox::uint32 first = 0u;
			nox::uint32 last = 0u;
		};

		/// @brief 型名順に整列済みのノード列から、指定した型のノードの範囲を引く。無ければ空。
		/// @details 同じ型のノード(EntityLogicの各メソッド)は整列後に必ず連続している。
		[[nodiscard]] UpdaterNodeRange find_updater_type_range(
			const std::span<const UpdaterBuildNode> nodes,
			const std::string_view type_name)noexcept
		{
			const auto first = std::ranges::lower_bound(nodes, type_name, std::less<>{}, &UpdaterBuildNode::type_name);
			const auto last = std::ranges::upper_bound(first, nodes.end(), type_name, std::less<>{}, &UpdaterBuildNode::type_name);
			return UpdaterNodeRange{
				.first = static_cast<nox::uint32>(first - nodes.begin()),
				.last = static_cast<nox::uint32>(last - nodes.begin()),
			};
		}

		/// @brief 明示辺を宣言しうる型1つ分の情報。フェーズをまたいだ名前解決に使う。
		struct UpdaterOrderDeclarer final
		{
			std::string_view name;
			std::span<const std::string_view> run_after;
			std::span<const std::string_view> run_before;
			/// @brief この型がノードを持つフェーズの集合(to_updater_phase_bit の和)。
			nox::uint32 phase_bits = 0u;
		};

		/// @brief RunAfter / RunBefore に並べた名前が、登録済みの型を指し、同じフェーズを共有しているか。
		/// @details フェーズごとの構築では「このフェーズに相手のノードが無い」と「そもそも誤記」を区別できない
		///          (EntityLogic / Serviceは複数フェーズにメソッドを持てるので、無いフェーズがあるのは正常)。
		///          そこで先に型の集合全体で検査し、辺が1本も張れない宣言をここで弾く。
		///          Taskは型ではないので、宣言する側にも並べられる側にもならない。
		[[nodiscard]] nox::UpdaterGraphBuildResult validate_updater_order_targets(
			const std::span<nox::EntitySystemBase* const> systems,
			const std::span<nox::EntityLogicStorage* const> storages,
			const std::span<const nox::UpdaterServiceBinding> services)
		{
			nox::Vector<UpdaterOrderDeclarer> declarers;
			declarers.reserve(systems.size() + storages.size() + services.size());
			for (const nox::EntitySystemBase* const system : systems)
			{
				const nox::EntitySystemTypeDescriptor& descriptor = system->GetDescriptor();
				declarers.push_back(UpdaterOrderDeclarer{
					.name = descriptor.name,
					.run_after = descriptor.run_after,
					.run_before = descriptor.run_before,
					.phase_bits = to_updater_phase_bit(descriptor.phase),
					});
			}
			for (const nox::EntityLogicStorage* const storage : storages)
			{
				const nox::EntityLogicTypeDescriptor& descriptor = storage->GetDescriptor();
				nox::uint32 phase_bits = 0u;
				for (const nox::EntityLogicMethodDescriptor& method : descriptor.get_methods())
				{
					phase_bits |= to_updater_phase_bit(method.phase);
				}
				declarers.push_back(UpdaterOrderDeclarer{
					.name = descriptor.name,
					.run_after = descriptor.run_after,
					.run_before = descriptor.run_before,
					.phase_bits = phase_bits,
					});
			}
			for (const nox::UpdaterServiceBinding& binding : services)
			{
				const nox::ServiceMethodTypeDescriptor& descriptor = *binding.descriptor;
				nox::uint32 phase_bits = 0u;
				for (const nox::ServiceMethodDescriptor& method : descriptor.get_methods())
				{
					phase_bits |= to_updater_phase_bit(method.phase);
				}
				declarers.push_back(UpdaterOrderDeclarer{
					.name = descriptor.name,
					.run_after = descriptor.run_after,
					.run_before = descriptor.run_before,
					.phase_bits = phase_bits,
					});
			}

			for (const UpdaterOrderDeclarer& declarer : declarers)
			{
				for (const std::span<const std::string_view> targets : { declarer.run_after, declarer.run_before })
				{
					for (const std::string_view target_name : targets)
					{
						const auto target = std::ranges::find(declarers, target_name, &UpdaterOrderDeclarer::name);
						if (target == declarers.end())
						{
							return nox::UpdaterGraphBuildResult{
								.error = nox::UpdaterGraphBuildError::UnresolvedOrderTarget,
								.declaring_type_name = declarer.name,
								.target_type_name = target_name,
							};
						}
						if ((declarer.phase_bits & target->phase_bits) == 0u)
						{
							return nox::UpdaterGraphBuildResult{
								.error = nox::UpdaterGraphBuildError::OrderTargetInOtherPhase,
								.declaring_type_name = declarer.name,
								.target_type_name = target_name,
							};
						}
					}
				}
			}

			return nox::UpdaterGraphBuildResult{};
		}

		/// @brief ワーカーへ配るノード1つ分のジョブコンテキスト。ディスパッチ毎にスタック上へ作る。
		/// @details ジョブは関数ポインタ + void* しか渡せないので、実行関数と呼び出し側の文脈とノードをここで束ねる。
		struct UpdaterLayerJobContext final
		{
			nox::UpdaterNodeExecuteFunction execute = nullptr;
			void* context = nullptr;
			const nox::UpdaterNode* node = nullptr;
		};

		/// @brief UpdaterLayerJobContext を実行するジョブ本体。
		void execute_updater_layer_job(void* const job_context)
		{
			const auto* const layer_job = static_cast<const UpdaterLayerJobContext*>(job_context);
			layer_job->execute(layer_job->context, *layer_job->node);
		}

		/// @brief 未配置の先行ノードを1つ返す。循環の報告にだけ使う。
		/// @details 未配置のノードは入次数が残っている = 未配置の先行ノードを必ず持つ。
		[[nodiscard]] nox::uint32 find_unplaced_predecessor(
			const std::span<const nox::UpdaterOrderEdge> edges,
			const std::span<const nox::uint32> order_positions,
			const nox::uint32 unplaced_position,
			const nox::uint32 node_index)noexcept
		{
			for (const nox::UpdaterOrderEdge& edge : edges)
			{
				if ((edge.to == node_index) && (order_positions[edge.from] == unplaced_position))
				{
					return edge.from;
				}
			}
			return node_index;
		}

#if !NOX_MASTER
		[[nodiscard]]
		constexpr std::u8string_view to_updater_phase_name(const nox::SystemPhaseType phase_type) noexcept
		{
			switch (phase_type)
			{
			case nox::SystemPhaseType::Init: return u8"Init";
			case nox::SystemPhaseType::Start: return u8"Start";
			case nox::SystemPhaseType::FrameIngress: return u8"FrameIngress";
			case nox::SystemPhaseType::Update: return u8"Update";
			case nox::SystemPhaseType::Presentation: return u8"Presentation";
			case nox::SystemPhaseType::Terminate: return u8"Terminate";
			default: return u8"Unknown";
			}
		}

		/// @brief ノードの種別の表示名。
		[[nodiscard]] constexpr std::u8string_view to_updater_node_kind_name(const nox::UpdaterNodeKind kind)noexcept
		{
			switch (kind)
			{
			case nox::UpdaterNodeKind::EntitySystem: return u8"system";
			case nox::UpdaterNodeKind::EntityLogicMethod: return u8"logic";
			case nox::UpdaterNodeKind::ServiceMethod: return u8"service";
			case nox::UpdaterNodeKind::Task: return u8"task";
			}
			return u8"unknown";
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

			void Append(const nox::uint32 value)noexcept
			{
				std::array<char, 16> temp{};
				const auto [ptr, ec] = std::to_chars(temp.data(), temp.data() + temp.size(), value);
				if (ec == std::errc{})
				{
					Append(std::string_view(temp.data(), static_cast<size_t>(ptr - temp.data())));
				}
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

		/// @brief 型名とメソッド名から表示名を書き出す。EntitySystemは型名、EntityLogicは「型名::メソッド名」。
		void append_type_and_method_name(
			UpdaterGraphTextBuilder& builder,
			const std::string_view type_name,
			const std::string_view method_name)noexcept
		{
			builder.Append(type_name);
			if (method_name.empty() == false)
			{
				builder.Append(u8"::");
				builder.Append(method_name);
			}
		}

		/// @brief ノードの表示名。EntitySystemは型名、EntityLogic / Serviceは「型名::メソッド名」、Taskは関数名。
		void append_node_name(UpdaterGraphTextBuilder& builder, const nox::UpdaterNode& node)noexcept
		{
			append_type_and_method_name(builder, nox::GetUpdaterNodeTypeName(node), nox::GetUpdaterNodeMethodName(node));
		}

		/// @brief 衝突理由を1つだけ書き出す(先に見つかったもの)。
		void append_conflict_reason(
			UpdaterGraphTextBuilder& builder,
			const nox::UpdaterNode& a_node,
			const nox::UpdaterNode& b_node)noexcept
		{
			const nox::UpdaterNodeAccess& a = a_node.access;
			const nox::UpdaterNodeAccess& b = b_node.access;
			if (a.exclusive || b.exclusive)
			{
				//	排他アクセス(nox::World&)。宣言の中身に関係なく全ノードと衝突する。
				builder.Append(u8"exclusive");
				return;
			}
			if ((a.group_index != nox::k_invalid_updater_group_index) && (a.group_index == b.group_index))
			{
				//	同じインスタンスのメソッド同士。EntityLogicはエンティティごとのインスタンス、Serviceは1つのインスタンス。
				builder.Append((a_node.kind == nox::UpdaterNodeKind::ServiceMethod) ? u8"service-state" : u8"logic-state");
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

		/// @brief 構築の失敗理由をログへ出す。abortするかどうかは呼び出し側が決める。
		void log_updater_graph_build_failure(const nox::UpdaterGraphBuildResult& result)noexcept
		{
#if !NOX_MASTER
			UpdaterGraphTextBuilder builder;
			builder.Clear();
			builder.Append(u8"UpdaterGraphの構築に失敗しました: ");
			switch (result.error)
			{
			case nox::UpdaterGraphBuildError::None:
				return;
			case nox::UpdaterGraphBuildError::UnresolvedOrderTarget:
				builder.Append(u8"RunAfter / RunBefore に並べた型が EntitySystem / EntityLogic / Service(属性付きメソッドを持ち登録済みのもの)として登録されていません: ");
				break;
			case nox::UpdaterGraphBuildError::OrderTargetInOtherPhase:
				builder.Append(u8"RunAfter / RunBefore に並べた型が、宣言した型と同じフェーズにノードを持ちません(辺が張れません): ");
				break;
			case nox::UpdaterGraphBuildError::OrderCycle:
				builder.Append(u8"明示辺(RunAfter / RunBefore)が循環しています (Phase ");
				builder.Append(to_updater_phase_name(result.phase));
				builder.Append(u8"): ");
				break;
			}

			//	循環のときは「target → declaring」の実行順になっている辺を示す。それ以外は「宣言した型 → 名前」。
			if (result.error == nox::UpdaterGraphBuildError::OrderCycle)
			{
				append_type_and_method_name(builder, result.target_type_name, result.target_method_name);
				builder.Append(u8" -> ");
				append_type_and_method_name(builder, result.declaring_type_name, result.declaring_method_name);
			}
			else
			{
				append_type_and_method_name(builder, result.declaring_type_name, result.declaring_method_name);
				builder.Append(u8" -> ");
				builder.Append(result.target_type_name);
			}

			NOX_ERROR_LINE(nox::log_id::CoreCommon, u8"{0}", builder.GetView());
#else
			(void)result;
#endif // !NOX_MASTER
		}

		/// @brief 構築の失敗で起動を止める。
		/// @details 明示辺の誤りは宣言(コード)の誤りで、続行すると宣言した順序が守られないまま走る。
		///          NOX_ASSERTは診断補助であり、成否の判定には使わない(Masterでは消える)。
		///          ログもMasterでは消えるため、nox::World::AbortOnEntityCommandOverflow と同じく
		///          理由と名前の所在をvolatileなローカルに残し、クラッシュダンプから読めるようにする。
		[[noreturn]] void abort_on_updater_graph_build_failure(const nox::UpdaterGraphBuildResult& result)noexcept
		{
			NOX_ASSERT(false, u8"UpdaterGraphの構築に失敗しました(理由は直前のエラーログ)");

			volatile const nox::uint32 failed_error = nox::util::ToUnderlying(result.error);
			volatile const nox::uint32 failed_phase = nox::util::ToUnderlying(result.phase);
			const char* volatile failed_declaring_type_name = result.declaring_type_name.data();
			volatile const size_t failed_declaring_type_name_length = result.declaring_type_name.length();
			const char* volatile failed_target_type_name = result.target_type_name.data();
			volatile const size_t failed_target_type_name_length = result.target_type_name.length();
			(void)failed_error;
			(void)failed_phase;
			(void)failed_declaring_type_name;
			(void)failed_declaring_type_name_length;
			(void)failed_target_type_name;
			(void)failed_target_type_name_length;

			std::abort();
		}
	}
}

bool nox::ConflictsUpdaterNodeAccess(
	const nox::UpdaterNodeAccess& a,
	const nox::UpdaterNodeAccess& b)noexcept
{
	//	排他アクセス(nox::World&)は何を読み書きするか宣言から導けないので、同じフェーズの全ノードと衝突させる。
	//	衝突辺は全順序の前 → 後へ張られるので、排他ノードは自分より前の全ノードの後ろ、後の全ノードの前の
	//	単独のレイヤーになる。
	if (a.exclusive || b.exclusive)
	{
		return true;
	}

	//	同一EntityLogic型・同一Serviceのメソッド同士はメンバ変数を共有するため、宣言が重ならなくても直列化する。
	if ((a.group_index != nox::k_invalid_updater_group_index) && (a.group_index == b.group_index))
	{
		return true;
	}

	return conflicts_declared_access(a, b);
}

std::string_view nox::GetUpdaterNodeTypeName(const nox::UpdaterNode& node)noexcept
{
	switch (node.kind)
	{
	case nox::UpdaterNodeKind::EntitySystem:
		return node.system->GetDescriptor().name;
	case nox::UpdaterNodeKind::EntityLogicMethod:
		return node.storage->GetDescriptor().name;
	case nox::UpdaterNodeKind::ServiceMethod:
		return node.service_type->name;
	case nox::UpdaterNodeKind::Task:
		return node.task->name;
	}
	return std::string_view();
}

std::string_view nox::GetUpdaterNodeMethodName(const nox::UpdaterNode& node)noexcept
{
	switch (node.kind)
	{
	case nox::UpdaterNodeKind::EntityLogicMethod:
		return node.method->name;
	case nox::UpdaterNodeKind::ServiceMethod:
		return node.service_method->name;
	case nox::UpdaterNodeKind::EntitySystem:
	case nox::UpdaterNodeKind::Task:
		break;
	}
	return std::string_view();
}

nox::uint32 nox::BuildUpdaterLayerIndices(
	const std::span<const nox::UpdaterNodeAccess> accesses,
	const std::span<nox::uint32> dest_layer_indices)noexcept
{
	return nox::BuildUpdaterLayerIndicesWithOrderEdges(
		accesses,
		std::span<const nox::UpdaterOrderEdge>(),
		dest_layer_indices);
}

nox::uint32 nox::BuildUpdaterLayerIndicesWithOrderEdges(
	const std::span<const nox::UpdaterNodeAccess> accesses,
	const std::span<const nox::UpdaterOrderEdge> order_edges,
	const std::span<nox::uint32> dest_layer_indices)noexcept
{
	NOX_ASSERT(dest_layer_indices.size() >= accesses.size(), u8"レイヤー番号の出力先が足りません");
	if (accesses.empty())
	{
		return 0u;
	}

	//	衝突辺も明示辺も必ず「全順序の前 → 後」に張られるので、並びがトポロジカル順そのものになる。
	//	よって最長経路レイヤリング(入ってくる辺の元の最大レイヤー + 1)は、前方への一度の走査に畳める。
	//	明示辺はtoの昇順に並んでいるので、カーソル1本で「このノードに入る辺」だけを拾える。
	nox::uint32 layer_count = 0u;
	size_t edge_cursor = 0u;
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

		for (; (edge_cursor < order_edges.size()) && (order_edges[edge_cursor].to <= index); ++edge_cursor)
		{
			const nox::UpdaterOrderEdge& edge = order_edges[edge_cursor];
			NOX_ASSERT((edge.to == index) && (edge.from < edge.to), u8"明示辺が全順序に沿っていないか、toの昇順に並んでいません");
			if ((edge.to != index) || (edge.from >= edge.to))
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

void nox::ExecuteUpdaterLayer(
	nox::JobSystem& job_system,
	const std::span<const nox::UpdaterNode> nodes,
	const nox::UpdaterNodeExecuteFunction execute,
	void* const context)
{
	//	ワーカーが無い、または1つしか無いレイヤーはその場で回す。配っても往復コストが乗るだけで、
	//	呼び出しスレッド上なので main_thread_only もそのまま満たす。
	if ((job_system.GetWorkerCount() == 0u) || (nodes.size() <= 1u))
	{
		for (const nox::UpdaterNode& node : nodes)
		{
			execute(context, node);
		}
		return;
	}

	//	main_thread_only でないノードだけをジョブにする。確保は一切走らない(スタック上の固定長)。
	std::array<UpdaterLayerJobContext, nox::kMaxUpdaterNodesPerLayer> job_contexts{};
	std::array<nox::Job, nox::kMaxUpdaterNodesPerLayer> jobs{};
	nox::uint32 job_count = 0u;
	//	上限に達した位置。ここから先の(main_thread_only でない)ノードは配らずに直列で回す。
	size_t overflow_index = nodes.size();
	for (size_t index = 0u; index < nodes.size(); ++index)
	{
		const nox::UpdaterNode& node = nodes[index];
		if (node.main_thread_only)
		{
			continue;
		}
		if (job_count == nox::kMaxUpdaterNodesPerLayer)
		{
			overflow_index = index;
			break;
		}

		job_contexts[job_count] = UpdaterLayerJobContext{ .execute = execute, .context = context, .node = &node };
		jobs[job_count] = nox::Job{ .func = &execute_updater_layer_job, .context = &job_contexts[job_count] };
		++job_count;
	}
	NOX_ASSERT(overflow_index == nodes.size(),
		u8"1レイヤーのノード数が上限を超えました 上限={0} 実際={1}",
		nox::kMaxUpdaterNodesPerLayer, static_cast<nox::uint32>(nodes.size()));

	//	先に配る。main_thread_only のノードを呼び出しスレッドで回している間も、ワーカーは配られたノードを進められる
	//	(先に回すと、その間ワーカーが遊ぶ)。同一レイヤーは互いに衝突しないので、どちらが先に走っても結果は同じ。
	nox::JobCounter counter{ 0u };
	if (job_count != 0u)
	{
		job_system.Dispatch(std::span<const nox::Job>(jobs.data(), job_count), counter);
	}

	for (const nox::UpdaterNode& node : nodes)
	{
		if (node.main_thread_only)
		{
			execute(context, node);
		}
	}

	if (job_count != 0u)
	{
		//	待つ側(呼び出しスレッド)も自分でジョブを引いて働く。
		job_system.Wait(counter);
	}

	//	上限を超えた分は取りこぼさずここで直列実行する(アサート済みの異常系)。
	for (size_t index = overflow_index; index < nodes.size(); ++index)
	{
		if (nodes[index].main_thread_only == false)
		{
			execute(context, nodes[index]);
		}
	}
}

nox::UpdaterGraph::UpdaterGraph() :
	phase_nodes_{},
	phase_layer_offsets_{},
	phase_command_buffer_counts_{}
#if !NOX_MASTER
	, phase_order_edges_{}
#endif // !NOX_MASTER
{
}

nox::UpdaterGraph::~UpdaterGraph() = default;

void nox::UpdaterGraph::Rebuild(
	const std::span<nox::EntitySystemBase* const> systems,
	const std::span<nox::EntityLogicStorage* const> storages,
	const std::span<const nox::UpdaterServiceBinding> services,
	const std::span<const nox::UpdaterTaskDescriptor* const> tasks)
{
	const nox::UpdaterGraphBuildResult result = TryRebuild(systems, storages, services, tasks);
	if (result.IsSuccess() == false)
	{
		abort_on_updater_graph_build_failure(result);
	}
}

nox::UpdaterGraphBuildResult nox::UpdaterGraph::TryRebuild(
	const std::span<nox::EntitySystemBase* const> systems,
	const std::span<nox::EntityLogicStorage* const> storages,
	const std::span<const nox::UpdaterServiceBinding> services,
	const std::span<const nox::UpdaterTaskDescriptor* const> tasks)
{
	Clear();

	//	名前の解決はフェーズをまたいで先に行う(理由は validate_updater_order_targets を参照)。
	const nox::UpdaterGraphBuildResult validation = validate_updater_order_targets(systems, storages, services);
	if (validation.IsSuccess() == false)
	{
		log_updater_graph_build_failure(validation);
		return validation;
	}

	for (nox::uint8 phase_index = 0u; phase_index < nox::util::ToUnderlying(nox::SystemPhaseType::_Max); ++phase_index)
	{
		const nox::UpdaterGraphBuildResult result =
			RebuildPhase(static_cast<nox::SystemPhaseType>(phase_index), systems, storages, services, tasks);
		if (result.IsSuccess() == false)
		{
			//	途中まで組んだフェーズを残すと、半端なグラフで走り得る。失敗したら全て空にする。
			Clear();
			log_updater_graph_build_failure(result);
			return result;
		}
	}

	return nox::UpdaterGraphBuildResult{};
}

void nox::UpdaterGraph::Clear()noexcept
{
	for (nox::uint8 phase_index = 0u; phase_index < nox::util::ToUnderlying(nox::SystemPhaseType::_Max); ++phase_index)
	{
		phase_nodes_[phase_index].clear();
		phase_layer_offsets_[phase_index].clear();
		phase_command_buffer_counts_[phase_index] = 0u;
#if !NOX_MASTER
		phase_order_edges_[phase_index].clear();
#endif // !NOX_MASTER
	}
}

nox::UpdaterGraphBuildResult nox::UpdaterGraph::RebuildPhase(
	const nox::SystemPhaseType phase_type,
	const std::span<nox::EntitySystemBase* const> systems,
	const std::span<nox::EntityLogicStorage* const> storages,
	const std::span<const nox::UpdaterServiceBinding> services,
	const std::span<const nox::UpdaterTaskDescriptor* const> tasks)
{
	const nox::uint32 phase_index = nox::util::ToUnderlying(phase_type);
	nox::Vector<nox::UpdaterNode>& dest_nodes = phase_nodes_[phase_index];
	nox::Vector<nox::uint32>& dest_offsets = phase_layer_offsets_[phase_index];
	dest_nodes.clear();
	dest_offsets.clear();
	phase_command_buffer_counts_[phase_index] = 0u;
#if !NOX_MASTER
	phase_order_edges_[phase_index].clear();
#endif // !NOX_MASTER

	//	1. このフェーズのノードを集める。
	//	   ここでの並び(登録順)は結果に影響させない。使うのは同名の二重登録の間の順序だけ。
	nox::Vector<UpdaterBuildNode> build_nodes;
	for (nox::EntitySystemBase* const system : systems)
	{
		const nox::EntitySystemTypeDescriptor& descriptor = system->GetDescriptor();
		if (descriptor.phase != phase_type)
		{
			continue;
		}

		UpdaterBuildNode build_node{};
		build_node.node.access.read_write_mask = descriptor.make_read_write_mask();
		build_node.node.access.write_mask = descriptor.make_write_mask();
		build_node.node.access.service_accesses = descriptor.get_service_accesses();
		build_node.node.access.group_index = nox::k_invalid_updater_group_index;
		build_node.node.kind = nox::UpdaterNodeKind::EntitySystem;
		build_node.node.system = system;
		build_node.node.main_thread_only = descriptor.main_thread_only;
		build_node.type_name = descriptor.name;
		build_node.method_name = std::string_view();
		build_node.registration_index = static_cast<nox::uint32>(build_nodes.size());
		build_node.emits_structural_change = descriptor.emits_structural_change;
		build_nodes.push_back(build_node);
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

			UpdaterBuildNode build_node{};
			build_node.node.access.read_write_mask = method.make_read_write_mask();
			build_node.node.access.write_mask = method.make_write_mask();
			build_node.node.access.service_accesses = method.get_service_accesses();
			//	同一EntityLogic型のメソッド同士を必ず衝突させるためのグループ。番号は同一性にしか使わない。
			build_node.node.access.group_index = storage_index;
			build_node.node.kind = nox::UpdaterNodeKind::EntityLogicMethod;
			build_node.node.storage = storage;
			build_node.node.method = &method;
			//	EntityLogicの kMainThreadOnly は型単位の宣言で、全更新メソッドに掛かる。
			build_node.node.main_thread_only = storage->GetDescriptor().main_thread_only;
			build_node.type_name = storage->GetDescriptor().name;
			build_node.method_name = method.name;
			build_node.registration_index = static_cast<nox::uint32>(build_nodes.size());
			build_node.emits_structural_change = method.emits_structural_change;
			build_nodes.push_back(build_node);
		}
	}

	for (nox::uint32 service_index = 0u; service_index < services.size(); ++service_index)
	{
		const nox::UpdaterServiceBinding& binding = services[service_index];
		for (const nox::ServiceMethodDescriptor& method : binding.descriptor->get_methods())
		{
			if (method.phase != phase_type)
			{
				continue;
			}

			UpdaterBuildNode build_node{};
			//	ComponentDataには触れない(引数に取れない)。宣言はServiceだけで、末尾に自分自身への書き込みを含む。
			build_node.node.access.service_accesses = method.get_service_accesses();
			//	同じServiceのメソッド同士を必ず衝突させるためのグループ。EntityLogicの番号と重ならないよう後ろに続ける。
			build_node.node.access.group_index = static_cast<nox::uint32>(storages.size()) + service_index;
			build_node.node.kind = nox::UpdaterNodeKind::ServiceMethod;
			build_node.node.service = binding.service;
			build_node.node.service_type = binding.descriptor;
			build_node.node.service_method = &method;
			//	nox::World& を取ったメソッドは排他アクセス。同じフェーズの全ノードと衝突する。
			build_node.node.access.exclusive = method.exclusive;
			//	Serviceの実行スレッドはメソッド単位の宣言(nox::attr::ThreadAffinity)。排他アクセスは暗黙にメインスレッド限定。
			build_node.node.main_thread_only = method.main_thread_only || method.exclusive;
			build_node.type_name = binding.descriptor->name;
			build_node.method_name = method.name;
			build_node.registration_index = static_cast<nox::uint32>(build_nodes.size());
			build_node.emits_structural_change = method.emits_structural_change;
			build_nodes.push_back(build_node);
		}
	}

	for (const nox::UpdaterTaskDescriptor* const task : tasks)
	{
		if (task->phase != phase_type)
		{
			continue;
		}

		UpdaterBuildNode build_node{};
		build_node.node.access.service_accesses = task->get_service_accesses();
		//	Taskはインスタンスを持たないので、状態を共有する相手がいない。
		build_node.node.access.group_index = nox::k_invalid_updater_group_index;
		build_node.node.kind = nox::UpdaterNodeKind::Task;
		build_node.node.task = task;
		//	nox::World& を取った Task は排他アクセス(Serviceのメソッドと同じ扱い)。
		build_node.node.access.exclusive = task->exclusive;
		build_node.node.main_thread_only = task->main_thread_only || task->exclusive;
		//	全順序のキーは完全修飾関数名。型名と同じ列で比べる。
		build_node.type_name = task->name;
		build_node.method_name = std::string_view();
		build_node.registration_index = static_cast<nox::uint32>(build_nodes.size());
		build_node.emits_structural_change = task->emits_structural_change;
		build_nodes.push_back(build_node);
	}

	if (build_nodes.empty())
	{
		return nox::UpdaterGraphBuildResult{};
	}

	//	2. 型名順(EntityLogicはさらにメソッド名順)に整列する。これが全順序のタイブレークになる。
	//	   同じ型のノードはここで連続するので、以降は型名の二分探索で範囲を引ける。
	std::sort(build_nodes.begin(), build_nodes.end(), &is_less_updater_build_node);
	const nox::uint32 node_count = static_cast<nox::uint32>(build_nodes.size());
	const std::span<const UpdaterBuildNode> sorted_nodes(build_nodes.data(), build_nodes.size());

	//	3. 明示辺を張る。番号は整列後の位置。
	//	   相手がこのフェーズにノードを持たなければ辺は張らない。宣言そのものの妥当性
	//	   (登録済みか・どこかのフェーズを共有するか)は validate_updater_order_targets で検査済み。
	nox::Vector<nox::UpdaterOrderEdge> edges;
	for (nox::uint32 group_first = 0u; group_first < node_count;)
	{
		nox::uint32 group_last = group_first + 1u;
		while ((group_last < node_count) && (sorted_nodes[group_last].type_name == sorted_nodes[group_first].type_name))
		{
			++group_last;
		}

		//	Taskは型ではないので、明示辺を宣言する側にも張られる側にもならない
		//	(関数名が型名と同じ綴りになる稀な場合でも、型の宣言をTaskへ波及させない)。
		const nox::UpdaterNode& declarer = sorted_nodes[group_first].node;
		for (const std::string_view target_name : get_updater_node_run_after(declarer))
		{
			const UpdaterNodeRange target = find_updater_type_range(sorted_nodes, target_name);
			for (nox::uint32 target_index = target.first; target_index < target.last; ++target_index)
			{
				if (sorted_nodes[target_index].node.kind == nox::UpdaterNodeKind::Task)
				{
					continue;
				}
				for (nox::uint32 node_index = group_first; node_index < group_last; ++node_index)
				{
					if (sorted_nodes[node_index].node.kind == nox::UpdaterNodeKind::Task)
					{
						continue;
					}
					edges.push_back(nox::UpdaterOrderEdge{ .from = target_index, .to = node_index });
				}
			}
		}
		for (const std::string_view target_name : get_updater_node_run_before(declarer))
		{
			const UpdaterNodeRange target = find_updater_type_range(sorted_nodes, target_name);
			for (nox::uint32 target_index = target.first; target_index < target.last; ++target_index)
			{
				if (sorted_nodes[target_index].node.kind == nox::UpdaterNodeKind::Task)
				{
					continue;
				}
				for (nox::uint32 node_index = group_first; node_index < group_last; ++node_index)
				{
					if (sorted_nodes[node_index].node.kind == nox::UpdaterNodeKind::Task)
					{
						continue;
					}
					edges.push_back(nox::UpdaterOrderEdge{ .from = node_index, .to = target_index });
				}
			}
		}

		group_first = group_last;
	}

	//	4. 明示辺だけでトポロジカル順を作る(Kahn法)。候補が複数あれば整列順の最小(= 型名順)を採る。
	//	   全ノードを置き切れなければ明示辺が循環している。
	//	   辺をfromの昇順に並べ、各ノードの出辺を区間として引けるようにする。
	//	同じ辺が重複しうる(AのRunAfter<B>とBのRunBefore<A>など)ので、ここで1本にまとめる。
	std::sort(edges.begin(), edges.end(), [](const nox::UpdaterOrderEdge& a, const nox::UpdaterOrderEdge& b)noexcept
		{
			return (a.from != b.from) ? (a.from < b.from) : (a.to < b.to);
		});
	edges.erase(
		std::unique(edges.begin(), edges.end(), [](const nox::UpdaterOrderEdge& a, const nox::UpdaterOrderEdge& b)noexcept
			{
				return (a.from == b.from) && (a.to == b.to);
			}),
		edges.end());

	nox::Vector<nox::uint32> in_degrees(node_count, 0u);
	nox::Vector<nox::uint32> out_edge_offsets(static_cast<size_t>(node_count) + 1u, 0u);
	for (const nox::UpdaterOrderEdge& edge : edges)
	{
		++in_degrees[edge.to];
		++out_edge_offsets[edge.from + 1u];
	}
	for (nox::uint32 index = 0u; index < node_count; ++index)
	{
		out_edge_offsets[index + 1u] += out_edge_offsets[index];
	}

	//	order_positions[整列後の位置] = 全順序での位置。sorted_indices_in_order はその逆写像。
	static constexpr nox::uint32 k_unplaced = std::numeric_limits<nox::uint32>::max();
	nox::Vector<nox::uint32> order_positions(node_count, k_unplaced);
	nox::Vector<nox::uint32> sorted_indices_in_order;
	sorted_indices_in_order.reserve(node_count);
	for (nox::uint32 position = 0u; position < node_count; ++position)
	{
		nox::uint32 picked = node_count;
		for (nox::uint32 index = 0u; index < node_count; ++index)
		{
			if ((order_positions[index] == k_unplaced) && (in_degrees[index] == 0u))
			{
				picked = index;
				break;
			}
		}

		if (picked == node_count)
		{
			//	循環。未配置のノードは必ず未配置の先行ノードを持つので、先行ノードをノード数ぶん
			//	遡れば必ず閉路の上に乗る(閉路の下流にあるだけのノードを報告しないため)。
			const std::span<const nox::UpdaterOrderEdge> edge_view(edges.data(), edges.size());
			const std::span<const nox::uint32> position_view(order_positions.data(), order_positions.size());
			nox::uint32 on_cycle = 0u;
			while (order_positions[on_cycle] != k_unplaced)
			{
				++on_cycle;
			}
			for (nox::uint32 step = 0u; step < node_count; ++step)
			{
				on_cycle = find_unplaced_predecessor(edge_view, position_view, k_unplaced, on_cycle);
			}
			const nox::uint32 predecessor = find_unplaced_predecessor(edge_view, position_view, k_unplaced, on_cycle);

#if !NOX_MASTER
			//	閉路を一周ぶん書き出す。先行ノードの辿り方は決定的なので、必ず on_cycle へ戻ってくる。
			NOX_ERROR_LINE(nox::log_id::CoreCommon, u8"UpdaterGraph Phase {0}: 明示辺の閉路(後に実行される側 <- 先に実行される側)",
				to_updater_phase_name(phase_type));
			UpdaterGraphTextBuilder builder;
			nox::uint32 current = on_cycle;
			for (nox::uint32 step = 0u; step < node_count; ++step)
			{
				const nox::uint32 previous = find_unplaced_predecessor(edge_view, position_view, k_unplaced, current);
				builder.Clear();
				builder.Append(u8"  ");
				append_type_and_method_name(builder, sorted_nodes[current].type_name, sorted_nodes[current].method_name);
				builder.Append(u8" <- ");
				append_type_and_method_name(builder, sorted_nodes[previous].type_name, sorted_nodes[previous].method_name);
				NOX_ERROR_LINE(nox::log_id::CoreCommon, u8"{0}", builder.GetView());
				current = previous;
				if (current == on_cycle)
				{
					break;
				}
			}
#endif // !NOX_MASTER

			return nox::UpdaterGraphBuildResult{
				.error = nox::UpdaterGraphBuildError::OrderCycle,
				.phase = phase_type,
				.declaring_type_name = sorted_nodes[on_cycle].type_name,
				.declaring_method_name = sorted_nodes[on_cycle].method_name,
				.target_type_name = sorted_nodes[predecessor].type_name,
				.target_method_name = sorted_nodes[predecessor].method_name,
			};
		}

		order_positions[picked] = position;
		sorted_indices_in_order.push_back(picked);
		for (nox::uint32 edge_index = out_edge_offsets[picked]; edge_index < out_edge_offsets[picked + 1u]; ++edge_index)
		{
			--in_degrees[edges[edge_index].to];
		}
	}

	//	5. 全順序に並べ直し、order_index とコマンドバッファ番号を振る。
	//	   コマンドバッファ番号は「遅延構造変更を出しうるノードだけ」に、全順序の昇順で詰めて振る。
	//	   よって番号順の再生は全順序での再生と同じ並びになり、実行順とも矛盾しない。
	nox::uint32 command_buffer_index = 0u;
	nox::Vector<nox::UpdaterNode> nodes;
	nodes.reserve(node_count);
	for (nox::uint32 position = 0u; position < node_count; ++position)
	{
		const UpdaterBuildNode& build_node = sorted_nodes[sorted_indices_in_order[position]];
		nox::UpdaterNode node = build_node.node;
		node.order_index = position;
		node.command_buffer_index = build_node.emits_structural_change
			? command_buffer_index++
			: nox::k_invalid_updater_command_buffer_index;
		nodes.push_back(node);
	}
	phase_command_buffer_counts_[phase_index] = command_buffer_index;

	//	6. 明示辺を全順序の番号へ付け替え、toの昇順に並べる(BuildUpdaterLayerIndicesWithOrderEdgesの前提)。
	//	   トポロジカル順なので付け替え後は必ず from < to になる。
	for (nox::UpdaterOrderEdge& edge : edges)
	{
		edge = nox::UpdaterOrderEdge{ .from = order_positions[edge.from], .to = order_positions[edge.to] };
	}
	std::sort(edges.begin(), edges.end(), [](const nox::UpdaterOrderEdge& a, const nox::UpdaterOrderEdge& b)noexcept
		{
			return (a.to != b.to) ? (a.to < b.to) : (a.from < b.from);
		});

	nox::Vector<nox::UpdaterNodeAccess> accesses;
	accesses.reserve(node_count);
	for (const nox::UpdaterNode& node : nodes)
	{
		accesses.push_back(node.access);
	}

	nox::Vector<nox::uint32> layer_indices(node_count, 0u);
	const nox::uint32 layer_count = nox::BuildUpdaterLayerIndicesWithOrderEdges(
		std::span<const nox::UpdaterNodeAccess>(accesses.data(), accesses.size()),
		std::span<const nox::UpdaterOrderEdge>(edges.data(), edges.size()),
		std::span<nox::uint32>(layer_indices.data(), layer_indices.size()));

	//	7. (レイヤー, 全順序)で整列する。全順序の走査を外に出さないので安定。
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

			nox::UpdaterNode node = nodes[index];
			node.layer_index = layer_index;
			dest_nodes.push_back(node);
		}
	}
	//	末尾番兵。GetLayerNodesが分岐なしで範囲を作れる。
	dest_offsets.push_back(static_cast<nox::uint32>(dest_nodes.size()));

#if !NOX_MASTER
	phase_order_edges_[phase_index] = std::move(edges);
#endif // !NOX_MASTER

	return nox::UpdaterGraphBuildResult{};
}

std::span<const nox::UpdaterNode> nox::UpdaterGraph::GetNodes(const nox::SystemPhaseType phase_type)const noexcept
{
	const nox::Vector<nox::UpdaterNode>& nodes = phase_nodes_[nox::util::ToUnderlying(phase_type)];
	return std::span<const nox::UpdaterNode>(nodes.data(), nodes.size());
}

#if !NOX_MASTER
std::span<const nox::UpdaterOrderEdge> nox::UpdaterGraph::GetOrderEdges(const nox::SystemPhaseType phase_type)const noexcept
{
	const nox::Vector<nox::UpdaterOrderEdge>& edges = phase_order_edges_[nox::util::ToUnderlying(phase_type)];
	return std::span<const nox::UpdaterOrderEdge>(edges.data(), edges.size());
}
#endif // !NOX_MASTER

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
		const nox::uint32 node_count = static_cast<nox::uint32>(nodes.size());
		const nox::Vector<nox::UpdaterOrderEdge>& order_edges = phase_order_edges_[phase_index];

		//	引数の数は既存のログ行と揃えてある。新しい引数個数で NOX_INFO_LINE を実体化すると、
		//	kernel/string_format.h 側の既存警告(-Wmissing-braces)がその実体化ぶんだけ増えるため。
		NOX_INFO_LINE(nox::log_id::CoreCommon, u8"UpdaterGraph Phase: {0} (nodes={1}, layers={2})",
			to_updater_phase_name(phase_type),
			node_count,
			GetLayerCount(phase_type));
		NOX_INFO_LINE(nox::log_id::CoreCommon, u8"  Phase {0}: コマンドバッファ={1}本 / ノード={2}",
			to_updater_phase_name(phase_type),
			GetCommandBufferCount(phase_type),
			node_count);

		//	order_index → nodes内の位置。nodesは(レイヤー, 全順序)で並んでいるので引き直す。
		nox::Vector<nox::uint32> positions_by_order(node_count, 0u);
		for (nox::uint32 position = 0u; position < node_count; ++position)
		{
			positions_by_order[nodes[position].order_index] = position;
		}

		for (const nox::UpdaterNode& node : nodes)
		{
			builder.Clear();
			append_node_name(builder, node);
			builder.Append(u8" kind=");
			builder.Append(to_updater_node_kind_name(node.kind));
			builder.Append(u8" rw=");
			append_component_mask(builder, node.access.read_write_mask);
			builder.Append(u8" w=");
			append_component_mask(builder, node.access.write_mask);
			builder.Append(u8" services=");
			append_service_accesses(builder, node.access.service_accesses);
			if (node.command_buffer_index != nox::k_invalid_updater_command_buffer_index)
			{
				builder.Append(u8" cb=");
				builder.Append(node.command_buffer_index);
			}
			if (node.main_thread_only)
			{
				//	ワーカーへ配られず、フェーズを回しているスレッドで実行される。
				builder.Append(u8" main-thread-only");
			}
			if (node.access.exclusive)
			{
				//	nox::World& を取った排他アクセス。同じフェーズの全ノードと衝突し、単独のレイヤーになる。
				builder.Append(u8" exclusive");
			}

			NOX_INFO_LINE(nox::log_id::CoreCommon, u8"  Layer {0}: n{1} {2}",
				node.layer_index,
				node.order_index,
				builder.GetView());
		}

		//	明示辺(RunAfter / RunBefore)。衝突の有無に関係なく直列化の理由になる。
		for (const nox::UpdaterOrderEdge& edge : order_edges)
		{
			builder.Clear();
			builder.Append(u8"explicit");
			NOX_INFO_LINE(nox::log_id::CoreCommon, u8"  EDGE n{0} -> n{1} ({2})",
				edge.from,
				edge.to,
				builder.GetView());
		}

		//	衝突辺(=直列化の理由)。全順序の前から後へ張られる。
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
				append_conflict_reason(builder, from, to);
				NOX_INFO_LINE(nox::log_id::CoreCommon, u8"  EDGE n{0} -> n{1} ({2})",
					from.order_index,
					to.order_index,
					builder.GetView());
			}
		}

		//	明示辺が無く、型名順だけで向きが決まった衝突(write/write・write/read)の一覧。
		//	明示辺を辿って to へ届くなら向きは宣言で決まっている。届かなければ型名で決まっただけで、
		//	型名を変えると実行順が入れ替わる。因果のある組ならRunAfter / RunBeforeで宣言すべき候補。
		//	同一EntityLogic型のメソッド同士(logic-state)はメソッド名順が規則なので、宣言の衝突が無ければ出さない。
		//	排他アクセス(exclusive)だけによる衝突も出さない(宣言の衝突だけを見る)。排他ノードは全ノードと衝突するので、
		//	出すとフェーズの全ノードとの組が並んで他の候補が埋もれる。排他ノードの位置は上の Layer 行で読める。
		nox::Vector<nox::uint32> out_edge_offsets(static_cast<size_t>(node_count) + 1u, 0u);
		nox::Vector<nox::uint32> out_edge_targets(order_edges.size(), 0u);
		for (const nox::UpdaterOrderEdge& edge : order_edges)
		{
			++out_edge_offsets[edge.from + 1u];
		}
		for (nox::uint32 index = 0u; index < node_count; ++index)
		{
			out_edge_offsets[index + 1u] += out_edge_offsets[index];
		}
		{
			nox::Vector<nox::uint32> fill_cursors(out_edge_offsets.begin(), out_edge_offsets.end() - 1);
			for (const nox::UpdaterOrderEdge& edge : order_edges)
			{
				out_edge_targets[fill_cursors[edge.from]++] = edge.to;
			}
		}

		nox::uint32 by_name_pair_count = 0u;
		nox::Vector<nox::uint8> reachable(node_count, 0u);
		nox::Vector<nox::uint32> stack;
		stack.reserve(node_count);
		for (nox::uint32 from_order = 0u; from_order < node_count; ++from_order)
		{
			//	from_order から明示辺だけで届くノード。明示辺は前方にしか張られないので深さ優先で足りる。
			std::ranges::fill(reachable, static_cast<nox::uint8>(0u));
			stack.clear();
			stack.push_back(from_order);
			while (stack.empty() == false)
			{
				const nox::uint32 current = stack.back();
				stack.pop_back();
				for (nox::uint32 edge_index = out_edge_offsets[current]; edge_index < out_edge_offsets[current + 1u]; ++edge_index)
				{
					const nox::uint32 next = out_edge_targets[edge_index];
					if (reachable[next] == 0u)
					{
						reachable[next] = 1u;
						stack.push_back(next);
					}
				}
			}

			const nox::UpdaterNode& from = nodes[positions_by_order[from_order]];
			for (nox::uint32 to_order = from_order + 1u; to_order < node_count; ++to_order)
			{
				const nox::UpdaterNode& to = nodes[positions_by_order[to_order]];
				if ((reachable[to_order] != 0u) || (conflicts_declared_access(from.access, to.access) == false))
				{
					continue;
				}
				//	同じServiceのメソッド同士は、暗黙の自己書き込みで必ず宣言が衝突する。
				//	順序はメソッド名順という規則なので(EntityLogicの logic-state と同じく)候補に出さない。
				if ((from.kind == nox::UpdaterNodeKind::ServiceMethod) &&
					(to.kind == nox::UpdaterNodeKind::ServiceMethod) &&
					(from.access.group_index == to.access.group_index))
				{
					continue;
				}

				++by_name_pair_count;
				builder.Clear();
				append_conflict_reason(builder, from, to);
				builder.Append(u8": ");
				append_node_name(builder, from);
				builder.Append(u8" -> ");
				append_node_name(builder, to);
				NOX_INFO_LINE(nox::log_id::CoreCommon, u8"  BY-NAME n{0} -> n{1} ({2})",
					from.order_index,
					to.order_index,
					builder.GetView());
			}
		}

		NOX_INFO_LINE(nox::log_id::CoreCommon, u8"  Phase {0}: 型名順だけで向きが決まった衝突={1}組 / 明示辺={2}本",
			to_updater_phase_name(phase_type),
			by_name_pair_count,
			static_cast<nox::uint32>(order_edges.size()));
	}
}
#endif // !NOX_MASTER
