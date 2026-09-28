// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	world.cpp
/// @brief	world
#include "pch.h"
#include "world.h"

#include "engine_module.h"
#include "entity_type_registry.h"
#include "log_id.h"

namespace nox
{
	namespace
	{
		/// @brief 遅延構造変更の記録先を表すスレッドローカルな束縛。
		/// @details 「今このスレッドが実行しているノード」を指す。worldが一致しない、
		///          あるいはノード番号が確保済み本数を超えている場合はノード外バッファへ落とす。
		///          スレッドローカルにしてあるのは、記録側(nox::EntityCommands)が
		///          Worldへのポインタ1つしか持たない薄いビューだからである。
		///          ここに置かなければ、System / EntityLogic の引数リストか
		///          コンストラクタに「自分がどのノードか」を書かせることになる。
		struct NodeCommandBinding
		{
			const nox::World* world = nullptr;
			nox::uint32 node_index = 0u;
		};

		thread_local NodeCommandBinding t_node_command_binding{};

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

		/// @brief 旧SystemPhaseの依存宣言が成立しないときに、理由を残して起動を止める。
		/// @details 見つからない依存や循環を黙って読み飛ばすと、宣言した順序が守られないまま走り続け、
		///          原因がフレームの挙動として遠くに現れる。原因はデータではなくコード
		///          (PhaseRegisterの書き間違い・登録漏れ)なので、Masterでも続行しない。
		///          NOX_ASSERTは診断補助であり、成否の判定には使わない(Masterでは消える)。
		///          ログもMasterでは消えるため、nox::World::AbortOnEntityCommandOverflow と同じく
		///          名前の所在をvolatileなローカルに残し、クラッシュダンプから読めるようにする。
		/// @param reason 失敗の種類。
		/// @param phase_name 依存を宣言した側のフェーズ名。
		/// @param related_phase_name 依存先(循環なら閉路を閉じた相手)のフェーズ名。
		[[noreturn]] void abort_on_system_phase_dependency_error(
			const std::u8string_view reason,
			const std::u8string_view phase_name,
			const std::u8string_view related_phase_name)noexcept
		{
			NOX_ERROR_LINE(nox::log_id::CoreCommon, u8"SystemPhaseの依存が成立しません: {0} ({1} -> {2})",
				reason,
				phase_name,
				related_phase_name);
			NOX_ASSERT(false, u8"SystemPhaseの依存が成立しません: {0} ({1} -> {2})",
				reason,
				phase_name,
				related_phase_name);

			const char8_t* volatile failed_phase_name = phase_name.data();
			volatile const size_t failed_phase_name_length = phase_name.length();
			const char8_t* volatile failed_related_phase_name = related_phase_name.data();
			volatile const size_t failed_related_phase_name_length = related_phase_name.length();
			(void)failed_phase_name;
			(void)failed_phase_name_length;
			(void)failed_related_phase_name;
			(void)failed_related_phase_name_length;

			std::abort();
		}

		/// @brief Serviceの初期化の失敗で起動を止める。
		/// @details 理由のログは nox::World::TryInitializeServices が出し終えている。ここでは残すだけ。
		///          NOX_ASSERTは診断補助であり、成否の判定には使わない(Masterでは消える)。
		///          ログもMasterでは消えるため、abort_on_system_phase_dependency_error と同じく
		///          理由と名前の所在をvolatileなローカルに残し、クラッシュダンプから読めるようにする。
		[[noreturn]] void abort_on_service_initialize_failure(const nox::ServiceInitializeResult& result)noexcept
		{
			NOX_ASSERT(false, u8"Serviceの初期化に失敗しました(理由は直前のエラーログ)");

			volatile const nox::uint32 failed_error = nox::util::ToUnderlying(result.error);
			const char* volatile failed_service_type_name = result.service_type_name.data();
			volatile const size_t failed_service_type_name_length = result.service_type_name.length();
			const char* volatile failed_related_type_name = result.related_type_name.data();
			volatile const size_t failed_related_type_name_length = result.related_type_name.length();
			(void)failed_error;
			(void)failed_service_type_name;
			(void)failed_service_type_name_length;
			(void)failed_related_type_name;
			(void)failed_related_type_name_length;

			std::abort();
		}

#if !NOX_MASTER
		[[nodiscard]]
		constexpr std::u8string_view to_graph_phase_name(const nox::SystemPhaseType phase_type) noexcept
		{
			switch (phase_type)
			{
			case nox::SystemPhaseType::Init: return u8"Init";
			case nox::SystemPhaseType::Start: return u8"Start";
			case nox::SystemPhaseType::FrameIngress: return u8"FrameIngress";
			case nox::SystemPhaseType::Update: return u8"Update";
			case nox::SystemPhaseType::Terminate: return u8"Terminate";
			default: return u8"Unknown";
			}
		}

		/// @brief 同じServiceを2回宣言している場合、2回目以降はチェッカーに入らないための判定。
		/// @details RWParallelExecuteCheckerは同一スレッドからの再入も並列とみなすため、
		///          1ノード内での重複Enterを避ける必要がある。
		[[nodiscard]] bool is_duplicated_service_access(
			const std::span<const nox::ServiceAccess> accesses,
			const nox::uint32 index)noexcept
		{
			for (nox::uint32 earlier_index = 0u; earlier_index < index; ++earlier_index)
			{
				if (accesses[earlier_index].type == accesses[index].type)
				{
					return true;
				}
			}
			return false;
		}

		/// @brief 同じServiceを複数回宣言している場合の、実効的な書き込み権限。
		/// @details 1つでも非constで受けていればそのノードはWriterとして扱う。
		[[nodiscard]] bool is_service_write_access(
			const std::span<const nox::ServiceAccess> accesses,
			const nox::uint32 index)noexcept
		{
			bool write = false;
			for (nox::uint32 other_index = 0u; other_index < accesses.size(); ++other_index)
			{
				if (accesses[other_index].type == accesses[index].type)
				{
					write = write || accesses[other_index].write;
				}
			}
			return write;
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

		/// @brief Serviceの初期化の失敗理由をログへ出す。abortするかどうかは呼び出し側が決める。
		/// @details 引数1つ(u8string_view)で出す。新しい引数の組で NOX_ERROR_LINE を実体化すると、
		///          kernel/string_format.h 側の既存警告(-Wmissing-braces)がその実体化ぶんだけ増えるため。
		void log_service_initialize_failure(const nox::ServiceInitializeResult& result)noexcept
		{
#if !NOX_MASTER
			RuntimeGraphTextBuilder builder;
			builder.Append(u8"Serviceの初期化に失敗しました: ");
			switch (result.error)
			{
			case nox::ServiceInitializeError::None:
				return;
			case nox::ServiceInitializeError::DuplicateService:
				builder.Append(u8"同じ型のServiceが2つ登録されています: ");
				builder.Append(result.service_type_name);
				break;
			case nox::ServiceInitializeError::UnresolvedDependency:
				builder.Append(u8"Depends に並べた型が Service として登録されていません: ");
				builder.Append(result.service_type_name);
				builder.Append(u8" -> ");
				builder.Append(result.related_type_name);
				break;
			case nox::ServiceInitializeError::DependencyCycle:
				builder.Append(u8"Depends が循環しています(先に初期化される側 -> 後に初期化される側): ");
				builder.Append(result.related_type_name);
				builder.Append(u8" -> ");
				builder.Append(result.service_type_name);
				break;
			case nox::ServiceInitializeError::InitializeFailed:
				builder.Append(u8"OnInitialize が false を返しました: ");
				builder.Append(result.service_type_name);
				break;
			case nox::ServiceInitializeError::UndeclaredServiceAccess:
				builder.Append(u8"OnInitialize の中で Depends に並べていない型を引きました: ");
				builder.Append(result.service_type_name);
				builder.Append(u8" -> ");
				builder.Append(result.related_type_name);
				break;
			}
			NOX_ERROR_LINE(nox::log_id::CoreCommon, u8"{0}", std::u8string_view(builder.buffer.data(), builder.length));
#else
			(void)result;
#endif // !NOX_MASTER
		}
	}
}

nox::WorldNodeCommandScope::WorldNodeCommandScope(const nox::World& world, const nox::uint32 node_index)noexcept :
	previous_world_(nox::t_node_command_binding.world),
	previous_node_index_(nox::t_node_command_binding.node_index)
{
	//	必ず保存・復元する。ジョブを配った側のスレッドは Wait の内側で別ノードのジョブを引くため、
	//	set/clear だと戻ってきたときに束縛が失われる。
	nox::t_node_command_binding.world = &world;
	nox::t_node_command_binding.node_index = node_index;
}

nox::WorldNodeCommandScope::~WorldNodeCommandScope()noexcept
{
	nox::t_node_command_binding.world = previous_world_;
	nox::t_node_command_binding.node_index = previous_node_index_;
}

nox::uint32 nox::ResolveUpdaterWorkerCount(
	const std::span<const nox::char16* const> command_line_args,
	const nox::uint32 default_worker_count)noexcept
{
	static constexpr std::u16string_view k_serial_key = u"--serial-updater";
	static constexpr std::u16string_view k_workers_key = u"--updater-workers";

	//	--serial-updater は「1本も作らない」の明示指定。--updater-workers より強い。
	//	先に全体を舐めるので、並び順に関係なくこちらが勝つ。
	if (nox::os::ContainsCommandLineArgKey(command_line_args, k_serial_key) == true)
	{
		return 0u;
	}

	//	キーの照合と値の切り出しは nox::os 側に寄せてある。
	//	"--updater-workersX" のような別の引数を拾わないこと、区切り文字を値に含めないことは
	//	TryGetCommandLineArgValue が保証する。
	const std::optional<std::u16string_view> value =
		nox::os::TryGetCommandLineArgValue(command_line_args, k_workers_key);
	if (value.has_value() == false)
	{
		return default_worker_count;
	}
	if (value->empty() == true)
	{
		//	"--updater-workers" や "--updater-workers=" だけ。値の書き忘れなので既定へ落とす。
		return default_worker_count;
	}

	//	10進の非負整数だけを受ける。ヒープも例外も使わないので自前で読む。
	nox::uint32 parsed = 0u;
	for (const nox::char16 character : *value)
	{
		if ((character < u'0') || (character > u'9'))
		{
			//	数字でない文字が混ざっていたら指定そのものを無視して既定へ落とす。
			//	黙って0本(=直列)にすると、打ち間違いが性能低下として表れて原因が見えない。
			return default_worker_count;
		}
		parsed = (parsed * 10u) + static_cast<nox::uint32>(character - u'0');
		if (parsed > nox::JobSystem::k_max_worker_count)
		{
			//	方針ではなく桁あふれ対策。ここで打ち切らないとuint32を回り込む。
			return nox::JobSystem::k_max_worker_count;
		}
	}
	return parsed;
}

nox::uint32 nox::ResolveExitAfterFrames(const std::span<const nox::char16* const> command_line_args)noexcept
{
	static constexpr std::u16string_view kExitAfterFramesKey = u"--exit-after-frames";

	//	キーの照合と値の切り出しは nox::os 側に寄せてある (ResolveUpdaterWorkerCount と同じ)。
	const std::optional<std::u16string_view> value =
		nox::os::TryGetCommandLineArgValue(command_line_args, kExitAfterFramesKey);
	if ((value.has_value() == false) || (value->empty() == true))
	{
		//	指定が無い、または "--exit-after-frames" / "--exit-after-frames=" だけ。自動では終了しない。
		return 0u;
	}

	//	10進の非負整数だけを受ける。ヒープも例外も使わないので自前で読む。
	nox::uint32 parsed = 0u;
	for (const nox::char16 character : *value)
	{
		if ((character < u'0') || (character > u'9'))
		{
			//	数字でない文字が混ざっていたら指定そのものを無視する。
			//	打ち間違いで即座に終了するより、終了しない方が CI の制限時間で気付ける。
			return 0u;
		}
		parsed = (parsed * 10u) + static_cast<nox::uint32>(character - u'0');
		if (parsed > nox::kMaxExitAfterFrames)
		{
			//	方針ではなく桁あふれ対策。ここで打ち切らないとuint32を回り込む。
			return nox::kMaxExitAfterFrames;
		}
	}
	return parsed;
}

nox::World::World() :
	free_entity_head_(make_free_entity_head(k_invalid_entity_index, 0u)),
	next_entity_index_(0u),
	first_entity_record_page_(),
	entity_record_pages_{},
	stop_watch_(),
	frame_counter_(0u),
	elapsed_milli_seconds_(0.0f),
	next_elapsed_milli_seconds_(0.0f),
	target_frame_rate_(60),
	enabled_vsync_(true),
	studio_mode_(nox::os::ContainsCommandLineArgKey(u"--studio")),
	kill_(false),
	structural_change_state_(0u),
	out_of_node_command_buffer_(),
	node_command_buffers_(nullptr),
	node_command_buffer_count_(0u),
	archetypes_(),
	entity_systems_(),
	entity_logic_storages_(),
	updater_graph_(),
	job_system_(),
	updater_worker_count_(nox::ResolveUpdaterWorkerCount(
		nox::os::GetCommandLineArgList(),
		nox::JobSystem::GetDefaultWorkerCount())),
	exit_after_frames_(nox::ResolveExitAfterFrames(nox::os::GetCommandLineArgList())),
	services_(),
	service_initialize_order_{},
	initialized_service_count_(0u),
	services_initialized_(false),
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
	//	ノードを触るものを片付ける前に、必ずワーカーを止めて回収する。
	job_system_.Finalize();

	//	Exitを通らなかった場合(テストや起動途中の破棄)も、初期化済みのServiceは逆順に終了させてから解放する。
	//	Exitで終了済みなら何もしない。
	ShutdownServices();

	//	ワーカーが止まった後なら、記録先が消えても誰も触らない。
	delete[] node_command_buffers_;
	node_command_buffers_ = nullptr;
	node_command_buffer_count_ = 0u;

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

	//	Serviceは全モジュールの登録が揃ってから、Depends の順にまとめて初期化する(登録順は使わない)。
	//	Systemの生成より前に済ませる。依存の誤りや OnInitialize の失敗は、ここで理由を残して起動を止める。
	for (const nox::EngineModule* const module : modules_)
	{
		module->RegisterServices(*this);
	}
	InitializeServices();

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

	//	属性付きメソッドを持つService型のうち、このWorldに登録されているものだけをノードにする。
	//	登録されていない型(テスト用の型など)は呼ぶ実体が無いので載せない。
	//	組はグラフの構築にだけ使う(ノードはServiceと記述子を直接指すので、この配列より長く生きなくてよい)。
	nox::FixedVector<nox::UpdaterServiceBinding, k_max_service_count> service_bindings;
	for (const nox::ServiceMethodTypeDescriptor* const descriptor : nox::GetServiceMethodTypes())
	{
		nox::Service* const service = TryGetService(*descriptor->type);
		if (service == nullptr)
		{
			continue;
		}
		service_bindings.PushBack(nox::UpdaterServiceBinding{ .service = service, .descriptor = descriptor });

#if !NOX_MASTER
		NOX_INFO_LINE(nox::log_id::CoreCommon, u8"Serviceメソッド購読: {0}", descriptor->name);
#endif // !NOX_MASTER
	}

	const std::span<const nox::UpdaterTaskDescriptor* const> tasks = nox::GetUpdaterTaskDescriptors();
#if !NOX_MASTER
	for (const nox::UpdaterTaskDescriptor* const task : tasks)
	{
		NOX_INFO_LINE(nox::log_id::CoreCommon, u8"Task購読: {0}", task->name);
	}
#endif // !NOX_MASTER

	//	ノード(EntitySystem / EntityLogic / Serviceのメソッド / Task)の集合が確定してからUpdaterGraphを組む。
	//	以降この集合が変わったら Rebuild を呼び直すこと。
	//	渡す並び(= 生成器の走査順)は結果に影響しない。順序は明示辺(RunAfter / RunBefore)と型名(Taskは関数名)だけで決まる。
	//	明示辺の名前が解決できない・循環する場合は、Rebuild が理由をログに出して起動を止める。
	updater_graph_.Rebuild(
		std::span<nox::EntitySystemBase* const>(entity_systems_.data(), entity_systems_.size()),
		std::span<nox::EntityLogicStorage* const>(entity_logic_storages_.data(), entity_logic_storages_.size()),
		std::span<const nox::UpdaterServiceBinding>(service_bindings.GetStorage().data(), service_bindings.GetLength()),
		tasks);

	//	遅延構造変更の記録先をノード単位に分ける。
	//
	//	確保するのは「nox::EntityCommands& を宣言したノード」のぶんだけでよい。
	//	宣言していないノードは1コマンドも積めないことが引数リストから分かるので、
	//	そこへ空バッファを割り当てるのは丸ごと無駄になる。
	//	依存解析を駆動しているのと同じシグネチャ解析を、確保にもそのまま使っている。
	//
	//	バッファ番号はフェーズ内で一意なので、必要な本数は
	//	「フェーズごとの本数の最大値」で足りる(フェーズ同士は同時に走らない)。
	//	確保はここ1回だけ。以降フレーム中には走らない。
	{
		nox::uint32 max_buffer_count = 0u;
		for (nox::uint8 phase_index = 0u; phase_index < nox::util::ToUnderlying(nox::SystemPhaseType::_Max); ++phase_index)
		{
			const nox::SystemPhaseType phase_type = static_cast<nox::SystemPhaseType>(phase_index);
			max_buffer_count = std::max(max_buffer_count, updater_graph_.GetCommandBufferCount(phase_type));
		}
		ReserveNodeEntityCommandBuffers(max_buffer_count);

		//	引数の数は既存のログ行と揃えてある。新しい引数個数で NOX_INFO_LINE を実体化すると、
		//	kernel/string_format.h 側の既存警告(-Wmissing-braces)がその実体化ぶんだけ増えるため。
		//	フェーズごとのノード数との対比は nox::UpdaterGraph::Trace() が出す。
		NOX_INFO_LINE(nox::log_id::CoreCommon,
			u8"EntityCommandBuffer: {0}本 x コマンド{1}件 = 合計{2}B",
			node_command_buffer_count_,
			k_entity_command_capacity,
			static_cast<nox::uint32>(node_command_buffer_count_ * sizeof(nox::World::EntityCommandBufferType)));
	}

#if !NOX_MASTER
	//	Rebuildまで来ればノードが宣言したComponentData型は全て登録済みなので、ここで名前を配れる。
	SetupExecuteCheckerNames();
	TraceExecuteNodeList();
	updater_graph_.Trace();
#endif // !NOX_MASTER

	//	ワーカー0本ならDispatchは呼び出しスレッド上で回る。決め方はResolveUpdaterWorkerCountを参照。
	job_system_.Initialize(updater_worker_count_);

	NOX_INFO_LINE(nox::log_id::CoreCommon,
		u8"UpdaterGraph実行モード: {0} ワーカー数={1} 論理プロセッサ数={2}",
		(updater_worker_count_ == 0u) ? u8"直列" : u8"並列",
		job_system_.GetWorkerCount(),
		nox::os::GetLogicalProcessorCount());
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

	//	外部からの取り込み(ソケット受信など)を先に済ませる。ExecutePhase の末尾で EntityCommands が
	//	反映されるので、FrameIngress で積んだ構造変更は Update から見える。
	ExecutePhase(nox::SystemPhaseType::FrameIngress);
	ExecutePhase(nox::SystemPhaseType::Update);
	++frame_counter_;

	next_elapsed_milli_seconds_ += (1000.0f / static_cast<nox::float_t>(target_frame_rate_));
	stop_watch_.Restart();
}

void nox::World::Exit()
{
	kill_.store(true, std::memory_order_release);
	system_map_.clear();

	//	フェーズは全て止まっている。初期化と逆順に終了させる。
	ShutdownServices();

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

		//	依存先が同じフェーズ種別のノードとして登録されていなければ起動を止める。
		//	読み飛ばすと、宣言した順序が守られていないことに誰も気付けない。
		//	見つからない原因は「依存先のSystemが生成されていない」「依存先のフェーズが
		//	そのSystemのPhaseRegisterに載っていない」「フェーズ種別が違う」のいずれか。
		static constexpr std::u8string_view k_unresolved_reason =
			u8"依存先のフェーズが同じフェーズ種別のノードとして登録されていません"
			u8"(Systemの未生成 / PhaseRegisterへの登録漏れ / フェーズ種別の不一致)";

		nox::Vector<nox::Vector<nox::uint32>> dependency_indices(nodes.size());
		for (nox::uint32 i = 0; i < nodes.size(); ++i)
		{
			for (const std::reference_wrapper<const nox::SystemBase::SystemPhase>& dep_ref : nodes[i].dependencies)
			{
				const auto it = phase_to_index.find(&dep_ref.get());
				if (it == phase_to_index.end())
				{
					abort_on_system_phase_dependency_error(
						k_unresolved_reason,
						nodes[i].phase.get().name,
						dep_ref.get().name);
				}
				dependency_indices[i].push_back(it->second);
			}

			for (const std::reference_wrapper<const nox::SystemBase::SystemPhase>& depended_ref : nodes[i].depended)
			{
				const auto it = phase_to_index.find(&depended_ref.get());
				if (it == phase_to_index.end())
				{
					abort_on_system_phase_dependency_error(
						k_unresolved_reason,
						nodes[i].phase.get().name,
						depended_ref.get().name);
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

			states[node_index] = 1;
			nox::uint32 max_dependency_layer = 0;
			for (const nox::uint32 dependency_index : dependency_indices[node_index])
			{
				//	訪問中(=再帰の途中)のノードへ戻ってきたら循環。
				//	アサートだけ出して続行すると、閉路上のレイヤー番号が未確定のまま使われる。
				if (states[dependency_index] == 1)
				{
					abort_on_system_phase_dependency_error(
						u8"Phase依存に循環があります",
						nodes[node_index].phase.get().name,
						nodes[dependency_index].phase.get().name);
				}
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

				dest.push_back(SystemExecuteNode{ nodes[i].instance, nodes[i].phase, layer_index });
			}
		}
	}
}

void nox::World::ExecutePhase(const nox::SystemPhaseType phase_type)
{
	const nox::Vector<SystemExecuteNode>& layers = system_phase_table_[nox::util::ToUnderlying(phase_type)];
	//	フェーズ実行中は即時系の構造変更を禁じる。列挙深度(下位ビット)には触れない。
	structural_change_state_.fetch_or(k_structural_change_phase_bit, std::memory_order_seq_cst);
	for (const SystemExecuteNode& layer : layers)
	{
		std::invoke(layer.phase.get().func, &layer.instance.get(), *this);
	}

	ExecuteUpdaterGraphPhase(phase_type);

	//	将来の並列ディスパッチでは、このplaybackポイントまでに全Systemジョブをjoinする必要がある。
	//	ここに来た時点で列挙は全て閉じているので、構造を動かしてよい。
	FlushEntityCommands();
	structural_change_state_.fetch_and(~k_structural_change_phase_bit, std::memory_order_seq_cst);
}

void nox::World::CreateEntitySystems()
{
	for (const nox::EntitySystemTypeDescriptor* const descriptor : nox::GetEntitySystemTypes())
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

#if !NOX_MASTER
		//	「ヘッダに定義しただけで購読される」ことを起動ログで確認できるようにする。
		NOX_INFO_LINE(nox::log_id::CoreCommon, u8"EntitySystem購読: {0}", descriptor->name);
#endif // !NOX_MASTER
	}
}

void nox::World::ExecuteNodeOfLayer(void* const context, const nox::UpdaterNode& node)
{
	static_cast<nox::World*>(context)->ExecuteNode(node);
}

void nox::World::ExecuteEntitySystemChunkJob(void* const context)
{
	auto* const job_context = static_cast<nox::World::ChunkJobContext*>(context);
	//	ジョブを引いたのが配り元とは別のワーカーでも、記録先は配り元のノードのままでなければならない。
	const nox::WorldNodeCommandScope command_scope(*job_context->world, job_context->node_index);
	job_context->system->ExecuteChunk(*job_context->world, *job_context->archetype, job_context->chunk_index);
}

void nox::World::ExecuteEntitySystemParallel(nox::EntitySystemBase& system, const nox::uint32 node_index)
{
	//	Chunkは互いに素なメモリブロックなので、2つのワーカーが同じバイトへ触ることはない。
	//	ノード同士の排他は呼び出し元(ExecuteNode)が既に取っている。
	//	確保は一切走らない。ジョブ配列もChunk参照配列もスタック上の固定長で、
	//	上限を超えるChunk数は同じ配列を使い回すバッチへ分けて配る。
	const nox::EntityQuery& query = system.GetQuery();
	const nox::uint32 total_chunk_count = query.GetTotalChunkCount();

	//	1つ以下なら配っても往復コストが乗るだけなので、その場で回す。
	if (total_chunk_count <= 1u || job_system_.GetWorkerCount() == 0u)
	{
		system.Execute(*this);
		return;
	}

	std::array<nox::EntityChunkRef, k_max_chunk_jobs_per_dispatch> chunk_refs{};
	std::array<nox::World::ChunkJobContext, k_max_chunk_jobs_per_dispatch> job_contexts{};
	std::array<nox::Job, k_max_chunk_jobs_per_dispatch> jobs{};

	for (nox::uint32 start = 0u; start < total_chunk_count; start += k_max_chunk_jobs_per_dispatch)
	{
		const nox::uint32 job_count = query.FillChunkRefs(
			start, std::span<nox::EntityChunkRef>(chunk_refs));
		if (job_count == 0u)
		{
			break;
		}

		for (nox::uint32 index = 0u; index < job_count; ++index)
		{
			job_contexts[index] = nox::World::ChunkJobContext{
				.world = this,
				.system = &system,
				.archetype = chunk_refs[index].archetype,
				.chunk_index = chunk_refs[index].chunk_index,
				.node_index = node_index,
			};
			jobs[index] = nox::Job{
				.func = &nox::World::ExecuteEntitySystemChunkJob,
				.context = &job_contexts[index],
			};
		}

		nox::JobCounter counter{ 0u };
		job_system_.Dispatch(std::span<const nox::Job>(jobs.data(), job_count), counter);
		//	待つ側(配った本人)も自分でジョブを引いて働く。
		job_system_.Wait(counter);
	}
}

void nox::World::ExecuteUpdaterGraphPhase(const nox::SystemPhaseType phase_type)
{
	//	レイヤーは「小さいほど先」。同一レイヤー内のノードは依存解析上互いに衝突しないので、
	//	そのままワーカーへ配ってよい。レイヤー間は直列のまま(次のレイヤーは前のレイヤーの完了が前提)。
	//	配り方(直列へ落とす条件・main_thread_only の扱い・上限超えの扱い)は nox::ExecuteUpdaterLayer に集約してある。
	//	確保は一切走らない。ノード列もレイヤー境界もInitで構築済みで、ジョブ配列はスタック上の固定長。
	const nox::uint32 layer_count = updater_graph_.GetLayerCount(phase_type);
	for (nox::uint32 layer_index = 0u; layer_index < layer_count; ++layer_index)
	{
		nox::ExecuteUpdaterLayer(
			job_system_,
			updater_graph_.GetLayerNodes(phase_type, layer_index),
			&nox::World::ExecuteNodeOfLayer,
			this);
	}
}

void nox::World::ExecuteNode(const nox::UpdaterNode& node)
{
	//	このノードが出す遅延構造変更の記録先を束ねる。Playbackはバッファ番号順に回るので、
	//	どのワーカーが先に走ったかはPlaybackの順序に影響しない。
	//	System / EntityLogic の書き手には何の記述も増えない(束縛はここで完結する)。
	//	構造変更を出さないノードには記録先が無く、番号は無効値のまま渡る。
	const nox::WorldNodeCommandScope command_scope(*this, node.command_buffer_index);

#if !NOX_MASTER
	//	宣言したComponentData / Serviceを実行中だけ占有する。直列実行では決して発火しない。
	//
	//	【stage 2cでのチェッカーの意味】
	//	スコープはノード単位で「配る側のスレッド」が1回だけ取る。Chunkジョブの中では取り直さない。
	//	RWチェッカーはWriteを取ったスレッドIDを覚える方式なので、もし各Chunkジョブが取り直すと
	//	「同一ノードの並列Chunk同士」が互いに違反として誤検出されてしまうため。
	//	結果として、チェッカーが依然として証明するのは
	//	  ・ノード対ノード(レイヤー内並列)の宣言違反 … 従来どおり検出できる
	//	  ・ノードが宣言していないComponentData / Serviceへの、他ノードからの同時アクセス … 検出できる
	//	証明しなくなったのは
	//	  ・1つのノードの内部、Chunkジョブ同士の競合 … 検出できない
	//	    (Chunkが互いに素なメモリであることと、k_parallel_for_eachを宣言したSystemが
	//	     entity間で共有される状態に触れないこと、の2点で担保する。後者はSystem作者の責務。
	//	     nox::IsParallelForEachEntitySystem のコメントに条件を明記してある)
	EnterNodeAccessScope(node.access);
#endif // !NOX_MASTER

	switch (node.kind)
	{
	case nox::UpdaterNodeKind::EntitySystem:
		//	Chunk並列を宣言したSystemだけ、自分の列挙をさらにワーカーへ配る。
		//	--serial-updater はワーカー数0なので、この判定で自動的に直列へ落ちる。
		if (node.system->GetDescriptor().parallel_for_each && job_system_.GetWorkerCount() != 0u)
		{
			ExecuteEntitySystemParallel(*node.system, node.command_buffer_index);
		}
		else
		{
			node.system->Execute(*this);
		}
		break;

	case nox::UpdaterNodeKind::EntityLogicMethod:
		for (const nox::EntityLogicStorage::Entry& entry : node.storage->GetEntries())
		{
			const auto* const entity_record = TryGetEntityRecord(entry.entity.index);
			if (entity_record == nullptr || entity_record->archetype == nullptr)
			{
				continue;
			}
			node.method->invoke(entry.instance, *this, *entity_record->archetype, entity_record->location, entry.entity);
		}
		break;

	case nox::UpdaterNodeKind::ServiceMethod:
		//	entityを列挙しないので、entityの数にかかわらず1回だけ呼ぶ。
		//	自分自身への書き込みは宣言(node.access.service_accesses)の末尾に入っているので、
		//	上の並列実行チェッカーは、このServiceへ宣言外に同時アクセスしたノードも拾える。
		node.service_method->invoke(*node.service, *this);
		break;

	case nox::UpdaterNodeKind::Task:
		//	Taskは状態を持たない。1回だけ呼ぶ。
		node.task->invoke(*this);
		break;

	default:
		NOX_ASSERT(false, u8"未知のUpdaterNodeKindです");
		break;
	}

#if !NOX_MASTER
	LeaveNodeAccessScope(node.access);
#endif // !NOX_MASTER
}

void nox::World::CreateEntityLogicStorages()
{
	for (const nox::EntityLogicTypeDescriptor* const descriptor : nox::GetEntityLogicTypes())
	{
#if !NOX_MASTER
		//	必須ComponentDataはメソッド群から導出されるので包含関係は自動的に成り立つ。
		//	空になるのは「ComponentDataを1つも宣言していない」場合で、全entityに付いてしまうため誤りとみなす。
		NOX_ASSERT(descriptor->make_required_mask().IsEmpty() == false,
			u8"EntityLogicがComponentDataを1つも宣言していません: {0}", descriptor->name);
#endif // !NOX_MASTER

		entity_logic_storages_.push_back(new nox::EntityLogicStorage(*descriptor));

#if !NOX_MASTER
		NOX_INFO_LINE(nox::log_id::CoreCommon, u8"EntityLogic購読: {0}", descriptor->name);
#endif // !NOX_MASTER
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
		const nox::Vector<SystemExecuteNode>& execute_nodes = system_phase_table_[phase_type_index];
		for (const SystemExecuteNode& execute_node : execute_nodes)
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

nox::util::RWParallelExecuteChecker* nox::World::TryGetServiceExecuteChecker(const nox::reflection::Type* const type)noexcept
{
	if (type == nullptr)
	{
		return nullptr;
	}

	//	Serviceの登録数は上限64なので線形走査で足りる。確保は走らない。
	for (nox::uint32 service_index = 0u; service_index < services_.GetLength(); ++service_index)
	{
		if (services_.GetStorage()[service_index].type == type)
		{
			return &service_execute_checkers_[service_index];
		}
	}

	//	未登録のServiceは実体がないので守る対象もない。
	return nullptr;
}

void nox::World::SetupExecuteCheckerNames()noexcept
{
	//	ComponentDataは登録順の密なインデックスなので、未登録に当たった時点で以降も未登録。
	for (nox::uint32 index = 0u; index < nox::k_max_component_type_count; ++index)
	{
		const nox::ComponentTypeInfo* const type_info =
			nox::detail::TryGetComponentTypeInfo(static_cast<nox::ComponentTypeIndex>(index));
		if (type_info == nullptr)
		{
			break;
		}
		component_execute_checkers_[index].SetName(type_info->name);
	}

	for (nox::uint32 index = 0u; index < services_.GetLength(); ++index)
	{
		service_execute_checkers_[index].SetName(services_.GetStorage()[index].type->GetTypeName());
	}
}

void nox::World::EnterNodeAccessScope(const nox::UpdaterNodeAccess& access)noexcept
{
	//	宣言のうち「書き込みが含まれるもの」だけWriteで入る。読み取りだけならReadなので、
	//	同じComponentDataを読むノード同士は並列に走ってもチェッカーは沈黙する。
	const std::source_location location = std::source_location::current();
	access.read_write_mask.ForEachIndex([this, &access, &location](const nox::ComponentTypeIndex type_index)noexcept
		{
			if (access.write_mask.Test(type_index))
			{
				component_execute_checkers_[type_index].EnterWrite(
					nox::util::detail::ParallelExecuteCheckOption::SourceLocation, location);
			}
			else
			{
				component_execute_checkers_[type_index].EnterRead(
					nox::util::detail::ParallelExecuteCheckOption::SourceLocation, location);
			}
		});

	for (nox::uint32 index = 0u; index < access.service_accesses.size(); ++index)
	{
		if (is_duplicated_service_access(access.service_accesses, index))
		{
			continue;
		}

		nox::util::RWParallelExecuteChecker* const checker =
			TryGetServiceExecuteChecker(access.service_accesses[index].type);
		if (checker == nullptr)
		{
			continue;
		}

		if (is_service_write_access(access.service_accesses, index))
		{
			checker->EnterWrite(nox::util::detail::ParallelExecuteCheckOption::SourceLocation, location);
		}
		else
		{
			checker->EnterRead(nox::util::detail::ParallelExecuteCheckOption::SourceLocation, location);
		}
	}
}

void nox::World::LeaveNodeAccessScope(const nox::UpdaterNodeAccess& access)noexcept
{
	//	Enterと完全に対でなければならない。判定条件はEnterと同じものを使う。
	access.read_write_mask.ForEachIndex([this, &access](const nox::ComponentTypeIndex type_index)noexcept
		{
			if (access.write_mask.Test(type_index))
			{
				component_execute_checkers_[type_index].ExitWrite();
			}
			else
			{
				component_execute_checkers_[type_index].ExitRead();
			}
		});

	for (nox::uint32 index = 0u; index < access.service_accesses.size(); ++index)
	{
		if (is_duplicated_service_access(access.service_accesses, index))
		{
			continue;
		}

		nox::util::RWParallelExecuteChecker* const checker =
			TryGetServiceExecuteChecker(access.service_accesses[index].type);
		if (checker == nullptr)
		{
			continue;
		}

		if (is_service_write_access(access.service_accesses, index))
		{
			checker->ExitWrite();
		}
		else
		{
			checker->ExitRead();
		}
	}
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
		for (const SystemExecuteNode& node : layers)
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
		//	ここは唯一フレーム中に確保が走りうる経路。1024体ごとに1回、
		//	entity数が増えている間だけなので償却はされるが、フェーズ実行中に踏むと
		//	ワーカースレッド上での確保になり、そのフレームだけレイテンシが伸びる。
		//	黙って払うと気づけないので、踏んだことが分かるようにしておく。
		//	潰すならロード時に必要ぶんを先に確保する口が要る。
		NOX_ASSERT(IsExecutingSystemPhase() == false,
			u8"フェーズ実行中にEntityRecordPageを確保しました index={0} page={1}", index, page_index);

		//	OSロックを持たずに公開する。CASに負けた側は自分のページを捨てて勝者のものを使う。
		//	std::mutexを使うと、ジョブから呼ばれたときにワーカースレッドがOS待ちに入る。
		auto* const created_page = new EntityRecordPage();
		nox::World::EntityRecordPage* expected = nullptr;
		if (entity_record_pages_[page_index].compare_exchange_strong(
			expected, created_page, std::memory_order_acq_rel, std::memory_order_acquire) == true)
		{
			entity_record_page = created_page;
		}
		else
		{
			delete created_page;
			entity_record_page = expected;
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

nox::StructuralChangePermission nox::World::GetStructuralChangePermission()const noexcept
{
	const nox::uint32 state = structural_change_state_.load(std::memory_order_seq_cst);
	if (state == 0u)
	{
		return nox::StructuralChangePermission::Allowed;
	}

	//	列挙のほうが具体的な理由なので優先して返す。
	if ((state & k_structural_change_iteration_mask) != 0u)
	{
		return nox::StructuralChangePermission::DeniedDuringIteration;
	}
	return nox::StructuralChangePermission::DeniedDuringPhase;
}

bool nox::World::EnsureImmediateStructuralChangeAllowed()const noexcept
{
	const nox::StructuralChangePermission permission = GetStructuralChangePermission();
	NOX_ASSERT(permission != nox::StructuralChangePermission::DeniedDuringPhase,
		u8"フェーズ実行中に即時系の構造変更は呼べません。nox::EntityCommands(遅延系)を使用してください");
	NOX_ASSERT(permission != nox::StructuralChangePermission::DeniedDuringIteration,
		u8"entityの列挙中に即時系の構造変更は呼べません。nox::EntityCommands(遅延系)を使用してください");
	return permission == nox::StructuralChangePermission::Allowed;
}

bool nox::World::EnsureDeferredStructuralChangeAllowed()const noexcept
{
	//	遅延系は即時系のちょうど裏返し。反映点(Playback)が来ることが前提なので、
	//	フェーズ実行中でも列挙中でもないときに積むのは記録漏れの兆候として弾く。
	const bool allowed = (structural_change_state_.load(std::memory_order_seq_cst) != 0u);
	NOX_ASSERT(allowed, u8"遅延系の構造変更は、フェーズ実行中または列挙中にのみ記録できます");
	return allowed;
}

void nox::World::EnterEntityIteration()noexcept
{
	//	acq_rel。入場を他スレッドのacquireロードへ見せるにはreleaseが要り(acquireのRMWでは
	//	カウンタ増加がhappens-beforeしない)、退場との対でacquireも要る。
	//	なおseq_cstにはしていない。理由は world.h の structural_change_state_ の注記を参照。
	structural_change_state_.fetch_add(1u, std::memory_order_seq_cst);
}

void nox::World::LeaveEntityIteration()noexcept
{
	NOX_ASSERT(IsIteratingEntities(), u8"対応するEnterEntityIterationがありません");
	structural_change_state_.fetch_sub(1u, std::memory_order_seq_cst);
}

void nox::detail::EnterEntityIterationOfWorld(nox::World& world)noexcept
{
	world.EnterEntityIteration();
}

void nox::detail::LeaveEntityIterationOfWorld(nox::World& world)noexcept
{
	world.LeaveEntityIteration();
}

nox::EntityId nox::World::CreateEntity()
{
	if (EnsureImmediateStructuralChangeAllowed() == false)
	{
		return nox::EntityId{ 0u };
	}

	return CreateEntityImmediate();
}

nox::EntityId nox::World::CreateEntityDuringPhase()noexcept
{
	if (EnsureDeferredStructuralChangeAllowed() == false)
	{
		return nox::EntityId{ 0u };
	}

	//	Idの払い出しはEntityRecord 1件で完結し、Archetypeにも他entityの行にも触れない。
	//	だから列挙中でもその場で返してよい(遅延させる必要があるのは構造の変更だけ)。
	//
	//	【既知の残課題: 払い出されるIdの「値」は決定的ではない】
	//	コマンドバッファをノード単位に分けたことで、Playbackの順序と、その結果できる
	//	Archetypeの行の並び・ComponentDataの値は決定的になった。
	//	しかしId自体は共通のフリーリスト/連番から取るため、レイヤー内の複数ノードが
	//	並列にCreateすると、どのノードがどの番号を取るかはrunごとに入れ替わる。
	//	つまり「どの行にどのId番号が載るか」は再現しない。
	//	Idをhash・乱数seed・シリアライズのキーに使うと、そこは再現しないことになる。
	//	潰すにはノード単位のIdレンジ払い出し(各ノードが自分の区間から取る)が要る。
	return CreateEntityImmediate();
}

nox::EntityId nox::World::CreateEntityImmediate()
{
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
	if (EnsureImmediateStructuralChangeAllowed() == false)
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

bool nox::World::IsEntityGenerationLive(const nox::EntityId entity)const noexcept
{
	const auto* const entity_record = TryGetEntityRecord(entity.index);
	return entity_record != nullptr &&
		entity_record->generation.load(std::memory_order_acquire) == entity.generation;
}

void nox::World::ReserveNodeEntityCommandBuffers(const nox::uint32 node_count)
{
	NOX_ASSERT(GetStructuralChangePermission() == nox::StructuralChangePermission::Allowed,
		u8"フェーズ実行中・列挙中にコマンドバッファを確保し直すことはできません");
	if (node_count <= node_command_buffer_count_)
	{
		//	既に足りている。確保し直すとhigh-water markまで失われる。
		return;
	}

	//	ノード集合はInitで確定して以降動かないので、ここは実質1回きり。
	//	既存分を作り直すことになるが、Playback前の状態でしか呼べないため取りこぼしは起きない。
	delete[] node_command_buffers_;
	node_command_buffers_ = new nox::World::EntityCommandBufferType[node_count];
	node_command_buffer_count_ = node_count;
}

nox::World::EntityCommandBufferType& nox::World::GetCurrentEntityCommandBuffer()noexcept
{
	const nox::NodeCommandBinding binding = nox::t_node_command_binding;
	if (binding.world != this)
	{
		//	ノードに束縛されていない経路(旧SystemPhase / リフレクション経由 / ツール・テストの自前列挙)。
		//	いずれも並列ディスパッチの外なので、この1本の中でも順序は決定的になる。
		return out_of_node_command_buffer_;
	}

	if (binding.node_index >= node_command_buffer_count_)
	{
		//	nox::EntityCommands& を宣言していないノードが、宣言の外側から遅延構造変更を出している。
		//	(例: World参照を握ったServiceがQueue系を直接呼ぶ)
		//	このノードには記録先が無いのでノード外バッファへ落ちるが、そこは並列実行の
		//	前提が置けないため、2つのノードが同時にやると順序が決まらない。
		//	宣言と実装の食い違いなので、開発中に気づけるようにしておく。
		NOX_ASSERT(false,
			u8"nox::EntityCommands& を宣言していないノードが遅延構造変更を記録しました。"
			u8"引数リストに nox::EntityCommands& を宣言してください");
		return out_of_node_command_buffer_;
	}
	return node_command_buffers_[binding.node_index];
}

nox::uint32 nox::World::GetEntityCommandPeakLength()const noexcept
{
	nox::uint32 peak = out_of_node_command_buffer_.GetPeakLength();
	for (nox::uint32 index = 0u; index < node_command_buffer_count_; ++index)
	{
		peak = std::max(peak, node_command_buffers_[index].GetPeakLength());
	}
	return peak;
}

nox::uint32 nox::World::GetEntityCommandPeakPayloadLength()const noexcept
{
	nox::uint32 peak = out_of_node_command_buffer_.GetPeakPayloadLength();
	for (nox::uint32 index = 0u; index < node_command_buffer_count_; ++index)
	{
		peak = std::max(peak, node_command_buffers_[index].GetPeakPayloadLength());
	}
	return peak;
}

nox::uint32 nox::World::GetNodeEntityCommandPeakLength(const nox::uint32 node_index)const noexcept
{
	if (node_index >= node_command_buffer_count_)
	{
		return 0u;
	}
	return node_command_buffers_[node_index].GetPeakLength();
}

nox::uint32 nox::World::GetNodeEntityCommandPeakPayloadLength(const nox::uint32 node_index)const noexcept
{
	if (node_index >= node_command_buffer_count_)
	{
		return 0u;
	}
	return node_command_buffers_[node_index].GetPeakPayloadLength();
}

nox::uint32 nox::World::GetOutOfNodeEntityCommandPeakLength()const noexcept
{
	return out_of_node_command_buffer_.GetPeakLength();
}

void nox::World::AbortOnEntityCommandOverflow(const nox::World::EntityCommandBufferType& buffer)noexcept
{
	NOX_ASSERT(false,
		u8"EntityCommandBuffer capacity exceeded: commands={0}/{1}, payload={2}/{3}",
		buffer.GetLength(),
		k_entity_command_capacity,
		buffer.GetPayloadLength(),
		k_entity_command_payload_bytes);

	//	Masterでは NOX_ASSERT もログも消えるため、素の std::abort() だと理由が何も残らない。
	//	volatile なローカルはスタック上に必ず実体化されるので、クラッシュダンプから読める。
	//	確保もグローバル変数も増やさずに、落ちた理由だけを持っていける。
	volatile const nox::uint32 overflow_command_length = buffer.GetLength();
	volatile const nox::uint32 overflow_command_capacity = k_entity_command_capacity;
	volatile const nox::uint32 overflow_payload_length = buffer.GetPayloadLength();
	volatile const nox::uint32 overflow_payload_capacity = k_entity_command_payload_bytes;
	//	どのノードのバッファで溢れたのかもダンプから読めるようにする。
	//	番号は nox::UpdaterNode::command_buffer_index で、起動ログのUpdaterGraphの cb=<番号> と一致する。
	volatile const nox::uint32 overflow_node_index =
		(&buffer == &out_of_node_command_buffer_)
		? std::numeric_limits<nox::uint32>::max()
		: static_cast<nox::uint32>(&buffer - node_command_buffers_);
	(void)overflow_command_length;
	(void)overflow_command_capacity;
	(void)overflow_payload_length;
	(void)overflow_payload_capacity;
	(void)overflow_node_index;

	std::abort();
}

bool nox::World::QueueDestroyEntity(const nox::EntityId entity)noexcept
{
	if (EnsureDeferredStructuralChangeAllowed() == false)
	{
		return false;
	}

	nox::World::EntityCommandBufferType& buffer = GetCurrentEntityCommandBuffer();
	const bool queued = buffer.TryDestroy(entity);
	if (queued == false)
	{
		AbortOnEntityCommandOverflow(buffer);
	}
	return queued;
}

bool nox::World::QueueAddComponent(
	const nox::EntityId entity,
	const nox::ComponentTypeInfo& type_info,
	const void* const source)noexcept
{
	if (EnsureDeferredStructuralChangeAllowed() == false)
	{
		return false;
	}

	nox::World::EntityCommandBufferType& buffer = GetCurrentEntityCommandBuffer();
	const bool queued = buffer.TryAddComponent(entity, type_info, source);
	//	コマンド枠かペイロード枠のどちらかが尽きている。構造変更を黙って落とすほうが後で困るので、即座に落とす。
	if (queued == false)
	{
		AbortOnEntityCommandOverflow(buffer);
	}
	return queued;
}

bool nox::World::QueueRemoveComponent(
	const nox::EntityId entity,
	const nox::ComponentTypeInfo& type_info)noexcept
{
	if (EnsureDeferredStructuralChangeAllowed() == false)
	{
		return false;
	}

	nox::World::EntityCommandBufferType& buffer = GetCurrentEntityCommandBuffer();
	const bool queued = buffer.TryRemoveComponent(entity, type_info);
	if (queued == false)
	{
		AbortOnEntityCommandOverflow(buffer);
	}
	return queued;
}

void nox::World::FlushEntityCommands()noexcept
{
	//	Playbackは実データを動かすので、列挙が1つでも開いていたら踏んではならない。
	//	ここでreturnして見送ると、積まれたコマンドが次フェーズ終端まで持ち越される。
	//	それは「遅延生成のIDを即時にする」判断で決定性が壊れるとして退けた繰り越しそのものなので、
	//	異常系として残さない。溢れと同じく即座に落とす(Masterでもアサートが消えるだけで挙動は同じ)。
	NOX_ASSERT(IsIteratingEntities() == false, u8"entityの列挙中にPlaybackはできません");
	if (IsIteratingEntities())
	{
		std::abort();
	}

	//	記録先はノードごとに分かれている。再生は「ノード外 → ノード番号順」の固定順で回す。
	//	ノード番号はUpdaterGraphの全順序(明示辺と衝突辺のトポロジカル順、決まらない箇所は型名順)で
	//	構築時に決まるため、どのワーカーがどのノードを先に走らせたかはここに一切影響しない。
	PlaybackEntityCommandBuffer(out_of_node_command_buffer_);
	for (nox::uint32 node_index = 0u; node_index < node_command_buffer_count_; ++node_index)
	{
		PlaybackEntityCommandBuffer(node_command_buffers_[node_index]);
	}
}

void nox::World::PlaybackEntityCommandBuffer(nox::World::EntityCommandBufferType& buffer)noexcept
{
	buffer.BeginPlayback();
	nox::uint32 command_index = 0u;
	while (command_index < buffer.GetLength())
	{
		nox::EntityCommand command{};
		const bool ready = buffer.TryGet(command_index++, command);
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

		case nox::EntityCommandType::AddComponent:
		{
			NOX_ASSERT(command.type_info != nullptr, u8"AddComponentコマンドに型情報がありません");
			if (command.type_info == nullptr)
			{
				break;
			}

			void* const destination = AddComponentImmediate(nox::EntityId{ command.entity_raw }, *command.type_info);
			const void* const payload = buffer.TryGetPayload(command);
			if (destination != nullptr && payload != nullptr)
			{
				//	ComponentDataは常にtrivially copyableなのでmemcpyで足りる。
				std::memcpy(destination, payload, command.payload_size);
			}
			break;
		}

		case nox::EntityCommandType::RemoveComponent:
			NOX_ASSERT(command.type_info != nullptr, u8"RemoveComponentコマンドに型情報がありません");
			if (command.type_info != nullptr)
			{
				RemoveComponentImmediate(nox::EntityId{ command.entity_raw }, *command.type_info);
			}
			break;

		default:
			NOX_ASSERT(false, u8"Unknown entity command");
			break;
		}
	}
	buffer.Clear();
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
	if (EnsureImmediateStructuralChangeAllowed() == false)
	{
		return nullptr;
	}

	//	stale handleの検出は即時系の入口だけで行う。
	//	AddComponentImmediateはPlaybackとも共通なので、あちらで落とすと
	//	「同一フェーズ内でDestroyされたentityへのAdd」という遅延系では正常な競合まで
	//	巻き込んでしまう(DeferredAddOnEntityDestroyedInSamePhaseIsSkipped を参照)。
	//	Masterでは NOX_ASSERT ごと消えるので、検査用のローカルも一緒に消えるよう
	//	IsEntityGenerationLive() の呼び出しをアサートの中へ畳んでおく
	//	(外に出すと未参照ローカルとしてMasterだけ警告が出る)。
	NOX_ASSERT(IsEntityGenerationLive(entity),
		u8"破棄済みのentityにComponentDataを追加しようとしました: index={0}", entity.index);

	return AddComponentImmediate(entity, type_info);
}

void* nox::World::AddComponentImmediate(const nox::EntityId entity, const nox::ComponentTypeInfo& type_info)
{
	auto* const entity_record = TryGetEntityRecord(entity.index);
	if (entity_record == nullptr || entity_record->generation.load(std::memory_order_acquire) != entity.generation)
	{
		//	Playbackから来た場合、同一フェーズ内で先にDestroyが再生されただけなので黙って捨てる。
		//	RemoveComponentImmediateが昔から同じ扱いをしている。
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
	if (EnsureImmediateStructuralChangeAllowed() == false)
	{
		return;
	}

	RemoveComponentImmediate(entity, type_info);
}

void nox::World::RemoveComponentImmediate(const nox::EntityId entity, const nox::ComponentTypeInfo& type_info)
{
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

void nox::World::RegisterService(const nox::ServiceTypeDescriptor& descriptor, nox::Service& service)
{
	const nox::reflection::Type& type = *descriptor.type;
	NOX_ASSERT(TryGetService(type) == nullptr, u8"Serviceが二重に登録されました: {0}", type.GetTypeName());
	NOX_ASSERT(services_initialized_ == false, u8"Serviceの初期化後に登録しようとしました: {0}", type.GetTypeName());
	services_.PushBack(nox::World::ServiceEntry{ .type = &type, .service = &service, .depends = descriptor.depends });
}

nox::ServiceInitializeResult nox::World::TryInitializeServices()noexcept
{
	//	依存の集合をuint64のビット集合で持つ。確保も再帰も要らない。
	static_assert(k_max_service_count <= 64u, "Serviceの依存をuint64のビット集合で持つため、登録上限は64まで");

	NOX_ASSERT(services_initialized_ == false, u8"Serviceが二重に初期化されようとしました");
	if (services_initialized_ == true)
	{
		return nox::ServiceInitializeResult{};
	}

	const nox::uint32 service_count = services_.GetLength();
	const std::array<nox::World::ServiceEntry, k_max_service_count>& entries = services_.GetStorage();

	//	1. 完全修飾型名順に並べる。決まらない箇所のタイブレークであり、名前での解決にも使う。
	//	   登録順(services_の並び)は結果に影響させない。
	std::array<nox::uint32, k_max_service_count> sorted_indices{};
	for (nox::uint32 index = 0u; index < service_count; ++index)
	{
		sorted_indices[index] = index;
	}
	std::sort(
		sorted_indices.data(),
		sorted_indices.data() + service_count,
		[&entries](const nox::uint32 a, const nox::uint32 b)noexcept
		{
			return entries[a].type->GetTypeName() < entries[b].type->GetTypeName();
		});

	//	整列後の位置で名前とServiceを引ける表。nox::ServiceContext もこの表を引く。
	std::array<nox::detail::ServiceLookupEntry, k_max_service_count> lookup{};
	for (nox::uint32 position = 0u; position < service_count; ++position)
	{
		const nox::World::ServiceEntry& entry = entries[sorted_indices[position]];
		lookup[position] = nox::detail::ServiceLookupEntry{ .type_name = entry.type->GetTypeName(), .service = entry.service };
	}
	const std::span<const nox::detail::ServiceLookupEntry> lookup_view(lookup.data(), service_count);

	//	2. 同名(= 同じ型)が2つあると名前で解決できない。整列済みなので隣だけ見ればよい。
	for (nox::uint32 position = 1u; position < service_count; ++position)
	{
		if (lookup[position].type_name == lookup[position - 1u].type_name)
		{
			const nox::ServiceInitializeResult result{
				.error = nox::ServiceInitializeError::DuplicateService,
				.service_type_name = lookup[position].type_name,
				.related_type_name = lookup[position].type_name,
			};
			log_service_initialize_failure(result);
			return result;
		}
	}

	//	3. Depends を整列後の位置のビット集合へ解決する。見つからない名前は宣言の誤り。
	std::array<nox::uint64, k_max_service_count> predecessor_masks{};
	for (nox::uint32 position = 0u; position < service_count; ++position)
	{
		for (const std::string_view depend_name : entries[sorted_indices[position]].depends)
		{
			const auto found = std::ranges::lower_bound(
				lookup_view, depend_name, std::less<>{}, &nox::detail::ServiceLookupEntry::type_name);
			if ((found == lookup_view.end()) || (found->type_name != depend_name))
			{
				const nox::ServiceInitializeResult result{
					.error = nox::ServiceInitializeError::UnresolvedDependency,
					.service_type_name = lookup[position].type_name,
					.related_type_name = depend_name,
				};
				log_service_initialize_failure(result);
				return result;
			}
			predecessor_masks[position] |= (1ull << static_cast<nox::uint32>(found - lookup_view.begin()));
		}
	}

	//	4. Depends だけでトポロジカル順を作る(Kahn法)。候補が複数あれば整列順の最小(= 型名順)を採る。
	//	   全て置き切れなければ循環している(自分自身を並べた場合も、自分のビットが残り続けるのでここで掛かる)。
	std::array<nox::uint32, k_max_service_count> order_positions{};
	nox::uint64 placed_mask = 0u;
	//	未配置の先行を1つ返す。未配置のServiceは必ず未配置の先行を持つ(循環の報告にだけ使う)。
	const auto find_unplaced_predecessor = [&predecessor_masks, &placed_mask, service_count](const nox::uint32 position)noexcept
		{
			const nox::uint64 unplaced_predecessors = predecessor_masks[position] & ~placed_mask;
			for (nox::uint32 candidate = 0u; candidate < service_count; ++candidate)
			{
				if (((unplaced_predecessors >> candidate) & 1ull) != 0u)
				{
					return candidate;
				}
			}
			return position;
		};

	for (nox::uint32 order = 0u; order < service_count; ++order)
	{
		nox::uint32 picked = service_count;
		for (nox::uint32 position = 0u; position < service_count; ++position)
		{
			const nox::uint64 bit = 1ull << position;
			if (((placed_mask & bit) == 0u) && ((predecessor_masks[position] & ~placed_mask) == 0u))
			{
				picked = position;
				break;
			}
		}

		if (picked == service_count)
		{
			//	先行をService数ぶん遡れば必ず閉路の上に乗る(閉路の下流にあるだけのServiceを報告しないため)。
			nox::uint32 on_cycle = 0u;
			while (((placed_mask >> on_cycle) & 1ull) != 0u)
			{
				++on_cycle;
			}
			for (nox::uint32 step = 0u; step < service_count; ++step)
			{
				on_cycle = find_unplaced_predecessor(on_cycle);
			}

#if !NOX_MASTER
			//	閉路を一周ぶん書き出す。先行の辿り方は決定的なので、必ず on_cycle へ戻ってくる。
			{
				RuntimeGraphTextBuilder builder;
				builder.Append(u8"Serviceの Depends の閉路(後に初期化される側 <- 先に初期化される側)");
				NOX_ERROR_LINE(nox::log_id::CoreCommon, u8"{0}", std::u8string_view(builder.buffer.data(), builder.length));
			}
			nox::uint32 current = on_cycle;
			for (nox::uint32 step = 0u; step < service_count; ++step)
			{
				const nox::uint32 previous = find_unplaced_predecessor(current);
				RuntimeGraphTextBuilder builder;
				builder.Append(u8"  ");
				builder.Append(lookup[current].type_name);
				builder.Append(u8" <- ");
				builder.Append(lookup[previous].type_name);
				NOX_ERROR_LINE(nox::log_id::CoreCommon, u8"{0}", std::u8string_view(builder.buffer.data(), builder.length));
				current = previous;
				if (current == on_cycle)
				{
					break;
				}
			}
#endif // !NOX_MASTER

			const nox::ServiceInitializeResult result{
				.error = nox::ServiceInitializeError::DependencyCycle,
				.service_type_name = lookup[on_cycle].type_name,
				.related_type_name = lookup[find_unplaced_predecessor(on_cycle)].type_name,
			};
			log_service_initialize_failure(result);
			return result;
		}

		placed_mask |= (1ull << picked);
		order_positions[order] = picked;
	}

	//	5. 順に初期化する。ここまで来れば宣言(Depends)は全て成立している。
	//	   OnInitialize が true を返したものだけを「初期化済み」として記録し、失敗時はそれだけを逆順に終了させる。
	for (nox::uint32 order = 0u; order < service_count; ++order)
	{
		const nox::uint32 service_index = sorted_indices[order_positions[order]];
		const nox::World::ServiceEntry& entry = entries[service_index];

		nox::ServiceContext context(entry.depends, lookup_view);
		const bool succeeded = entry.service->Initialize(context);
		if (succeeded == true)
		{
			service_initialize_order_[initialized_service_count_] = service_index;
			++initialized_service_count_;
		}

		//	宣言外の型を引いていたら、OnInitialize が true を返していても失敗にする。
		//	宣言 = 依存解析の唯一の入力であり、宣言外の依存は初期化順が保証されないため。
		const std::string_view undeclared_type_name = context.GetUndeclaredTypeName();
		if ((succeeded == false) || (undeclared_type_name.empty() == false))
		{
			const nox::ServiceInitializeResult result = (undeclared_type_name.empty() == false)
				? nox::ServiceInitializeResult{
					.error = nox::ServiceInitializeError::UndeclaredServiceAccess,
					.service_type_name = entry.type->GetTypeName(),
					.related_type_name = undeclared_type_name,
				}
				: nox::ServiceInitializeResult{
					.error = nox::ServiceInitializeError::InitializeFailed,
					.service_type_name = entry.type->GetTypeName(),
					.related_type_name = std::string_view(),
				};
			//	初期化済みのものだけを逆順に終了させてから理由を残す。
			ShutdownServices();
			log_service_initialize_failure(result);
			return result;
		}

#if !NOX_MASTER
		//	引数は左辺値で渡す(EntitySystem購読 のログ行と同じ実体化に揃え、string_format.h 側の既存警告を増やさない)。
		const std::string_view initialized_type_name = entry.type->GetTypeName();
		NOX_INFO_LINE(nox::log_id::CoreCommon, u8"Service初期化: {0}", initialized_type_name);
#endif // !NOX_MASTER
	}

	services_initialized_ = true;
	return nox::ServiceInitializeResult{};
}

void nox::World::InitializeServices()noexcept
{
	const nox::ServiceInitializeResult result = TryInitializeServices();
	if (result.IsSuccess() == false)
	{
		//	初期化済みのServiceは TryInitializeServices が逆順に終了させ、理由もログに出してある。
		abort_on_service_initialize_failure(result);
	}
}

void nox::World::ShutdownServices()noexcept
{
	//	数を先に減らしてから呼ぶ。OnShutdown の中から何が起きても、同じServiceを2回終了させない。
	while (initialized_service_count_ != 0u)
	{
		--initialized_service_count_;
		services_.GetStorage()[service_initialize_order_[initialized_service_count_]].service->Shutdown();
	}
	services_initialized_ = false;
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
