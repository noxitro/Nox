// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	world.cpp
/// @brief	world
#include "pch.h"
#include "world.h"

#include "engine_module.h"
#include "log_id.h"

namespace nox
{
	namespace
	{
		[[nodiscard]]
		constexpr bool is_live_generation(nox::uint32 generation) noexcept
		{
			return (generation & 1u) != 0u;
		}

		[[nodiscard]]
		constexpr nox::uint32 get_entity_record_page_index(nox::uint32 index, nox::uint32 entity_record_page_shift) noexcept
		{
			return index >> entity_record_page_shift;
		}

		[[nodiscard]]
		constexpr nox::uint32 get_entity_record_offset(nox::uint32 index, nox::uint32 entity_record_page_mask) noexcept
		{
			return index & entity_record_page_mask;
		}

		[[nodiscard]]
		constexpr nox::uint64 make_free_entity_head(nox::uint32 index, nox::uint32 version) noexcept
		{
			return (static_cast<nox::uint64>(version) << 32u) | static_cast<nox::uint64>(index);
		}

		[[nodiscard]]
		constexpr nox::EntityId make_entity_id(nox::uint32 generation, nox::uint32 index) noexcept
		{
			return nox::EntityId{
				(static_cast<nox::uint64>(index) << 32u) | static_cast<nox::uint64>(generation)
			};
		}

		[[nodiscard]]
		constexpr nox::uint32 get_free_entity_head_index(nox::uint64 head) noexcept
		{
			return static_cast<nox::uint32>(head & 0xffffffffull);
		}

		[[nodiscard]]
		constexpr nox::uint32 get_free_entity_head_version(nox::uint64 head) noexcept
		{
			return static_cast<nox::uint32>(head >> 32u);
		}

#if !NOX_MASTER
		[[nodiscard]]
		constexpr std::u8string_view to_graph_phase_name(const nox::SystemPhaseType phase_type) noexcept
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

		struct RuntimeGraphTextBuilder
		{
			std::array<nox::char8, 3072> buffer{};
			size_t length = 0;

			void Append(std::u8string_view value)noexcept
			{
				const size_t writable_length = std::min(value.length(), buffer.size() - length - 1);
				std::ranges::copy_n(value.data(), writable_length, buffer.data() + length);
				length += writable_length;
				buffer[length] = u8'\0';
			}

			void Append(std::string_view value)noexcept
			{
				const size_t writable_length = std::min(value.length(), buffer.size() - length - 1);
				for (size_t i = 0; i < writable_length; ++i)
				{
					buffer[length + i] = static_cast<nox::char8>(value[i]);
				}
				length += writable_length;
				buffer[length] = u8'\0';
			}

			void Append(nox::uint32 value)noexcept
			{
				std::array<char, 16> temp{};
				const auto [ptr, ec] = std::to_chars(temp.data(), temp.data() + temp.size(), value);
				if (ec == std::errc{})
				{
					Append(std::string_view(temp.data(), static_cast<size_t>(ptr - temp.data())));
				}
			}
		};
#endif // !NOX_MASTER
	}
}

nox::World::World() :
	free_entity_head_(make_free_entity_head(k_invalid_entity_index, 0u)),
	next_entity_index_(0u),
	first_entity_record_page_(),
	entity_record_pages_{},
	entity_record_page_mutex_(),
	stop_watch_(),
	frame_counter_(0u),
	elapsed_milli_seconds_(0.0f),
	next_elapsed_milli_seconds_(0.0f),
	target_frame_rate_(60),
	enabled_vsync_(true),
	studio_mode_(nox::os::ContainsCommandLineArgKey(u"--studio")),
	kill_(false),
	is_executing_system_phase_(false),
	entity_command_buffer_(),
	archetypes_(),
	entity_systems_(),
	entity_logic_storages_(),
	entity_system_phase_table_{},
	services_(),
	modules_(),
	systems_(),
	system_map_(),
	system_phase_table_{}
{
	for (auto&& entity_record_page : entity_record_pages_)
	{
		entity_record_page.store(nullptr, std::memory_order_relaxed);
	}

	entity_record_pages_[0].store(&first_entity_record_page_, std::memory_order_relaxed);

	//	Archetypeの追加でVectorが再確保されてもポインタは動かないが、確保回数自体を減らしておく。
	archetypes_.reserve(k_initial_archetype_capacity);
}

nox::World::~World()
{
	for (nox::EntityLogicStorage* const storage : entity_logic_storages_)
	{
		delete storage;
	}

	for (nox::EntitySystemBase* const entity_system : entity_systems_)
	{
		entity_system->Destroy();
	}

	for (nox::Archetype* const archetype : archetypes_)
	{
		delete archetype;
	}

	for (nox::uint32 service_index = 0u; service_index < services_.GetLength(); ++service_index)
	{
		delete services_.GetStorage()[service_index].service;
	}

	for (nox::SystemBase* const system : systems_)
	{
		delete system;
	}

	for (nox::EngineModule* const module : modules_)
	{
		delete module;
	}

	for (nox::uint32 page_index = 1u; page_index < k_max_entity_page_count; ++page_index)
	{
		nox::World::EntityRecordPage* const page = entity_record_pages_[page_index].load(std::memory_order_relaxed);
		if (page != nullptr)
		{
			delete page;
		}
	}
}
void nox::World::Run()
{
	Init();
	stop_watch_.Start();

	nox::os::Thread game_thread;
	game_thread.SetThreadName(u"Game");
	game_thread.Dispatch([this]()
		{
			ExecutePhase(nox::SystemPhaseType::Init);
			ExecutePhase(nox::SystemPhaseType::Start);

			while (!kill_.load(std::memory_order_acquire))
			{
				Update();
			}

			ExecutePhase(nox::SystemPhaseType::Terminate);
		});

	while (nox::os::Update())
	{
	}

	kill_.store(true, std::memory_order_release);
	game_thread.Wait();
	Exit();
}

void nox::World::SetVSync(bool flag)noexcept
{
	enabled_vsync_ = flag;
}

nox::SystemBase* nox::World::FindSystem(const nox::reflection::Type& type)const noexcept
{
	const auto it = system_map_.find(&type);
	if (it == system_map_.end())
	{
		return nullptr;
	}
	return it->second;
}

nox::SystemBase& nox::World::GetSystem(const nox::reflection::Type& type)const
{
	nox::SystemBase* const system = FindSystem(type);
	if (system != nullptr)
	{
		return *system;
	}
	NOX_ASSERT(false, u8"システムが見つかりませんでした: {0}", type.GetTypeName());
	std::abort();
}

void nox::World::Init()
{
	nox::reflection::ForeachDerivedClassInfoList(
		nox::reflection::Typeof<nox::EngineModule>(),
		[this](const nox::reflection::ClassInfo& class_info)
		{
			auto* const module = static_cast<nox::EngineModule*>(class_info.GetType().CreateObject());
			NOX_ASSERT(module != nullptr, u8"EngineModuleの生成に失敗しました: {0}", class_info.GetFullName());
			if (module != nullptr)
			{
				modules_.emplace_back(module);
			}
		});

	nox::FixedVector<nox::SystemBase*, 128> system_list;
	{
		nox::StackAllocVector<nox::SystemBase*, 512> system_dest_buffer_vector;
		auto& dest_buffer = system_dest_buffer_vector.GetContainer();
		dest_buffer.reserve(32);

		for (const nox::EngineModule* const module : modules_)
		{
			module->CreateEngineSystems(dest_buffer);

			for (nox::SystemBase* const system : dest_buffer)
			{
				if (system == nullptr)
				{
					NOX_ASSERT(false, u8"EngineSystemの生成結果にnullが含まれています");
					continue;
				}

				const nox::reflection::Type& type = system->GetType();
				if (system_map_.contains(&type))
				{
					NOX_ASSERT(false, u8"登録済み: {0}", type.GetTypeName());
					delete system;
					continue;
				}

				RegisterSystem(*system);
				systems_.emplace_back(system);
				system_list.PushBack(system);
			}

			dest_buffer.clear();
		}
	}

	BuildExecuteNodeList(system_list);
	CreateEntitySystems();
	CreateEntityLogicStorages();

#if !NOX_MASTER
	TraceExecuteNodeList();
#endif // !NOX_MASTER
}

void nox::World::Update()
{
	elapsed_milli_seconds_ = stop_watch_.ElapsedMilliseconds();
	if (enabled_vsync_)
	{
		if (elapsed_milli_seconds_ < next_elapsed_milli_seconds_)
		{
			nox::os::Thread::Sleep(1);
			return;
		}
	}

	ExecutePhase(nox::SystemPhaseType::Update);
	++frame_counter_;

	next_elapsed_milli_seconds_ += (1000.0f / static_cast<nox::float_t>(target_frame_rate_));
	stop_watch_.Restart();
}

void nox::World::Exit()
{
	kill_.store(true, std::memory_order_release);
	system_map_.clear();

	for (auto& layer : system_phase_table_)
	{
		layer.clear();
		layer.shrink_to_fit();
	}
}

void nox::World::BuildExecuteNodeList(std::span<nox::SystemBase*> system_list)
{
	struct Node
	{
		std::reference_wrapper<nox::SystemBase> instance;
		std::reference_wrapper<const nox::SystemBase::SystemPhase> phase;
		std::span<const std::reference_wrapper<const nox::SystemBase::SystemPhase>> dependencies;
		std::span<const std::reference_wrapper<const nox::SystemBase::SystemPhase>> depended;
	};

	for (nox::uint8 phase_index = 0; phase_index < nox::util::ToUnderlying(nox::SystemPhaseType::_Max); ++phase_index)
	{
		const auto current_phase_type = static_cast<nox::SystemPhaseType>(phase_index);

		nox::Vector<Node> nodes;
		for (nox::SystemBase* const system : system_list)
		{
			for (const nox::SystemBase::PhaseRegister& reg : system->GetPhaseRegisterList())
			{
				if (reg.GetPhase().type != current_phase_type)
				{
					continue;
				}

				nodes.push_back(Node{
					*system,
					reg.GetPhase(),
					reg.GetDependencies(),
					reg.GetDepended()
					});
			}
		}

		if (nodes.empty())
		{
			continue;
		}

		nox::UnorderedMap<const nox::SystemBase::SystemPhase*, nox::uint32> phase_to_index;
		phase_to_index.reserve(nodes.size());
		for (nox::uint32 i = 0; i < nodes.size(); ++i)
		{
			phase_to_index.emplace(&nodes[i].phase.get(), i);
		}

		auto& dest = system_phase_table_[phase_index];
		dest.reserve(nodes.size());

		nox::Vector<nox::Vector<nox::uint32>> dependency_indices(nodes.size());
		for (nox::uint32 i = 0; i < nodes.size(); ++i)
		{
			for (const std::reference_wrapper<const nox::SystemBase::SystemPhase>& dep_ref : nodes[i].dependencies)
			{
				const auto it = phase_to_index.find(&dep_ref.get());
				if (it == phase_to_index.end())
				{
					continue;
				}
				dependency_indices[i].push_back(it->second);
			}

			for (const std::reference_wrapper<const nox::SystemBase::SystemPhase>& depended_ref : nodes[i].depended)
			{
				const auto it = phase_to_index.find(&depended_ref.get());
				if (it == phase_to_index.end())
				{
					continue;
				}
				dependency_indices[it->second].push_back(i);
			}
		}

		nox::Vector<nox::uint8> states(nodes.size(), 0);
		nox::Vector<nox::uint32> layer_indices(nodes.size(), 0);
		nox::uint32 max_layer_index = 0;

		auto visit = [&](nox::uint32 node_index, auto& self) -> void
		{
			if (states[node_index] == 2)
			{
				return;
			}
			if (states[node_index] == 1)
			{
				NOX_ASSERT(false, u8"Phase依存に循環があります: {0}", nodes[node_index].phase.get().name);
				return;
			}

			states[node_index] = 1;
			nox::uint32 max_dependency_layer = 0;
			for (const nox::uint32 dependency_index : dependency_indices[node_index])
			{
				self(dependency_index, self);
				max_dependency_layer = std::max(max_dependency_layer, layer_indices[dependency_index] + 1);
			}

			states[node_index] = 2;
			layer_indices[node_index] = max_dependency_layer;
			max_layer_index = std::max(max_layer_index, max_dependency_layer);
		};

		for (nox::uint32 i = 0; i < nodes.size(); ++i)
		{
			visit(i, visit);
		}

		for (nox::uint32 layer_index = 0; layer_index <= max_layer_index; ++layer_index)
		{
			for (nox::uint32 i = 0; i < nodes.size(); ++i)
			{
				if (layer_indices[i] != layer_index)
				{
					continue;
				}

				dest.push_back(ExecuteNode{ nodes[i].instance, nodes[i].phase, layer_index });
			}
		}
	}
}

void nox::World::ExecutePhase(const nox::SystemPhaseType phase_type)
{
	const nox::Vector<ExecuteNode>& layers = system_phase_table_[nox::util::ToUnderlying(phase_type)];
	is_executing_system_phase_.store(true, std::memory_order_release);
	for (const ExecuteNode& layer : layers)
	{
		std::invoke(layer.phase.get().func, &layer.instance.get(), *this);
	}

	ExecuteEntitySystemPhase(phase_type);
	ExecuteEntityLogicPhase(phase_type);

	//	将来の並列ディスパッチでは、このplaybackポイントまでに全Systemジョブをjoinする必要がある。
	FlushEntityCommands();
	is_executing_system_phase_.store(false, std::memory_order_release);
}

void nox::World::CreateEntitySystems()
{
	for (const nox::EntitySystemTypeDescriptor* descriptor = nox::detail::GetEntitySystemTypeListHead();
		descriptor != nullptr;
		descriptor = descriptor->next)
	{
		nox::EntitySystemBase* const entity_system = descriptor->create();
		if (entity_system == nullptr)
		{
			NOX_ASSERT(false, u8"EntitySystemの生成に失敗しました");
			continue;
		}

		//	生成済みArchetypeをQueryへ反映する。以降はArchetype追加時に差分だけが通知される。
		for (nox::Archetype* const archetype : archetypes_)
		{
			entity_system->GetQuery().TryAddArchetype(*archetype);
		}

		entity_systems_.push_back(entity_system);
		entity_system_phase_table_[nox::util::ToUnderlying(descriptor->phase)].push_back(entity_system);
	}
}

void nox::World::ExecuteEntitySystemPhase(const nox::SystemPhaseType phase_type)
{
	//	同一フェーズ内は、宣言(引数リスト)から導出したマスクが衝突しない限り並列実行できる。
	//	現状は登録順の直列実行だが、依存解析の入力は既に揃っている。
	for (nox::EntitySystemBase* const entity_system : entity_system_phase_table_[nox::util::ToUnderlying(phase_type)])
	{
		entity_system->Execute(*this);
	}
}

void nox::World::CreateEntityLogicStorages()
{
	for (const nox::EntityLogicTypeDescriptor* descriptor = nox::detail::GetEntityLogicTypeListHead();
		descriptor != nullptr;
		descriptor = descriptor->next)
	{
#if !NOX_MASTER
		//	更新メソッドが宣言したComponentDataは、必ず必須ComponentDataに含まれていなければならない。
		//	含まれていないと、インスタンスが存在するのに列が引けないentityが生じる。
		const nox::ComponentMask required_mask = descriptor->make_required_mask();
		for (const nox::EntityLogicMethodDescriptor& method : descriptor->get_methods())
		{
			NOX_ASSERT(required_mask.Contains(method.make_read_write_mask()),
				u8"EntityLogicの更新メソッドが必須ComponentDataの外を宣言しています: {0}", method.name);
		}
#endif // !NOX_MASTER

		entity_logic_storages_.push_back(new nox::EntityLogicStorage(*descriptor));
	}
}

void nox::World::RefreshEntityLogics(const nox::EntityId entity, const nox::Archetype* const archetype)
{
	for (nox::EntityLogicStorage* const storage : entity_logic_storages_)
	{
		const bool satisfied =
			(archetype != nullptr) && archetype->GetMask().Contains(storage->GetDescriptor().make_required_mask());
		if (satisfied)
		{
			storage->CreateInstance(*this, entity);
		}
		else
		{
			storage->DestroyInstance(entity);
		}
	}
}

void nox::World::ExecuteEntityLogicPhase(const nox::SystemPhaseType phase_type)
{
	for (nox::EntityLogicStorage* const storage : entity_logic_storages_)
	{
		const std::span<const nox::EntityLogicMethodDescriptor> methods = storage->GetDescriptor().get_methods();
		for (const nox::EntityLogicMethodDescriptor& method : methods)
		{
			if (method.phase != phase_type)
			{
				continue;
			}

			for (const nox::EntityLogicStorage::Entry& entry : storage->GetEntries())
			{
				const auto* const entity_record = TryGetEntityRecord(entry.entity.index);
				if (entity_record == nullptr || entity_record->archetype == nullptr)
				{
					continue;
				}
				method.invoke(entry.instance, *this, *entity_record->archetype, entity_record->location, entry.entity);
			}
		}
	}
}

void nox::World::RegisterSystem(nox::SystemBase& system)
{
	const nox::reflection::Type& type = system.GetType();
	if (system_map_.contains(&type))
	{
		NOX_ASSERT(false, u8"登録済み: {0}", type.GetTypeName());
		return;
	}

	system_map_.emplace(&type, &system);
}

#if !NOX_MASTER
nox::U8FixedString<3072> nox::World::BuildRuntimeDependencyGraphText()const
{
	struct PhaseNode
	{
		const nox::SystemBase::SystemPhase* phase = nullptr;
		const nox::SystemBase* instance = nullptr;
		nox::uint32 id = 0;
		nox::uint32 layer = 0;
	};

	RuntimeGraphTextBuilder builder;
	nox::Vector<PhaseNode> phase_nodes;
	phase_nodes.reserve(128);

	for (nox::uint32 phase_type_index = 0; phase_type_index < nox::util::ToUnderlying(nox::SystemPhaseType::_Max); ++phase_type_index)
	{
		const nox::SystemPhaseType phase_type = static_cast<nox::SystemPhaseType>(phase_type_index);
		const nox::Vector<ExecuteNode>& execute_nodes = system_phase_table_[phase_type_index];
		for (const ExecuteNode& execute_node : execute_nodes)
		{
			const nox::uint32 id = static_cast<nox::uint32>(phase_nodes.size());
			const nox::uint32 graph_layer = (phase_type_index * 8u) + execute_node.layer_index;
			phase_nodes.push_back(PhaseNode{
				&execute_node.phase.get(),
				&execute_node.instance.get(),
				id,
				graph_layer,
				});

			builder.Append(u8"NODE|n");
			builder.Append(id);
			builder.Append(u8"|");
			builder.Append(execute_node.instance.get().GetType().GetTypeName());
			builder.Append(u8"|");
			builder.Append(execute_node.phase.get().name);
			builder.Append(u8"|");
			builder.Append(to_graph_phase_name(phase_type));
			builder.Append(u8"|");
			builder.Append(graph_layer);
			builder.Append(u8"\n");
		}
	}

	const auto find_node_id = [&phase_nodes](const nox::SystemBase::SystemPhase& phase) -> std::optional<nox::uint32>
	{
		for (const PhaseNode& node : phase_nodes)
		{
			if (node.phase == &phase)
			{
				return node.id;
			}
		}
		return std::nullopt;
	};

	for (const PhaseNode& node : phase_nodes)
	{
		const nox::SystemBase::PhaseRegister* current_register = nullptr;
		for (const nox::SystemBase::PhaseRegister& phase_register : node.instance->GetPhaseRegisterList())
		{
			if (&phase_register.GetPhase() == node.phase)
			{
				current_register = &phase_register;
				break;
			}
		}
		if (current_register == nullptr)
		{
			continue;
		}

		for (const std::reference_wrapper<const nox::SystemBase::SystemPhase>& dependency : current_register->GetDependencies())
		{
			if (const std::optional<nox::uint32> dependency_id = find_node_id(dependency.get()))
			{
				builder.Append(u8"EDGE|n");
				builder.Append(*dependency_id);
				builder.Append(u8"|n");
				builder.Append(node.id);
				builder.Append(u8"|depends\n");
			}
		}

		for (const std::reference_wrapper<const nox::SystemBase::SystemPhase>& depended : current_register->GetDepended())
		{
			if (const std::optional<nox::uint32> depended_id = find_node_id(depended.get()))
			{
				builder.Append(u8"EDGE|n");
				builder.Append(node.id);
				builder.Append(u8"|n");
				builder.Append(*depended_id);
				builder.Append(u8"|depends\n");
			}
		}
	}

	nox::U8FixedString<3072> graph_text;
	graph_text.Assign(std::u8string_view(builder.buffer.data(), builder.length));
	return graph_text;
}

void nox::World::TraceExecuteNodeList()const
{
	for (nox::uint8 phase_index = 0; phase_index < nox::util::ToUnderlying(nox::SystemPhaseType::_Max); ++phase_index)
	{
		const auto current_phase_type = static_cast<nox::SystemPhaseType>(phase_index);
		const auto& layers = system_phase_table_[phase_index];
		if (layers.empty())
		{
			continue;
		}

		NOX_INFO_LINE(nox::log_id::CoreCommon, u"Phase: {0}", current_phase_type);
		for (const ExecuteNode& node : layers)
		{
			NOX_INFO_LINE(nox::log_id::CoreCommon, u"  Layer {0}: {1}", node.layer_index, node.phase.get().name);
		}
	}
}
#endif // !NOX_MASTER

nox::World::EntityRecord* nox::World::TryGetEntityRecord(nox::uint32 index) noexcept
{
	const nox::uint32 page_index = get_entity_record_page_index(index, k_entity_record_page_shift);
	if (page_index >= k_max_entity_page_count)
	{
		return nullptr;
	}

	auto* entity_record_page = entity_record_pages_[page_index].load(std::memory_order_acquire);
	if (entity_record_page == nullptr)
	{
		return nullptr;
	}

	return &entity_record_page->records[get_entity_record_offset(index, k_entity_record_page_mask)];
}

const nox::World::EntityRecord* nox::World::TryGetEntityRecord(nox::uint32 index) const noexcept
{
	return const_cast<nox::World*>(this)->TryGetEntityRecord(index);
}

nox::World::EntityRecord* nox::World::EnsureEntityRecord(nox::uint32 index)
{
	const nox::uint32 page_index = get_entity_record_page_index(index, k_entity_record_page_shift);
	if (page_index >= k_max_entity_page_count)
	{
		NOX_ASSERT(false, u8"World entity capacity exceeded: index={0}", index);
		std::abort();
	}

	auto* entity_record_page = entity_record_pages_[page_index].load(std::memory_order_acquire);
	if (entity_record_page == nullptr)
	{
		std::lock_guard<std::mutex> lock(entity_record_page_mutex_);
		entity_record_page = entity_record_pages_[page_index].load(std::memory_order_relaxed);
		if (entity_record_page == nullptr)
		{
			entity_record_page = new EntityRecordPage();
			entity_record_pages_[page_index].store(entity_record_page, std::memory_order_release);
		}
	}

	return &entity_record_page->records[get_entity_record_offset(index, k_entity_record_page_mask)];
}

nox::uint32 nox::World::TryPopFreeEntityIndex() noexcept
{
	nox::uint64 head = free_entity_head_.load(std::memory_order_acquire);
	while (true)
	{
		const nox::uint32 index = get_free_entity_head_index(head);
		if (index == k_invalid_entity_index)
		{
			return k_invalid_entity_index;
		}

		auto* entity_record = TryGetEntityRecord(index);
		NOX_ASSERT(entity_record != nullptr, u8"Invalid free entity slot: index={0}", index);
		if (entity_record == nullptr)
		{
			return k_invalid_entity_index;
		}

		const nox::uint32 next_index = entity_record->next_free_index.load(std::memory_order_relaxed);
		const nox::uint64 next_head = make_free_entity_head(next_index, get_free_entity_head_version(head) + 1u);
		if (free_entity_head_.compare_exchange_weak(head, next_head, std::memory_order_acq_rel, std::memory_order_acquire))
		{
			return index;
		}
	}
}

void nox::World::PushFreeEntityIndex(nox::uint32 index) noexcept
{
	auto* entity_record = TryGetEntityRecord(index);
	NOX_ASSERT(entity_record != nullptr, u8"Invalid free entity slot push: index={0}", index);
	if (entity_record == nullptr)
	{
		return;
	}

	nox::uint64 head = free_entity_head_.load(std::memory_order_acquire);
	while (true)
	{
		entity_record->next_free_index.store(get_free_entity_head_index(head), std::memory_order_relaxed);
		const nox::uint64 next_head = make_free_entity_head(index, get_free_entity_head_version(head) + 1u);
		if (free_entity_head_.compare_exchange_weak(head, next_head, std::memory_order_release, std::memory_order_acquire))
		{
			return;
		}
	}
}

nox::EntityId nox::World::CreateEntity()
{
	NOX_ASSERT(is_executing_system_phase_.load(std::memory_order_acquire) == false, u8"SystemからのEntity生成はサポートされていません");
	if (is_executing_system_phase_.load(std::memory_order_acquire))
	{
		return nox::EntityId{ 0u };
	}

	nox::uint32 index = TryPopFreeEntityIndex();
	if (index != k_invalid_entity_index)
	{
		auto* entity_record = TryGetEntityRecord(index);
		NOX_ASSERT(entity_record != nullptr, u8"Invalid recycled entity slot: index={0}", index);
		if (entity_record == nullptr)
		{
			std::abort();
		}

		const nox::uint32 current_generation = entity_record->generation.load(std::memory_order_relaxed);
		NOX_ASSERT(is_live_generation(current_generation) == false, u8"Recycled slot must be free: index={0}", index);
		const nox::uint32 next_generation = current_generation + 1u;
		NOX_ASSERT(next_generation != 0u, u8"Entity generation overflow: index={0}", index);

		entity_record->archetype = nullptr;
		entity_record->location = nox::ArchetypeLocation::Invalid();
		entity_record->next_free_index.store(k_invalid_entity_index, std::memory_order_relaxed);
		entity_record->generation.store(next_generation, std::memory_order_release);

		return make_entity_id(next_generation, index);
	}

	index = next_entity_index_.fetch_add(1u, std::memory_order_relaxed);
	if (index >= k_max_entity_count)
	{
		NOX_ASSERT(false, u8"World entity capacity exceeded: index={0}", index);
		std::abort();
	}

	auto* entity_record = EnsureEntityRecord(index);
	entity_record->archetype = nullptr;
	entity_record->location = nox::ArchetypeLocation::Invalid();
	entity_record->next_free_index.store(k_invalid_entity_index, std::memory_order_relaxed);
	entity_record->generation.store(k_initial_live_generation, std::memory_order_release);

	return make_entity_id(k_initial_live_generation, index);
}

void nox::World::DestroyEntity(nox::EntityId entity)
{
	NOX_ASSERT(is_executing_system_phase_.load(std::memory_order_acquire) == false, u8"SystemからのEntity破棄にはQueueDestroyEntityを使用してください");
	if (is_executing_system_phase_.load(std::memory_order_acquire))
	{
		return;
	}

	DestroyEntityImmediate(entity);
}

void nox::World::DestroyEntityImmediate(const nox::EntityId entity)noexcept
{
	auto* entity_record = TryGetEntityRecord(entity.index);
	if (entity_record == nullptr)
	{
		return;
	}

	nox::uint32 expected_generation = entity.generation;
	if (is_live_generation(expected_generation) == false)
	{
		return;
	}

	const nox::uint32 next_generation = expected_generation + 1u;
	NOX_ASSERT(next_generation != 0u, u8"Entity generation overflow: index={0}", entity.index);

	if (entity_record->generation.compare_exchange_strong(expected_generation, next_generation, std::memory_order_acq_rel, std::memory_order_acquire) == false)
	{
		return;
	}

	//	ComponentDataの列から抜く。swap-removeで詰めた分だけ他entityの位置を更新する。
	MoveEntityToArchetype(*entity_record, entity, nullptr);
	PushFreeEntityIndex(entity.index);
}

bool nox::World::QueueDestroyEntity(const nox::EntityId entity)noexcept
{
	NOX_ASSERT(is_executing_system_phase_.load(std::memory_order_acquire), u8"QueueDestroyEntityはSystem実行中のみ使用できます");
	if (is_executing_system_phase_.load(std::memory_order_acquire) == false)
	{
		return false;
	}

	const bool queued = entity_command_buffer_.TryDestroy(entity);
	NOX_ASSERT(queued, u8"EntityCommandBuffer capacity exceeded: {0}", k_entity_command_capacity);
	if (queued == false)
	{
		std::abort();
	}
	return queued;
}

void nox::World::FlushEntityCommands()noexcept
{
	entity_command_buffer_.BeginPlayback();
	nox::uint32 command_index = 0u;
	while (command_index < entity_command_buffer_.GetLength())
	{
		nox::EntityCommand command{};
		const bool ready = entity_command_buffer_.TryGet(command_index++, command);
		NOX_ASSERT(ready, u8"EntityCommandBuffer command was not published");
		if (ready == false)
		{
			std::abort();
		}

		switch (command.type)
		{
		case nox::EntityCommandType::Destroy:
			DestroyEntityImmediate(nox::EntityId{ command.entity_raw });
			break;
		default:
			NOX_ASSERT(false, u8"Unknown entity command");
			break;
		}
	}
	entity_command_buffer_.Clear();
}

bool nox::World::IsAlive(nox::EntityId entity)const noexcept
{
	const auto* entity_record = TryGetEntityRecord(entity.index);
	if (entity_record == nullptr)
	{
		return false;
	}

	const nox::uint32 current_generation = entity_record->generation.load(std::memory_order_acquire);
	return is_live_generation(current_generation) && (current_generation == entity.generation);
}

#pragma region ComponentData

void* nox::World::AddComponent(const nox::EntityId entity, const nox::ComponentTypeInfo& type_info)
{
	NOX_ASSERT(is_executing_system_phase_.load(std::memory_order_acquire) == false,
		u8"System実行中の構造変更はサポートされていません");
	if (is_executing_system_phase_.load(std::memory_order_acquire))
	{
		return nullptr;
	}

	auto* const entity_record = TryGetEntityRecord(entity.index);
	if (entity_record == nullptr || entity_record->generation.load(std::memory_order_acquire) != entity.generation)
	{
		NOX_ASSERT(false, u8"破棄済みのentityにComponentDataを追加しようとしました: index={0}", entity.index);
		return nullptr;
	}

	nox::ComponentMask mask = (entity_record->archetype != nullptr) ? entity_record->archetype->GetMask() : nox::ComponentMask{};
	if (mask.Test(type_info.index) == false)
	{
		mask.Set(type_info.index);
		MoveEntityToArchetype(*entity_record, entity, &GetOrCreateArchetype(mask));
	}

	void* const column = entity_record->archetype->TryGetComponentArray(entity_record->location.chunk_index, type_info.index);
	if (column == nullptr)
	{
		return nullptr;
	}
	return static_cast<nox::uint8*>(column) + static_cast<size_t>(entity_record->location.row) * type_info.size;
}

void nox::World::RemoveComponent(const nox::EntityId entity, const nox::ComponentTypeInfo& type_info)
{
	NOX_ASSERT(is_executing_system_phase_.load(std::memory_order_acquire) == false,
		u8"System実行中の構造変更はサポートされていません");
	if (is_executing_system_phase_.load(std::memory_order_acquire))
	{
		return;
	}

	auto* const entity_record = TryGetEntityRecord(entity.index);
	if (entity_record == nullptr ||
		entity_record->generation.load(std::memory_order_acquire) != entity.generation ||
		entity_record->archetype == nullptr)
	{
		return;
	}

	nox::ComponentMask mask = entity_record->archetype->GetMask();
	if (mask.Test(type_info.index) == false)
	{
		return;
	}

	mask.Reset(type_info.index);
	MoveEntityToArchetype(*entity_record, entity, mask.IsEmpty() ? nullptr : &GetOrCreateArchetype(mask));
}

void* nox::World::TryGetComponent(const nox::EntityId entity, const nox::ComponentTypeIndex type_index)noexcept
{
	const auto* const entity_record = TryGetEntityRecord(entity.index);
	if (entity_record == nullptr ||
		entity_record->generation.load(std::memory_order_acquire) != entity.generation ||
		entity_record->archetype == nullptr)
	{
		return nullptr;
	}

	void* const column = entity_record->archetype->TryGetComponentArray(entity_record->location.chunk_index, type_index);
	if (column == nullptr)
	{
		return nullptr;
	}

	const nox::ComponentTypeInfo* const type_info = nox::detail::TryGetComponentTypeInfo(type_index);
	NOX_ASSERT(type_info != nullptr, u8"未登録のComponentTypeIndexが指定されました");
	if (type_info == nullptr)
	{
		return nullptr;
	}
	return static_cast<nox::uint8*>(column) + static_cast<size_t>(entity_record->location.row) * type_info->size;
}

bool nox::World::HasComponent(const nox::EntityId entity, const nox::ComponentTypeIndex type_index)const noexcept
{
	const auto* const entity_record = TryGetEntityRecord(entity.index);
	if (entity_record == nullptr ||
		entity_record->generation.load(std::memory_order_acquire) != entity.generation ||
		entity_record->archetype == nullptr)
	{
		return false;
	}
	return entity_record->archetype->GetMask().Test(type_index);
}

#pragma endregion

#pragma region Archetype

nox::Archetype* nox::World::TryFindArchetype(const nox::ComponentMask& mask)const noexcept
{
	for (nox::Archetype* const archetype : archetypes_)
	{
		if (archetype->GetMask() == mask)
		{
			return archetype;
		}
	}
	return nullptr;
}

nox::Archetype& nox::World::GetOrCreateArchetype(const nox::ComponentMask& mask)
{
	if (nox::Archetype* const found = TryFindArchetype(mask); found != nullptr)
	{
		return *found;
	}

	nox::FixedVector<const nox::ComponentTypeInfo*, nox::k_max_component_type_per_archetype> type_list;
	for (nox::uint32 raw_type_index = 0u; raw_type_index < nox::k_max_component_type_count; ++raw_type_index)
	{
		const auto type_index = static_cast<nox::ComponentTypeIndex>(raw_type_index);
		if (mask.Test(type_index) == false)
		{
			continue;
		}

		const nox::ComponentTypeInfo* const type_info = nox::detail::TryGetComponentTypeInfo(type_index);
		NOX_ASSERT(type_info != nullptr, u8"未登録のComponentTypeIndexがマスクに含まれています: {0}", raw_type_index);
		if (type_info != nullptr)
		{
			type_list.PushBack(type_info);
		}
	}

	auto* const archetype = new nox::Archetype(
		mask,
		std::span(type_list.GetStorage().data(), type_list.GetLength()));
	archetypes_.push_back(archetype);

	//	既存のQueryへ即座に通知する。以降このArchetypeの照合は二度と走らない。
	for (nox::EntitySystemBase* const entity_system : entity_systems_)
	{
		entity_system->GetQuery().TryAddArchetype(*archetype);
	}
	return *archetype;
}

void nox::World::MoveEntityToArchetype(nox::World::EntityRecord& entity_record, const nox::EntityId entity, nox::Archetype* const destination)
{
	nox::Archetype* const source = entity_record.archetype;
	if (source == destination)
	{
		return;
	}

	nox::ArchetypeLocation destination_location = nox::ArchetypeLocation::Invalid();
	if (destination != nullptr)
	{
		destination_location = destination->AddEntity(entity);
		if (source != nullptr)
		{
			source->CopySharedComponents(entity_record.location, *destination, destination_location);
		}
	}

	if (source != nullptr)
	{
		const nox::EntityId moved_entity = source->RemoveEntity(entity_record.location);
		if (moved_entity.raw != 0ull)
		{
			PatchMovedEntityLocation(moved_entity, entity_record.location);
		}
	}

	entity_record.archetype = destination;
	entity_record.location = destination_location;

	//	構造変更フック。宣言したComponentDataが揃った/欠けたEntityLogicを追従させる。
	RefreshEntityLogics(entity, destination);
}

void nox::World::PatchMovedEntityLocation(const nox::EntityId moved_entity, const nox::ArchetypeLocation location)noexcept
{
	auto* const moved_record = TryGetEntityRecord(moved_entity.index);
	NOX_ASSERT(moved_record != nullptr, u8"swap-removeで移動したentityのレコードが見つかりません");
	if (moved_record != nullptr)
	{
		moved_record->location = location;
	}
}

nox::Archetype* nox::World::TryGetArchetype(const nox::EntityId entity)const noexcept
{
	const auto* const entity_record = TryGetEntityRecord(entity.index);
	if (entity_record == nullptr || entity_record->generation.load(std::memory_order_acquire) != entity.generation)
	{
		return nullptr;
	}
	return entity_record->archetype;
}

nox::ArchetypeLocation nox::World::GetArchetypeLocation(const nox::EntityId entity)const noexcept
{
	const auto* const entity_record = TryGetEntityRecord(entity.index);
	if (entity_record == nullptr || entity_record->generation.load(std::memory_order_acquire) != entity.generation)
	{
		return nox::ArchetypeLocation::Invalid();
	}
	return entity_record->location;
}

void nox::World::BuildQuery(nox::EntityQuery& query, const nox::ComponentMask& required_mask)
{
	query.Reset(required_mask);
	for (nox::Archetype* const archetype : archetypes_)
	{
		query.TryAddArchetype(*archetype);
	}
}

#pragma endregion

#pragma region Service

void nox::World::RegisterService(const nox::reflection::Type& type, nox::Service& service)
{
	NOX_ASSERT(TryGetService(type) == nullptr, u8"Serviceが二重に登録されました: {0}", type.GetTypeName());
	services_.PushBack(nox::World::ServiceEntry{ .type = &type, .service = &service });
}

nox::Service* nox::World::TryGetService(const nox::reflection::Type& type)const noexcept
{
	for (nox::uint32 service_index = 0u; service_index < services_.GetLength(); ++service_index)
	{
		const nox::World::ServiceEntry& entry = services_.GetStorage()[service_index];
		if (entry.type == &type)
		{
			return entry.service;
		}
	}
	return nullptr;
}

nox::Service* nox::detail::TryGetServiceOfWorld(nox::World& world, const nox::reflection::Type& type)noexcept
{
	return world.TryGetService(type);
}

#pragma endregion
