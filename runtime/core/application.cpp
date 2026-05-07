//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	application.cpp
///	@brief	application
//import std;
#include	"pch.h"
#include	"application.h"

#include	"engine_module.h"
#include	"scene_view.h"
#include	"engine_system.h"
#include	"log_id.h"

namespace nox
{
	namespace
	{
		inline	void HookException(nox::uint32 code, ::_EXCEPTION_POINTERS* const exception_ptr)
		{
		}
	}
}

nox::Application::Application()noexcept :
	enabled_vsync_(true),
	target_frame_rate_(60),
	kill_(false),
	studio_mode_(nox::os::ContainsCommandLineArgKey(u"--studio"))
{
}

nox::Application::~Application()
{

}

inline	void	nox::Application::Init()
{
	RegisterEngineSystem(*this);

	//	モジュールエントリクラス群を収集
	nox::reflection::ForeachDerivedClassInfoList(nox::reflection::Typeof<nox::EngineModule>(),
		[this](const nox::reflection::ClassInfo& class_info) {

			nox::EngineModule*const module_entry = static_cast<nox::EngineModule*>(class_info.GetType().CreateObject());
			module_entry_list_.emplace_back(*module_entry);
		});

	nox::FixedVector<nox::EngineSystem*, 128> engine_system_list;
	{
		nox::FixedPmrArena<sizeof(nox::EngineSystem*) * 32> engine_system_dest_buffer_arena;
		auto dest_buffer = engine_system_dest_buffer_arena.MakeVector<nox::EngineSystem*>();

		for (const nox::EngineModule& entry : module_entry_list_)
		{
			entry.CreateEngineSystems(dest_buffer);

			for (nox::EngineSystem* engine_system : dest_buffer)
			{
				const nox::reflection::Type& type = engine_system->GetType();
				if (engine_system_map_.contains(&type) == true)
				{
					NOX_ASSERT(false, u8"");
					continue;
				}

				RegisterEngineSystem(*engine_system);
				engine_system_list.PushBack(engine_system);
			}
			dest_buffer.clear();
		}
	}

	//	フェーズ実行ノード構築
	BuildExecuteNodeList(engine_system_list);

#if !NOX_MASTER
	TraceExecuteNodeList();
#endif // !NOX_MASTER

}

void nox::Application::BuildExecuteNodeList(std::span<nox::EngineSystem*> system_list)
{
	struct Node
	{
		std::reference_wrapper<nox::EngineSystem> instance;
		std::reference_wrapper<const nox::EngineSystem::SystemPhase> phase;
		std::span<const std::reference_wrapper<const nox::EngineSystem::SystemPhase>> dependencies;
      std::span<const std::reference_wrapper<const nox::EngineSystem::SystemPhase>> depended;
	};

	for (nox::uint8 phase_index = 0; phase_index < nox::util::ToUnderlying(nox::SystemPhaseType::_Max); ++phase_index)
	{
		const auto current_phase_type = static_cast<nox::SystemPhaseType>(phase_index);

		//	当該フェーズタイプのノードを収集
		nox::Vector<Node> nodes;
		for (nox::EngineSystem* system : system_list)
		{
			for (const nox::EngineSystem::PhaseRegister& reg : system->GetPhaseRegisterList())
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

		//	依存先（SystemPhase*）→ ノードインデックスのマップ
		nox::UnorderedMap<const nox::EngineSystem::SystemPhase*, nox::uint32> phase_to_index;
		phase_to_index.reserve(nodes.size());
		for (nox::uint32 i = 0; i < nodes.size(); ++i)
		{
			phase_to_index.emplace(&nodes[i].phase.get(), i);
		}

		auto& dest = system_phase_table_[phase_index];
		dest.reserve(nodes.size());

      //	dependencies / depended から実行ノード間の依存関係を構築
		nox::Vector<nox::Vector<nox::uint32>> dependency_indices(nodes.size());
		for (nox::uint32 i = 0; i < nodes.size(); ++i)
		{
			for (const std::reference_wrapper<const nox::EngineSystem::SystemPhase>& dep_ref : nodes[i].dependencies)
			{
				auto it = phase_to_index.find(&dep_ref.get());
				if (it == phase_to_index.end())
				{
					continue;
				}
				dependency_indices[i].push_back(it->second);
			}

			for (const std::reference_wrapper<const nox::EngineSystem::SystemPhase>& depended_ref : nodes[i].depended)
			{
				auto it = phase_to_index.find(&depended_ref.get());
				if (it == phase_to_index.end())
				{
					continue;
				}
				dependency_indices[it->second].push_back(i);
			}
		}

		//	0=未訪問, 1=訪問中, 2=確定
		nox::Vector<nox::uint8> states(nodes.size(), 0);
		nox::Vector<nox::uint32> layer_indices(nodes.size(), 0);
		nox::uint32 max_layer_index = 0;

		auto visit = [&](nox::uint32 idx, auto& self) -> void {
			if (states[idx] == 2)
			{
				return;
			}
			if (states[idx] == 1)
			{
				NOX_ASSERT(false, u8"Phase依存に循環があります: {0}", nodes[idx].phase.get().name);
				return;
			}
			states[idx] = 1;
			nox::uint32 max_dep_layer = 0;
            for (const nox::uint32 dep_index : dependency_indices[idx])
			{
                self(dep_index, self);
				max_dep_layer = std::max(max_dep_layer, layer_indices[dep_index] + 1);
			}
			states[idx] = 2;
			layer_indices[idx] = max_dep_layer;
            max_layer_index = std::max(max_layer_index, max_dep_layer);
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

void	nox::Application::Run()
{
	this->Init();
	stop_watch_.Start();

	nox::os::Thread game_thread;
	game_thread.SetThreadName(u"Game");
	game_thread.Dispatch([this]() {

		ExecutePhase(nox::SystemPhaseType::Init);
		ExecutePhase(nox::SystemPhaseType::Start);

		while (!this->kill_)
		{
			try
			{
				this->Update();
			}
			catch (const std::exception&)
			{
				//	
				break;
			}
		}

		ExecutePhase(nox::SystemPhaseType::Terminate);
		});


	while (nox::os::Update())
	{
		//	...
	}

	//	ここを抜けたらkill
	kill_ = true;

	game_thread.Wait();

	Exit();
}

void nox::Application::SetVSync(bool flag)noexcept
{
	enabled_vsync_ = flag;
}

inline	void	nox::Application::Update()
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

	nox::os::ContainsCommandLineArgKey(u"--studio");

	//	次のフレーム更新時間
	next_elapsed_milli_seconds_ += (1000.0f / static_cast<nox::float_t>(target_frame_rate_));

	stop_watch_.Restart();
}

inline	void	nox::Application::Exit()
{
	kill_ = true;

	for (const auto& [type, system] : engine_system_map_)
	{
		if (*type == GetType())
		{
			continue;
		}

		delete system;
	}

	engine_system_map_.clear();
	for (auto& layer : system_phase_table_)
	{
		layer.clear();
		layer.shrink_to_fit();
	}

	for (nox::uint32 i = 0; i < module_entry_list_.size(); ++i)
	{
		nox::EngineModule& entry = module_entry_list_[i];
		delete (&entry);
	}

	module_entry_list_.clear();
	module_entry_list_.shrink_to_fit();
}

nox::EngineSystem* nox::Application::FindSystem(const nox::reflection::Type& type)const noexcept
{
	auto it = engine_system_map_.find(&type);
	if (it == engine_system_map_.end())
	{
		return nullptr;
	}
	return it->second;
}

nox::EngineSystem& nox::Application::GetSystem(const nox::reflection::Type& type)const
{
	auto* const system = FindSystem(type);
	if (system == nullptr)
	{
		NOX_ASSERT(false, u8"システムが見つかりませんでした: {0}", type.GetTypeName());
		throw std::runtime_error("System not found");
	}
	return *system;
}

void nox::Application::ExecutePhase(const nox::SystemPhaseType phase_type)
{
	const nox::Vector<ExecuteNode>& layers = system_phase_table_[nox::util::ToUnderlying(phase_type)];
	for (const ExecuteNode& layer : layers)
	{
		std::invoke(layer.phase.get().func, &layer.instance.get(), *this);
	}
}

void nox::Application::RegisterEngineSystem(nox::EngineSystem& engine_system)
{
	const nox::reflection::Type& type = engine_system.GetType();
	if (engine_system_map_.contains(&type) == true)
	{
		NOX_ASSERT(false, u8"登録済み: {0}", type.GetTypeName());
		return;
	}

	engine_system_map_.emplace(&type, &engine_system);
}
#if !NOX_MASTER
void nox::Application::TraceExecuteNodeList()const
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
std::span<const nox::EngineSystem::PhaseRegister> nox::Application::GetPhaseRegisterList()const noexcept
{
	return {};
}