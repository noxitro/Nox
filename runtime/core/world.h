// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	world.h
/// @brief	world
#pragma once
#include	"system.h"
#include	"entity.h"
#include	"entity_command_buffer.h"
#include	"archetype.h"
#include	"entity_system.h"
#include	"entity_logic.h"
#include	"updater_graph.h"
#include	"service.h"
#include	"../kernel/job_system.h"

namespace nox
{
	struct IComponentData;
	class Component;
	class SystemBase;
	class EngineModule;
	class World;

	/// @brief 即時系の構造変更API(CreateEntity / DestroyEntity / AddComponent / RemoveComponent)を
	///        「今」呼んでよいかどうか。
	/// @details 即時系はArchetype間の物理移動とswap-removeを伴うため、列挙中に呼ぶと
	///          列挙側が握っている列ポインタと行番号が壊れる。
	///
	///          このenumを返す nox::World::GetStructuralChangePermission() が、
	///          即時系4本すべてが実際に分岐している唯一の判定点である。
	///          アサートは「なぜ弾かれたか」を開発者へ伝えるだけで、弾く判断自体はこの値が行う。
	///          そのためテストからはこの値を直接見れば、アサートを発火させずに
	///          制約が働いていることを確認できる。
	enum class StructuralChangePermission : nox::uint8
	{
		/// @brief 即時系を呼んでよい。
		Allowed,
		/// @brief フェーズ(System / EntityLogic)実行中。遅延系(nox::EntityCommands)を使うこと。
		DeniedDuringPhase,
		/// @brief entityの列挙中。列挙が終わるまで構造を変えられない。
		DeniedDuringIteration,
	};

	/// @brief 遅延構造変更の記録先を「今このスレッドが実行しているノード」へ束ねるスコープ。
	/// @details 遅延系のコマンドバッファは「構造変更を出しうるノード」1本ずつあり、
	///          Playbackはその登録順に回る。
	///          記録側(nox::EntityCommands)は引数リストにWorldしか持たないので、
	///          「どのノードのバッファへ積むか」はスレッドローカルな束縛で伝える。
	///          こうしておくと、EntityLogic / EntitySystem の書き手には何の記述も増えない。
	///
	///          入れ子を必ず保存・復元する。ジョブを配った側のスレッドは Wait の内側でも
	///          別ノードのジョブを引いて働くため、単純な set/clear では束縛が壊れる。
	///
	///          エンジン(nox::World::ExecuteNode とChunkジョブのthunk)が使う。
	///          テストからノード実行を模すためにも使えるよう公開してある。
	class WorldNodeCommandScope final
	{
	public:
		/// @brief node_indexは nox::UpdaterNode::command_buffer_index (フェーズ内で一意)。
		/// @details 構造変更を出さないノードは nox::k_invalid_updater_command_buffer_index を渡す。
		///          その状態で記録しようとした場合はアサートで弾かれる(宣言と実装の食い違い)。
		WorldNodeCommandScope(const nox::World& world, nox::uint32 node_index)noexcept;
		~WorldNodeCommandScope()noexcept;

		WorldNodeCommandScope(const WorldNodeCommandScope&) = delete;
		WorldNodeCommandScope& operator=(const WorldNodeCommandScope&) = delete;

	private:
		const nox::World* previous_world_;
		nox::uint32 previous_node_index_;
	};

	/// @brief コマンドライン引数列から、UpdaterGraphを回すワーカー数を決める。
	/// @details 決め方(優先順):
	///            1. --serial-updater      … 0本。Dispatchは呼び出しスレッド上で回る
	///            2. --updater-workers=N   … N本。0を渡せば --serial-updater と同じ直列になる
	///            3. どちらも無い / 値が壊れている … default_worker_count をそのまま返す
	///
	///          「並列化が実際に効いているか」を出荷構成でも測るための入口。
	///          直列(0)と既定の2点しか選べないと、スケールするかどうかが数字で言えない。
	///
	///          既定値を引数で受け取り、コマンドラインの取得(nox::os)にも
	///          nox::JobSystem にも触れない純粋関数にしてある。副作用が無いので
	///          World を組み立てずに引数列を並べるだけで検証できる。
	///
	///          Nを nox::JobSystem::k_max_worker_count で頭打ちにするのは方針ではなく
	///          桁あふれ対策(解析途中の値がuint32を超えないようにする)。
	///          ハードウェアに合わせた最終的な丸めは nox::JobSystem::Initialize が行う。
	/// @param command_line_args nox::os::GetCommandLineArgList() が返す並び。
	/// @param default_worker_count 指定が無いときに使う値。
	[[nodiscard]] nox::uint32 ResolveUpdaterWorkerCount(
		std::span<const nox::char16* const> command_line_args,
		nox::uint32 default_worker_count)noexcept;

	class World final: public nox::Object
	{
		NOX_DECLARE_OBJECT(World, nox::Object);
	private:
		static constexpr nox::uint32 k_entity_record_page_shift = 10u;
		static constexpr nox::uint32 k_entity_record_page_size = 1u << k_entity_record_page_shift;
		static constexpr nox::uint32 k_entity_record_page_mask = k_entity_record_page_size - 1u;
		static constexpr nox::uint32 k_max_entity_page_count = 1u << 14;
		static constexpr nox::uint32 k_max_entity_count = k_entity_record_page_size * k_max_entity_page_count;
		static constexpr nox::uint32 k_invalid_entity_index = std::numeric_limits<nox::uint32>::max();
		static constexpr nox::uint32 k_initial_live_generation = 1u;
		/// @brief コマンドバッファ1本が積める遅延構造変更の上限。
		/// @details 1本 = UpdaterGraphのノード1つ分。フェーズ全体の予算ではない。
		///
		///          弾1発の生成が Create + Add×2〜3 でおよそ3コマンドなので、1024なら
		///          「1つのSystem(またはEntityLogicメソッド)が1フェーズで341発生成できる」に相当する。
		///          60fpsで毎フレーム341発を出し続けるSystemは実在しない規模なので、ここを起点にした。
		///          ノード単位に分ける前は4096だったが、あれは全ノードの合計に対する予算だった。
		///          分割後の1本は1ノード分しか受けないので、同じ数字を持ち回る必要が無い。
		///
		///          既知の限界: 「1つのノードが1フレームで大量に破棄する」形は生成より数が出る。
		///          画面上の全entityを1ノードで一括Destroyするような設計(いわゆるボム)は
		///          entity数がそのままコマンド数になるため、ここを超え得る。
		///          その場合は破棄をフレームに分散させるか、この定数を上げること。
		///
		///          溢れは黙って捨てずに落とす設計なので、出荷前に
		///          GetEntityCommandPeakLength() で実測して裏を取ること。
		static constexpr nox::uint32 k_entity_command_capacity = 1024u;
		/// @brief 遅延Addが運ぶComponentData初期値の総バイト数(コマンドバッファ1本あたり)。
		/// @details 1コマンドあたり64Bを見込む。ComponentDataは実測でTransform相当が最大級なので、
		///          64Bあれば全コマンドがAddでも足りる。
		static constexpr nox::uint32 k_entity_command_payload_bytes = k_entity_command_capacity * 64u;
		/// @brief コマンドバッファ1本の型。ノードごとに1本ずつ持つ。
		using EntityCommandBufferType =
			nox::EntityCommandBuffer<k_entity_command_capacity, k_entity_command_payload_bytes>;
		/// @brief structural_change_state_のビット割り当て。
		/// @details 「フェーズ実行中」と「列挙の入れ子深度」を1ワードに詰めるので、
		///          即時系の判定はatomicロード1回で済む。
		static constexpr nox::uint32 k_structural_change_phase_bit = 0x80000000u;
		static constexpr nox::uint32 k_structural_change_iteration_mask = ~k_structural_change_phase_bit;
		static constexpr nox::uint32 k_max_service_count = 64u;
		static constexpr nox::uint32 k_initial_archetype_capacity = 64u;
		/// @brief 1レイヤーに載せられるノード数の上限。ジョブ配列をスタックに置くために固定する。
		static constexpr nox::uint32 k_max_nodes_per_layer = 256u;
		/// @brief 1回のディスパッチで配れるChunkジョブ数の上限。
		/// @details ジョブ配列をスタック上の固定長で持つための上限。Queryのマッチ数がこれを超える場合は
		///          この単位のバッチへ分けて配る(確保は一切しない)。
		static constexpr nox::uint32 k_max_chunk_jobs_per_dispatch = 256u;

		/// @brief 実行ノード
		struct SystemExecuteNode
		{
			std::reference_wrapper<nox::SystemBase> instance;
			std::reference_wrapper<const nox::SystemBase::SystemPhase> phase;
			nox::uint32 layer_index;	///< 小さいほど先に実行。同一レイヤーは並列実行可能
		};

		struct EntityRecord
		{
			nox::Atomic<nox::uint32> generation;
			nox::Atomic<nox::uint32> next_free_index;
			/// @brief 所属Archetype。ComponentDataを1つも持たない間はnullptr。
			nox::Archetype* archetype;
			nox::ArchetypeLocation location;

			EntityRecord() noexcept :
				generation(0u),
				next_free_index(k_invalid_entity_index),
				archetype(nullptr),
				location(nox::ArchetypeLocation::Invalid())
			{
			}
		};

		struct ServiceEntry
		{
			const nox::reflection::Type* type;
			nox::Service* service;
		};

		struct EntityRecordPage
		{
			std::array<nox::World::EntityRecord, k_entity_record_page_size> records;
		};

		/// @brief ノード1つ分のジョブコンテキスト。ディスパッチ毎にスタック上へ作る。
		/// @details 関数ポインタ + void* しか渡せないので、Worldとノードをここで束ねる。
		struct NodeJobContext
		{
			nox::World* world;
			const nox::UpdaterNode* node;
		};

		/// @brief Chunk1つ分のジョブコンテキスト。ディスパッチ毎にスタック上へ作る。
		struct ChunkJobContext
		{
			nox::World* world;
			nox::EntitySystemBase* system;
			nox::Archetype* archetype;
			nox::uint32 chunk_index;
			/// @brief 配り元のノード番号。ジョブを引いたワーカーが記録先を束ね直すために要る。
			nox::uint32 node_index;
		};
	public:
		World();
		~World()override;

		void Run();
		inline constexpr nox::uint32 GetFrameCount()const noexcept { return frame_counter_; }
		inline constexpr nox::uint16 GetTargetFrameRate()const noexcept { return target_frame_rate_; }
		inline constexpr bool EnabledVSync()const noexcept { return enabled_vsync_; }
		void SetVSync(bool flag)noexcept;

		inline bool IsKill()const noexcept { return kill_.load(std::memory_order_acquire); }
		inline bool IsStudioMode()const noexcept { return studio_mode_; }

		nox::SystemBase* FindSystem(const nox::reflection::Type& type)const noexcept;

		template<std::derived_from<nox::SystemBase> T>
		inline T* FindSystem()const noexcept
		{
			return static_cast<T*>(FindSystem(nox::reflection::Typeof<T>()));
		}

		nox::SystemBase& GetSystem(const nox::reflection::Type& type)const;

		template<std::derived_from<nox::SystemBase> T>
		inline T& GetSystem()const
		{
			return static_cast<T&>(GetSystem(nox::reflection::Typeof<T>()));
		}

#if !NOX_MASTER
		NOX_ATTR_DECLARE(::nox::reflection::attr::IgnoreReflection())
		nox::U8FixedString<3072> BuildRuntimeDependencyGraphText()const;
#endif // !NOX_MASTER

#pragma region 構造変更(即時系)
		/// @brief entityを生成する。列挙中・フェーズ実行中は呼べない。
		nox::EntityId CreateEntity();
		/// @brief entityを即座に破棄する。列挙中・フェーズ実行中は呼べない。
		void DestroyEntity(nox::EntityId entity);
		bool IsAlive(nox::EntityId entity)const noexcept;

		/// @brief 即時系の構造変更を今呼んでよいか。
		/// @details 即時系4本(CreateEntity / DestroyEntity / AddComponent / RemoveComponent)が
		///          実際に分岐している唯一の判定点。アサートは理由を伝えるだけで、
		///          弾く判断そのものはこの値が行う。
		///
		///          Master構成でも生き続ける。NOX_ASSERTはMasterで消えるが、
		///          早期returnまで消すと「Debugでは安全にno-opになる操作がMasterではArchetypeを
		///          壊す」という構成間の挙動差になるため。コストはatomicロード1回で、
		///          Archetype間の実データ移動に比べれば無視できる。
		[[nodiscard]] nox::StructuralChangePermission GetStructuralChangePermission()const noexcept;

		[[nodiscard]] inline bool IsExecutingSystemPhase()const noexcept
		{
			return (structural_change_state_.load(std::memory_order_seq_cst) & k_structural_change_phase_bit) != 0u;
		}

		[[nodiscard]] inline bool IsIteratingEntities()const noexcept
		{
			return (structural_change_state_.load(std::memory_order_seq_cst) & k_structural_change_iteration_mask) != 0u;
		}

		/// @brief entityの列挙に入る。この間、即時系の構造変更は弾かれる。
		/// @details 入れ子にできる。Chunk並列では各ワーカーが独立に出入りするので、
		///          CASではなくfetch_add/subで回す。
		///
		///          フェーズ内での並列実行に対する本命の防御はフェーズbitのほうで、
		///          このカウンタが主に拾うのは「フェーズの外で列挙している間の即時系呼び出し」。
		///          詳細は structural_change_state_ の注記を参照。
		void EnterEntityIteration()noexcept;
		void LeaveEntityIteration()noexcept;
#pragma endregion

#pragma region 構造変更(遅延系)
		//	いずれもフェーズ実行中または列挙中にのみ記録できる。
		//	即時系が呼べないときのための系統なので、両者の可否はちょうど相補になっている。
		//	通常は nox::EntityCommands 経由で呼ばれる。

		/// @brief フェーズ実行中・列挙中にentityのIdだけを即座に払い出す。
		/// @details EntityRecordを1件触るだけでArchetypeにも他entityの行にも触れないため、
		///          列挙中のポインタと行番号を壊さない。ComponentDataはQueueAddComponentで積む。
		nox::EntityId CreateEntityDuringPhase()noexcept;
		[[nodiscard]] bool QueueDestroyEntity(nox::EntityId entity)noexcept;
		/// @brief ComponentDataの追加を予約する。sourceが非nullならその初期値を複製して運ぶ。
		[[nodiscard]] bool QueueAddComponent(
			nox::EntityId entity,
			const nox::ComponentTypeInfo& type_info,
			const void* source)noexcept;
		[[nodiscard]] bool QueueRemoveComponent(nox::EntityId entity, const nox::ComponentTypeInfo& type_info)noexcept;

		/// @brief ノード用コマンドバッファを確保する。Initがグラフを組んでから1回だけ呼ぶ。
		/// @details 確保はここだけ。フレーム中には一切走らない。
		///          既に同数以上を確保済みなら何もしない。
		///
		///          必要な本数は「ノード数」ではなく「nox::EntityCommands& を宣言したノードの数」。
		///          宣言していないノードは1コマンドも積めないことが引数リストから分かるので、
		///          そこへ空バッファを割り当てない。ノード数が増えるほど効く。
		void ReserveNodeEntityCommandBuffers(nox::uint32 node_count);

		/// @brief 確保済みのノード用コマンドバッファの本数。
		[[nodiscard]] inline nox::uint32 GetNodeEntityCommandBufferCount()const noexcept
		{
			return node_command_buffer_count_;
		}

		/// @brief コマンドバッファがこれまでに使った最大コマンド数 / ペイロードバイト数。
		/// @details 溢れたら abort する設計なので、容量を根拠づけるための実測窓口。
		///          全構成で使える(Masterで容量を詰めるときにも要るため)。
		///
		///          引数なしの版は「1本あたりの最大値のうち最大のもの」を返す。
		///          容量は1本ごとに効くので、合計ではなく最大を見るのが容量判断の正しい指標になる。
		[[nodiscard]] nox::uint32 GetEntityCommandPeakLength()const noexcept;
		[[nodiscard]] nox::uint32 GetEntityCommandPeakPayloadLength()const noexcept;

		/// @brief ノード1本ぶんのhigh-water mark。node_indexは nox::UpdaterNode::command_buffer_index。
		/// @details 未確保の番号を渡した場合は0を返す。
		[[nodiscard]] nox::uint32 GetNodeEntityCommandPeakLength(nox::uint32 node_index)const noexcept;
		[[nodiscard]] nox::uint32 GetNodeEntityCommandPeakPayloadLength(nox::uint32 node_index)const noexcept;

		/// @brief ノードの外(=どのノードにも束縛されていない状態)で積まれたコマンドのhigh-water mark。
		[[nodiscard]] nox::uint32 GetOutOfNodeEntityCommandPeakLength()const noexcept;

		[[nodiscard]] static inline constexpr nox::uint32 GetEntityCommandCapacity()noexcept
		{
			return k_entity_command_capacity;
		}

		[[nodiscard]] static inline constexpr nox::uint32 GetEntityCommandPayloadCapacity()noexcept
		{
			return k_entity_command_payload_bytes;
		}

		/// @brief 積まれた構造変更をまとめて反映する(Playbackポイント)。
		/// @details フェーズ終端でWorld自身が呼ぶ。列挙中は呼べない。
		///
		///          【Playback順が決定的である根拠】
		///          コマンドバッファは「遅延構造変更を出しうるノード」1つにつき1本あり、再生は
		///          「ノード外バッファ → バッファ0 → バッファ1 → …」の固定順で回す。
		///          バッファ番号は nox::UpdaterNode::command_buffer_index で、
		///          UpdaterGraphの登録順に昇順で詰めて振られる。
		///          これは構築時に決まりフレーム間で動かない(かつ衝突辺が必ず
		///          小さい番号から大きい番号へ張られるためトポロジカル順でもある)。
		///          よって「どのワーカーがどのノードを先に走らせたか」はPlaybackの順序に影響しない。
		///          ノード外バッファを先頭に置いているのは、そこへ積まれるのが
		///          UpdaterGraphのディスパッチより前に走る旧SystemPhaseなど、時間的に先行する経路だから。
		void FlushEntityCommands()noexcept;
#pragma endregion

#pragma region ComponentData
		/// @brief ComponentDataを追加する。Archetype間の移動を伴うため列挙中・System実行中は呼べない。
		/// @return 追加された(既に持っていた場合は既存の)ComponentDataへのポインタ。
		template<class T>
			requires(nox::IsComponentDataType<T>())
		T* AddComponent(const nox::EntityId entity)
		{
			return static_cast<T*>(AddComponent(entity, nox::ComponentTypeOf<T>()));
		}

		template<class T>
			requires(nox::IsComponentDataType<T>())
		void RemoveComponent(const nox::EntityId entity)
		{
			RemoveComponent(entity, nox::ComponentTypeOf<T>());
		}

		template<class T>
			requires(nox::IsComponentDataType<T>())
		[[nodiscard]] T* TryGetComponent(const nox::EntityId entity)noexcept
		{
			return static_cast<T*>(TryGetComponent(entity, nox::ComponentTypeIndexOf<T>()));
		}

		template<class T>
			requires(nox::IsComponentDataType<T>())
		[[nodiscard]] bool HasComponent(const nox::EntityId entity)const noexcept
		{
			return HasComponent(entity, nox::ComponentTypeIndexOf<T>());
		}

		void* AddComponent(nox::EntityId entity, const nox::ComponentTypeInfo& type_info);
		void RemoveComponent(nox::EntityId entity, const nox::ComponentTypeInfo& type_info);
		[[nodiscard]] void* TryGetComponent(nox::EntityId entity, nox::ComponentTypeIndex type_index)noexcept;
		[[nodiscard]] bool HasComponent(nox::EntityId entity, nox::ComponentTypeIndex type_index)const noexcept;
#pragma endregion

#pragma region Service
		/// @brief Serviceを登録する。所有権はWorldに移り、World破棄時に解放される。
		void RegisterService(const nox::reflection::Type& type, nox::Service& service);

		template<std::derived_from<nox::Service> T>
		void RegisterService(T& service) { RegisterService(nox::reflection::Typeof<T>(), service); }

		[[nodiscard]] nox::Service* TryGetService(const nox::reflection::Type& type)const noexcept;

		template<std::derived_from<nox::Service> T>
		[[nodiscard]] T* TryGetService()const noexcept
		{
			return static_cast<T*>(TryGetService(nox::reflection::Typeof<T>()));
		}
#pragma endregion

		/// @brief 指定したComponentDataを全て持つArchetypeにマッチするQueryを構築する。
		void BuildQuery(nox::EntityQuery& query, const nox::ComponentMask& required_mask);

		/// @brief entityが所属するArchetype。ComponentDataを1つも持たない場合はnullptr。
		[[nodiscard]] nox::Archetype* TryGetArchetype(nox::EntityId entity)const noexcept;

		/// @brief entityのArchetype内での位置。所属していない場合はInvalid。
		[[nodiscard]] nox::ArchetypeLocation GetArchetypeLocation(nox::EntityId entity)const noexcept;

	private:
		void Init();
		void Update();
		void Exit();
		void BuildExecuteNodeList(std::span<nox::SystemBase*> system_list);
		void ExecutePhase(const nox::SystemPhaseType phase_type);
		void RegisterSystem(nox::SystemBase& system);

		/// @brief 制約を検査せずに構造を変える本体。Playbackと即時系の共通の実装。
		[[nodiscard]] nox::EntityId CreateEntityImmediate();
		void DestroyEntityImmediate(nox::EntityId entity)noexcept;
		[[nodiscard]] void* AddComponentImmediate(nox::EntityId entity, const nox::ComponentTypeInfo& type_info);
		void RemoveComponentImmediate(nox::EntityId entity, const nox::ComponentTypeInfo& type_info);

		/// @brief entityのハンドルが今も生きているか(stale handleの検出用)。
		[[nodiscard]] bool IsEntityGenerationLive(nox::EntityId entity)const noexcept;

		/// @brief 今このスレッドが積むべきコマンドバッファ。
		/// @details nox::WorldNodeCommandScope で束縛されていればそのノードのもの、
		///          束縛が無い(またはこのWorldのものでない)ならノード外バッファ。
		[[nodiscard]] nox::World::EntityCommandBufferType& GetCurrentEntityCommandBuffer()noexcept;

		/// @brief コマンドバッファ1本を再生して空にする。
		void PlaybackEntityCommandBuffer(nox::World::EntityCommandBufferType& buffer)noexcept;

		/// @brief コマンドバッファが溢れたときに、理由を残して落とす。
		/// @details 構造変更を黙って捨てるのも次フレームへ繰り越すのも、後から原因を追えなくなる。
		[[noreturn]] void AbortOnEntityCommandOverflow(
			const nox::World::EntityCommandBufferType& buffer)noexcept;

		/// @brief 即時系を呼んでよいか検査し、駄目なら理由をアサートで伝える。
		[[nodiscard]] bool EnsureImmediateStructuralChangeAllowed()const noexcept;
		/// @brief 遅延系を記録してよいか(=フェーズ実行中または列挙中か)検査する。
		[[nodiscard]] bool EnsureDeferredStructuralChangeAllowed()const noexcept;

		void CreateEntitySystems();
		void CreateEntityLogicStorages();
		/// @brief UpdaterGraphのレイヤー順にノードを実行する。
		/// @details 現段階は「レイヤー順・レイヤー内は登録順」の直列実行。
		///          同一レイヤーのノードは互いに衝突しないため、stage 2bではこのループが配分点になる。
		void ExecuteUpdaterGraphPhase(nox::SystemPhaseType phase_type);
		/// @brief ノード1つを実行する。stage 2bではこの関数をそのままワーカーへ渡す。
		void ExecuteNode(const nox::UpdaterNode& node);
		/// @brief ExecuteNodeをジョブとして呼ぶためのthunk。contextはNodeJobContext*。
		static void ExecuteNodeJob(void* context);
		/// @brief レイヤーのノードを直列に実行する。
		void ExecuteLayerNodesSerial(std::span<const nox::UpdaterNode> nodes);
		/// @brief EntitySystemの列挙をChunk単位でワーカーへ配る(stage 2c)。
		/// @details ノードの排他はExecuteNodeが既に取っている前提。Chunk同士は互いに素なメモリなので、
		///          この内側では追加の排他は要らない。
		void ExecuteEntitySystemParallel(nox::EntitySystemBase& system, nox::uint32 node_index);
		/// @brief ExecuteChunkをジョブとして呼ぶためのthunk。contextはChunkJobContext*。
		static void ExecuteEntitySystemChunkJob(void* context);
		/// @brief entityのComponentData構成が変わったので、EntityLogicの生成/破棄を追従させる。
		void RefreshEntityLogics(nox::EntityId entity, const nox::Archetype* archetype);

		[[nodiscard]] nox::Archetype& GetOrCreateArchetype(const nox::ComponentMask& mask);
		[[nodiscard]] nox::Archetype* TryFindArchetype(const nox::ComponentMask& mask)const noexcept;
		/// @brief entityを別のArchetypeへ移す。共通のComponentDataだけが引き継がれる。
		void MoveEntityToArchetype(nox::World::EntityRecord& entity_record, nox::EntityId entity, nox::Archetype* destination);
		/// @brief swap-removeで移動してきたentityの位置情報を更新する。
		void PatchMovedEntityLocation(nox::EntityId moved_entity, nox::ArchetypeLocation location)noexcept;

#if !NOX_MASTER
		void TraceExecuteNodeList()const;

		/// @brief ノードが宣言したComponentData / Serviceの並列実行チェックに入る。
		void EnterNodeAccessScope(const nox::UpdaterNodeAccess& access)noexcept;
		/// @brief EnterNodeAccessScopeで入ったチェックから抜ける。
		void LeaveNodeAccessScope(const nox::UpdaterNodeAccess& access)noexcept;
		/// @brief Serviceの型に対応するチェッカー。未登録のServiceならnullptr。
		[[nodiscard]] nox::util::RWParallelExecuteChecker* TryGetServiceExecuteChecker(const nox::reflection::Type* type)noexcept;
		/// @brief 検出時のメッセージに載せる名前を、全チェッカーへ割り当てる。
		void SetupExecuteCheckerNames()noexcept;
#endif // !NOX_MASTER

		[[nodiscard]]
		nox::World::EntityRecord* TryGetEntityRecord(nox::uint32 index) noexcept;

		[[nodiscard]]
		const nox::World::EntityRecord* TryGetEntityRecord(nox::uint32 index) const noexcept;

		[[nodiscard]]
		nox::World::EntityRecord* EnsureEntityRecord(nox::uint32 index);

		[[nodiscard]]
		nox::uint32 TryPopFreeEntityIndex() noexcept;

		void PushFreeEntityIndex(nox::uint32 index) noexcept;

	private:
		NOX_ATTR(nox::reflection::attr::IgnoreReflection())
		nox::Atomic<nox::uint64> free_entity_head_;

		NOX_ATTR(nox::reflection::attr::IgnoreReflection())
		nox::Atomic<nox::uint32> next_entity_index_;

		NOX_ATTR(nox::reflection::attr::IgnoreReflection())
		nox::World::EntityRecordPage first_entity_record_page_;

		NOX_ATTR(nox::reflection::attr::IgnoreReflection())
		std::array<nox::Atomic<nox::World::EntityRecordPage*>, k_max_entity_page_count> entity_record_pages_;

		NOX_ATTR(nox::reflection::attr::IgnoreReflection())

		nox::StopWatch stop_watch_;
		nox::uint32 frame_counter_;
		nox::float_t elapsed_milli_seconds_;
		nox::float_t next_elapsed_milli_seconds_;
		nox::uint16 target_frame_rate_;
		bool enabled_vsync_;
		const bool studio_mode_;

		NOX_ATTR(nox::reflection::attr::IgnoreReflection())
		std::atomic_bool kill_;
		/// @brief 即時系の構造変更をブロックしている要因。
		/// @details bit31 = フェーズ実行中 / bit0..30 = 列挙スコープの入れ子深度。
		///          1ワードに詰めてあるので、即時系の判定はロード1回で済む。
		///
		///          【2つのビットの役割の違い】
		///          並列実行に対する本命の防御は bit31(フェーズ)のほう。ExecutePhaseが
		///          ワーカーへ配る前にメインスレッドで立て、Wait後に落とすため、
		///          JobSystemのDispatch/Wait自体が同期点になり、全ワーカーから確実に見える。
		///
		///          列挙カウンタ(下位ビット)が主に拾うのは、フェーズの外で列挙している間の誤用
		///          (Systemを直接Executeするツールやテスト、EntityLogicの単体呼び出しなど)。
		///          フェーズ内ではbit31が先に立っているので、こちらは二重の網でしかない。
		///
		///          【memory_orderをseq_cstにしてある理由】
		///          acq_relだと、列挙側のカウンタ増加と即時系側のロードとの間に
		///          store-load順序(Dekker型)が入らず、「並列列挙中の即時系を必ず弾ける」とは言えない。
		///          seq_cstなら全seq_cst操作に単一の全順序が入るため、見え方の遅れによる取りこぼしは無くなる
		///          (それでも「即時系の呼び出しが本当に列挙開始より前」の場合は弾かれないが、
		///           それは可視性の問題ではなく実際の事象順序なので正しい挙動)。
		///
		///          コストは実質ゼロ。この変数への書き込みは全てRMW(fetch_add/sub/or/and)で、
		///          x64では lock xadd 等、ARM64では ldaxr/stlxr となり acq_rel と同じ命令。
		///          ロード側も x64 では plain mov、ARM64 では ldar で acquire と同じ命令になる。
		///          「速度優先」を崩さずに保証だけ強くできるので、弱める理由が無い。
		std::atomic<nox::uint32> structural_change_state_;
		/// @brief どのノードにも束縛されていない状態で積まれたコマンドの受け皿。
		/// @details 旧SystemPhase(system_phase_table_)の実行中や、ツール・テストが
		///          自前で列挙している間に積まれたぶんがここへ入る。いずれもUpdaterGraphの
		///          並列ディスパッチの外なので、この1本の中の順序も決定的になる。
		///          Worldに埋め込む固定長。Initを呼ばないWorld(テスト等)でも必ず存在する。
		nox::World::EntityCommandBufferType out_of_node_command_buffer_;
		/// @brief ノードごとのコマンドバッファ。要素番号は nox::UpdaterNode::command_buffer_index。
		/// @details Initで一度だけ確保する。フレーム中の確保・解放は無い。
		///          Worldへ固定長配列で埋め込まないのは、ノード数がプロジェクト次第で
		///          「上限ぶんを常に抱える」形になるため。実際に要る本数だけ持つほうが素直に軽い。
		///          本数は「nox::EntityCommands& を宣言したノードの数」で、
		///          ノード総数より通常かなり少ない。
		NOX_ATTR(nox::reflection::attr::IgnoreReflection())
		nox::World::EntityCommandBufferType* node_command_buffers_;

		NOX_ATTR(nox::reflection::attr::IgnoreReflection())
		nox::uint32 node_command_buffer_count_;

		NOX_ATTR(nox::reflection::attr::IgnoreReflection())
		nox::Vector<nox::Archetype*> archetypes_;

		NOX_ATTR(nox::reflection::attr::IgnoreReflection())
		nox::Vector<nox::EntitySystemBase*> entity_systems_;

		NOX_ATTR(nox::reflection::attr::IgnoreReflection())
		nox::Vector<nox::EntityLogicStorage*> entity_logic_storages_;

		NOX_ATTR(nox::reflection::attr::IgnoreReflection())
		nox::UpdaterGraph updater_graph_;

		NOX_ATTR(nox::reflection::attr::IgnoreReflection())
		nox::JobSystem job_system_;

		/// @brief UpdaterGraphを回すワーカー数。0ならスレッドを1本も作らず直列実行になる。
		/// @details コマンドラインで決まる。決め方は nox::ResolveUpdaterWorkerCount を参照。
		const nox::uint32 updater_worker_count_;

#if !NOX_MASTER
		//	依存解析の誤りを即座に検出するためのチェッカー。ComponentTypeIndexごと / Service登録順ごとに1つ持つ。
		//	直列実行の現段階では決して発火しない。stage 2bで並列化したときに、宣言と実アクセスの
		//	食い違いをその場で落とすための土台。
		NOX_ATTR(nox::reflection::attr::IgnoreReflection())
		std::array<nox::util::RWParallelExecuteChecker, k_max_component_type_count> component_execute_checkers_;

		NOX_ATTR(nox::reflection::attr::IgnoreReflection())
		std::array<nox::util::RWParallelExecuteChecker, k_max_service_count> service_execute_checkers_;
#endif // !NOX_MASTER

		NOX_ATTR(nox::reflection::attr::IgnoreReflection())
		nox::FixedVector<nox::World::ServiceEntry, k_max_service_count> services_;

		nox::Vector<nox::EngineModule*> modules_;
		nox::Vector<nox::SystemBase*> systems_;
		nox::UnorderedMap<const nox::reflection::Type*, nox::SystemBase*> system_map_;
		std::array<nox::Vector<SystemExecuteNode>, nox::util::ToUnderlying(nox::SystemPhaseType::_Max)> system_phase_table_;
	};
}