// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	updater_graph.h
/// @brief	同一フェーズ内の実行順(レイヤー)を、引数リストと明示的な順序宣言だけで決めるグラフ。
/// @details 規則:
///          1. 衝突: AがBの読み書きする対象へ書き込む(またはその逆)なら衝突。同一EntityLogic型の
///             更新メソッド同士もインスタンス状態を共有するので衝突。衝突しないノードは同時実行してよい。
///          2. 明示辺: 型に書いた `using RunAfter = nox::TypeList<...>;` / `using RunBefore = ...;`
///             (nox::TypeList を参照)。読み書きの衝突からは導けない因果を書くためのもので、
///             衝突の有無に関係なく辺として張る。
///          3. 全順序: 明示辺のトポロジカル順。明示辺で順序が決まらない箇所は
///             完全修飾型名(EntityLogicはさらにメソッド名)の昇順で決める。
///             登録順(= リフレクション生成器の走査順)は一切使わない。
///             型名は nox::util::GetTypeName でMSVC / clang-clの綴りを揃えてあるので、
///             ビルドやツールセットをまたいでも同じ順序になる。
///          4. 衝突するノードは全順序の前→後へ直列化する。
///
///          明示辺も衝突辺も必ず全順序の前から後へ張られるため、DAGに循環が生まれ得ない
///          (明示辺だけの循環は構築時に検出して起動を止める)。
///          このためレイヤー計算はBuildExecuteNodeListと同じ最長経路レイヤリングでありながら、
///          全順序がそのままトポロジカル順になり、前方への一度の走査で閉じる(訪問状態も再帰も要らない)。
///
///          明示辺の名前が解決できない・明示辺が循環する、は宣言の誤りなので nox::UpdaterGraph::Rebuild が
///          理由をログに残して abort する(Masterでも同じ)。テストは失敗を返す TryRebuild を使う。
///
///          ソートも名前解決も構築時に一度だけ行い、実行時はノード配列を順に舐めるだけ。
///          同一レイヤーのノード群は nox::ExecuteUpdaterLayer がワーカーへ配る
///          (kMainThreadOnly を宣言した型のノードだけは配らず、フェーズを回しているスレッドで実行する)。
#pragma once
#include	"entity_system.h"
#include	"entity_logic.h"
#include	"../kernel/job_system.h"

namespace nox
{
	/// @brief 実行ノードの種別。
	enum class UpdaterNodeKind : nox::uint8
	{
		/// @brief EntitySystem 1つ。
		EntitySystem,
		/// @brief EntityLogicの (ストレージ, 更新メソッド) 1組。
		EntityLogicMethod,
	};

	/// @brief インスタンス状態を共有しないことを表すグループ番号。
	inline constexpr nox::uint32 k_invalid_updater_group_index = std::numeric_limits<nox::uint32>::max();

	/// @brief 遅延構造変更を出さない(= nox::EntityCommands& を宣言していない)ノードの記録先番号。
	inline constexpr nox::uint32 k_invalid_updater_command_buffer_index = std::numeric_limits<nox::uint32>::max();

	/// @brief ノード1つ分のアクセス宣言。依存解析の唯一の入力。
	/// @details Worldを介さずに組み立てられるため、テストから直接レイヤリングを検証できる。
	struct UpdaterNodeAccess final
	{
		/// @brief 読み書きするComponentData。
		nox::ComponentMask read_write_mask;
		/// @brief 書き込みするComponentData(read_write_maskの部分集合)。
		nox::ComponentMask write_mask;
		/// @brief 読み書きするService。
		std::span<const nox::ServiceAccess> service_accesses;
		/// @brief 同じインスタンス状態を共有するノードのグループ。
		/// @details 同一EntityLogic型の更新メソッド同士は、宣言が重ならなくてもメンバ変数を共有するため
		///          必ず衝突させる。k_invalid_updater_group_index なら共有相手がいない。
		nox::uint32 group_index = nox::k_invalid_updater_group_index;
	};

	/// @brief 明示的な順序宣言(RunAfter / RunBefore)から張った辺1本。番号は全順序での位置。
	struct UpdaterOrderEdge final
	{
		/// @brief 先に実行するノード。
		nox::uint32 from = 0u;
		/// @brief 後に実行するノード。from < to。
		nox::uint32 to = 0u;
	};

	/// @brief 2つのノードが同一フェーズ内で同時実行できないか。
	/// @details (a) 同一ComponentDataにRWが絡む (b) 同一Serviceにwriteが絡む (c) インスタンス状態を共有する
	///          のいずれかで衝突する。read同士は衝突しない。
	[[nodiscard]] bool ConflictsUpdaterNodeAccess(
		const nox::UpdaterNodeAccess& a,
		const nox::UpdaterNodeAccess& b)noexcept;

	/// @brief 宣言リストからレイヤー番号を計算する。明示辺が無い場合の短縮形。
	/// @details accessesは全順序に並んでいること(衝突するノードは前の方が先に実行される)。
	///          dest_layer_indicesはaccessesと同じ長さが必要。
	///          ヒープを一切使わないため、テストからスタック上の配列だけで呼べる。
	/// @return 使われたレイヤー数(最大レイヤー番号 + 1)。空なら0。
	nox::uint32 BuildUpdaterLayerIndices(
		std::span<const nox::UpdaterNodeAccess> accesses,
		std::span<nox::uint32> dest_layer_indices)noexcept;

	/// @brief 宣言リストと明示辺からレイヤー番号を計算する。
	/// @details accessesは全順序に並んでいること。order_edgesは全て from < to で、toの昇順に並んでいること
	///          (nox::UpdaterGraph の構築がこの形で渡す)。明示辺は衝突が無くても直列化する。
	///          ヒープを一切使わない。
	/// @return 使われたレイヤー数(最大レイヤー番号 + 1)。空なら0。
	nox::uint32 BuildUpdaterLayerIndicesWithOrderEdges(
		std::span<const nox::UpdaterNodeAccess> accesses,
		std::span<const nox::UpdaterOrderEdge> order_edges,
		std::span<nox::uint32> dest_layer_indices)noexcept;

	/// @brief UpdaterGraphの構築が失敗した理由。いずれも宣言(コード)の誤りで、実行時の状態には依存しない。
	enum class UpdaterGraphBuildError : nox::uint8
	{
		/// @brief 成功。
		None,
		/// @brief RunAfter / RunBefore に並べた型が、EntitySystem / EntityLogic として登録されていない。
		UnresolvedOrderTarget,
		/// @brief 並べた型は登録されているが、宣言した型と同じフェーズにノードを1つも持たない
		///        (辺が1本も張れず、宣言が黙って無効になる)。
		OrderTargetInOtherPhase,
		/// @brief 明示辺だけで循環している(自分自身を並べた場合を含む)。
		OrderCycle,
	};

	/// @brief UpdaterGraphの構築結果。
	/// @details 名前はいずれも静的記憶域(記述子・型名)を指すので、グラフより長く生きる。
	///          OrderCycle のときは「declaring が target の後に実行される」閉路上の辺1本を指す。
	struct UpdaterGraphBuildResult final
	{
		nox::UpdaterGraphBuildError error = nox::UpdaterGraphBuildError::None;
		/// @brief 失敗を検出したフェーズ。フェーズに依らない失敗では _Max。
		nox::SystemPhaseType phase = nox::SystemPhaseType::_Max;
		/// @brief 宣言した側(循環なら後に実行される側)の型名。
		std::string_view declaring_type_name;
		/// @brief 宣言した側のメソッド名。EntitySystem・フェーズに依らない失敗では空。
		std::string_view declaring_method_name;
		/// @brief 解決できなかった名前(循環なら先に実行される側の型名)。
		std::string_view target_type_name;
		/// @brief 循環のときの、先に実行される側のメソッド名。それ以外は空。
		std::string_view target_method_name;

		[[nodiscard]] constexpr bool IsSuccess()const noexcept { return error == nox::UpdaterGraphBuildError::None; }
	};

	/// @brief 実行ノード1つ。実行時に必要な情報だけを持ち、確保を伴う操作は何も持たない。
	struct UpdaterNode final
	{
		/// @brief 依存解析に使った宣言。実行時の並列実行チェッカーもこれを見る。
		nox::UpdaterNodeAccess access;
		nox::UpdaterNodeKind kind = nox::UpdaterNodeKind::EntitySystem;
		/// @brief kind == EntitySystem のときの実体。
		nox::EntitySystemBase* system = nullptr;
		/// @brief kind == EntityLogicMethod のときのインスタンス置き場。
		nox::EntityLogicStorage* storage = nullptr;
		/// @brief kind == EntityLogicMethod のときの更新メソッド。
		const nox::EntityLogicMethodDescriptor* method = nullptr;
		/// @brief フェーズ内の全順序での位置(明示辺のトポロジカル順、決まらない箇所は型名順)。
		/// @details 衝突したノードはこの順で直列化される。起動ログの n<番号> はこの値。
		nox::uint32 order_index = 0u;
		/// @brief 小さいほど先に実行。同一レイヤーは同時実行可能。
		nox::uint32 layer_index = 0u;
		/// @brief ワーカーへ配らず、フェーズを回しているスレッド上で実行するか。
		/// @details 型の kMainThreadOnly 宣言から来る(EntityLogicは型の全更新メソッドに掛かる)。
		///          依存解析(レイヤー)には影響しない。 nox::IsMainThreadOnlyUpdaterType を参照。
		bool main_thread_only = false;
		/// @brief 遅延構造変更の記録先バッファ番号。出さないノードは k_invalid_updater_command_buffer_index。
		/// @details nox::EntityCommands& を宣言したノードにだけ、全順序(order_index)の昇順に詰めて振る。
		///          大半のノードは構造変更を出さないため、order_indexをそのまま使うと
		///          「一度も使われない空バッファ」をノード数ぶん抱えることになる。
		///
		///          詰めても順序は保たれる。番号はorder_indexの昇順に振られるので、
		///          「バッファ番号順の再生」と「全順序での再生」は同じ並びになる。
		///          全順序は明示辺と衝突辺のトポロジカル順なので、実行と矛盾しない再生順でもある。
		nox::uint32 command_buffer_index = nox::k_invalid_updater_command_buffer_index;
	};

	/// @brief 1レイヤーでワーカーへ配れるノード数の上限。ジョブ配列をスタックに置くために固定する。
	/// @details 超えた分は配らずに、配った側のスレッドで直列に実行する(アサート済みの異常系)。
	inline constexpr nox::uint32 kMaxUpdaterNodesPerLayer = 256u;

	/// @brief ノード1つを実行する関数。contextは nox::ExecuteUpdaterLayer の呼び出し側が渡したもの。
	using UpdaterNodeExecuteFunction = void(*)(void* context, const nox::UpdaterNode& node);

	/// @brief 1レイヤー分のノードを実行する。全ノードが終わってから戻る。
	/// @details 同一レイヤーのノードは互いに衝突しないので、順序を問わず同時に走らせてよい。
	///            - ワーカーが無い、またはノードが1つ以下 … 呼び出しスレッド上で全順序どおりに直列実行
	///            - それ以外 … main_thread_only でないノードをワーカーへ配り、配ってから
	///                          main_thread_only のノードを呼び出しスレッド上で直接実行し、最後に Wait する
	///                          (Wait の間は呼び出しスレッドも残りのジョブを引いて働く)
	///          main_thread_only のノードは、どの経路でも必ず呼び出しスレッド上で走る。
	///
	///          配ってから回すのは、先に回すとその間ワーカーが遊ぶため。配った後なら
	///          呼び出しスレッドが main_thread_only のノードを回している間も、ワーカーは配られたノードを進められる。
	///
	///          確保は一切走らない。ジョブ配列はスタック上の固定長(nox::kMaxUpdaterNodesPerLayer)。
	///          nox::World::ExecuteUpdaterGraphPhase がレイヤーごとに呼ぶ。World非依存なので、
	///          テストから自前の nox::JobSystem で同じ経路を検証できる。
	///          エンジン内部の配分点なので汎用リフレクションには載せない(関数ポインタを引数に取る)。
	NOX_ATTR_DECLARE(::nox::reflection::attr::IgnoreReflection())
	void ExecuteUpdaterLayer(
		nox::JobSystem& job_system,
		std::span<const nox::UpdaterNode> nodes,
		nox::UpdaterNodeExecuteFunction execute,
		void* context);

	/// @brief フェーズごとの実行ノードとレイヤー境界。
	/// @details 構築時にだけ確保し、実行時は確保も解放も行わない。
	class UpdaterGraph final
	{
	public:
		UpdaterGraph();
		~UpdaterGraph();

		UpdaterGraph(const UpdaterGraph&) = delete;
		UpdaterGraph& operator=(const UpdaterGraph&) = delete;

		/// @brief EntitySystem / EntityLogicの集合からグラフを組み直す。失敗したら起動を止める。
		/// @details 引数の並び(登録順)は結果に影響しない。順序は明示辺と型名だけで決まる。
		///          明示辺の名前が解決できない・循環する場合は、理由をログに出して abort する
		///          (Masterでも同じ。宣言の誤りを抱えたまま走らせない)。
		void Rebuild(
			std::span<nox::EntitySystemBase* const> systems,
			std::span<nox::EntityLogicStorage* const> storages);

		/// @brief Rebuild の本体。失敗を abort せずに返す。
		/// @details 失敗した場合、グラフは空(全フェーズでノード0)になる。
		///          起動経路は Rebuild を使うこと。これは失敗経路を検証するテストのための窓口。
		[[nodiscard]] nox::UpdaterGraphBuildResult TryRebuild(
			std::span<nox::EntitySystemBase* const> systems,
			std::span<nox::EntityLogicStorage* const> storages);

		/// @brief フェーズ内の全ノード。(レイヤー, 全順序)で整列済み。
		[[nodiscard]] std::span<const nox::UpdaterNode> GetNodes(nox::SystemPhaseType phase_type)const noexcept;

		[[nodiscard]] nox::uint32 GetLayerCount(nox::SystemPhaseType phase_type)const noexcept;

		/// @brief フェーズ内で遅延構造変更を出しうるノードの数(= 必要なコマンドバッファの本数)。
		[[nodiscard]] nox::uint32 GetCommandBufferCount(nox::SystemPhaseType phase_type)const noexcept;

		/// @brief 指定レイヤーのノード群。互いに衝突しないため、そのまま並列に配れる。
		[[nodiscard]] std::span<const nox::UpdaterNode> GetLayerNodes(
			nox::SystemPhaseType phase_type,
			nox::uint32 layer_index)const noexcept;

#if !NOX_MASTER
		/// @brief グラフをログへ書き出す。ノードのレイヤー・宣言・明示辺・衝突辺が読める。
		/// @details 明示辺が無く、型名順だけで直列化の向きが決まった衝突(write/write・write/read)も
		///          一覧で出す。因果のある組ならRunAfter / RunBeforeで向きを宣言すべき候補になる。
		void Trace()const;
#endif // !NOX_MASTER

	private:
		/// @brief 全フェーズを空にする。
		void Clear()noexcept;

		[[nodiscard]] nox::UpdaterGraphBuildResult RebuildPhase(
			nox::SystemPhaseType phase_type,
			std::span<nox::EntitySystemBase* const> systems,
			std::span<nox::EntityLogicStorage* const> storages);

	private:
		/// @brief フェーズごとのノード列。(レイヤー, 全順序)で整列済み。
		std::array<nox::Vector<nox::UpdaterNode>, nox::util::ToUnderlying(nox::SystemPhaseType::_Max)> phase_nodes_;
		/// @brief フェーズごとのレイヤー開始位置。要素数は レイヤー数 + 1(末尾番兵)。
		std::array<nox::Vector<nox::uint32>, nox::util::ToUnderlying(nox::SystemPhaseType::_Max)> phase_layer_offsets_;
		/// @brief フェーズごとの、遅延構造変更を出しうるノード数。
		std::array<nox::uint32, nox::util::ToUnderlying(nox::SystemPhaseType::_Max)> phase_command_buffer_counts_;
#if !NOX_MASTER
		/// @brief フェーズごとの明示辺(order_indexで表す)。Traceで「明示辺で決まった順序か」を判定するためだけに持つ。
		std::array<nox::Vector<nox::UpdaterOrderEdge>, nox::util::ToUnderlying(nox::SystemPhaseType::_Max)> phase_order_edges_;
#endif // !NOX_MASTER
	};
}
