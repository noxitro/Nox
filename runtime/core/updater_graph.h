// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	updater_graph.h
/// @brief	同一フェーズ内の実行順(レイヤー)を、引数リストから導出した宣言だけで決めるグラフ。
/// @details 衝突の規則:
///          「AがBの読み書きする対象へ書き込む(またはその逆)なら衝突。衝突するノードは登録順で直列化し、
///            衝突しないノードは同時実行してよい」。
///
///          衝突辺は必ず登録順の小さい方から大きい方へ張られるため、DAGに循環が生まれ得ない。
///          このためレイヤー計算はBuildExecuteNodeListと同じ最長経路レイヤリングでありながら、
///          登録順がそのままトポロジカル順になり、前方への一度の走査で閉じる(訪問状態も再帰も要らない)。
///
///          明示的な順序の指定(Service のフェーズの After / Before)があるときは、最初にその指定だけで
///          登録順を安定に並べ替える(nox::SortUpdaterNodeOrder)。並べ替えた後は指定の辺も
///          登録順の小さい方から大きい方へ向くので、レイヤー計算は同じ前方への一度の走査で閉じる。
///
///          レイヤー分けは構築時に一度だけ行い、実行時はノード配列を順に舐めるだけ。
///          stage 2b では「同一レイヤーのノード群」をそのままワーカーへ配ればよい。
#pragma once
#include	"entity_system_legacy.h"
#include	"entity_logic.h"
#include	"service.h"

namespace nox
{
	/// @brief 実行ノードの種別。
	enum class UpdaterNodeKind : nox::uint8
	{
		/// @brief EntitySystem 1つ。
		EntitySystem,
		/// @brief EntityLogicの (ストレージ, 更新メソッド) 1組。
		EntityLogicMethod,
		/// @brief Serviceの (実体, フェーズ関数) 1組。
		ServicePhaseMethod,
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

	/// @brief 明示的な順序の指定1つ。from のノードを to のノードより先に実行する。
	/// @details 番号はノードの並び(呼び出し側が渡す accesses や dest_order の添字)。
	///          衝突の有無に関わらず守る。衝突しないノード同士でも、指定があれば別のレイヤーへ分ける。
	struct UpdaterNodeOrderEdge final
	{
		nox::uint32 from;
		nox::uint32 to;
	};

	/// @brief 2つのノードが同一フェーズ内で同時実行できないか。
	/// @details (a) 同一ComponentDataにRWが絡む (b) 同一Serviceにwriteが絡む (c) インスタンス状態を共有する
	///          のいずれかで衝突する。read同士は衝突しない。
	[[nodiscard]] bool ConflictsUpdaterNodeAccess(
		const nox::UpdaterNodeAccess& a,
		const nox::UpdaterNodeAccess& b)noexcept;

	/// @brief 明示的な順序の指定を満たすよう、登録順を安定に並べ替える。
	/// @details 元の登録順にノードを訪ね、まだ並んでいない先行ノードがあれば、それをノードの直前へ前倒しする
	///          (先行ノードが複数あれば元の番号の小さい順)。
	///          指定の無いノード同士は元の登録順を保ち、どのノードも元の位置より後ろへは下がらない。
	///          ヒープを一切使わないため、テストからスタック上の配列だけで呼べる。
	/// @param order_edges 元の並びの番号で表した順序の指定。
	/// @param dest_order 並べ替えた結果。dest_order[k] は k 番目に実行するノードの元の番号。要素数がノード数になる。
	/// @param scratch_state 作業領域。要素数はノード数以上。
	/// @return 指定に循環が無ければ true。循環があれば、循環を作る指定を1本ずつ無視して並べ、false を返す。
	[[nodiscard]] bool SortUpdaterNodeOrder(
		std::span<const nox::UpdaterNodeOrderEdge> order_edges,
		std::span<nox::uint32> dest_order,
		std::span<nox::uint8> scratch_state)noexcept;

	/// @brief 宣言リストからレイヤー番号を計算する。
	/// @details accessesは登録順に並んでいること。dest_layer_indicesはaccessesと同じ長さが必要。
	///          ヒープを一切使わないため、テストからスタック上の配列だけで呼べる。
	/// @return 使われたレイヤー数(最大レイヤー番号 + 1)。空なら0。
	nox::uint32 BuildUpdaterLayerIndices(
		std::span<const nox::UpdaterNodeAccess> accesses,
		std::span<nox::uint32> dest_layer_indices)noexcept;

	/// @brief 宣言リストと明示的な順序の指定からレイヤー番号を計算する。
	/// @details order_edges は accesses の添字で表し、from < to であること(nox::SortUpdaterNodeOrder で並べ替えた後の形)。
	///          指定のある2ノードは、衝突しなくても to の方が後のレイヤーへ載る。
	/// @return 使われたレイヤー数(最大レイヤー番号 + 1)。空なら0。
	nox::uint32 BuildUpdaterLayerIndices(
		std::span<const nox::UpdaterNodeAccess> accesses,
		std::span<const nox::UpdaterNodeOrderEdge> order_edges,
		std::span<nox::uint32> dest_layer_indices)noexcept;

	/// @brief 実行ノード1つ。実行時に必要な情報だけを持ち、確保を伴う操作は何も持たない。
	struct UpdaterNode final
	{
		/// @brief 依存解析に使った宣言。実行時の並列実行チェッカーもこれを見る。
		nox::UpdaterNodeAccess access;
		nox::UpdaterNodeKind kind = nox::UpdaterNodeKind::EntitySystem;
		/// @brief kind == EntitySystem のときの実体。
		nox::legacy::EntitySystemBase* system = nullptr;
		/// @brief kind == EntityLogicMethod のときのインスタンス置き場。
		nox::EntityLogicStorage* storage = nullptr;
		/// @brief kind == EntityLogicMethod のときの更新メソッド。
		const nox::EntityLogicMethodDescriptor* method = nullptr;
		/// @brief kind == ServicePhaseMethod のときの Service の実体。
		void* service_instance = nullptr;
		/// @brief kind == ServicePhaseMethod のときのフェーズ関数。
		const nox::ServicePhaseMethodDescriptor* service_method = nullptr;
		/// @brief kind == ServicePhaseMethod のときの解決済みの引数。フェーズ関数の引数と同じ並び。
		/// @details UpdaterGraph が構築時に一度だけ解決して持つ配列を指す。実行時に Service を探さない。
		void* const* service_arguments = nullptr;
		/// @brief 登録順。衝突したノードはこの順で直列化される。
		nox::uint32 order_index = 0u;
		/// @brief 小さいほど先に実行。同一レイヤーは同時実行可能。
		nox::uint32 layer_index = 0u;
		/// @brief 遅延構造変更の記録先バッファ番号。出さないノードは k_invalid_updater_command_buffer_index。
		/// @details nox::EntityCommands& を宣言したノードにだけ、登録順に詰めて振る。
		///          大半のノードは構造変更を出さないため、order_indexをそのまま使うと
		///          「一度も使われない空バッファ」をノード数ぶん抱えることになる。
		///
		///          詰めても順序は保たれる。番号は登録順の昇順に振られるので、
		///          「バッファ番号順の再生」と「ノード登録順の再生」は同じ並びになる。
		nox::uint32 command_buffer_index = nox::k_invalid_updater_command_buffer_index;
	};

	/// @brief フェーズごとの実行ノードとレイヤー境界。
	/// @details 構築時にだけ確保し、実行時は確保も解放も行わない。
	class UpdaterGraph final
	{
	public:
		UpdaterGraph();
		~UpdaterGraph();

		UpdaterGraph(const UpdaterGraph&) = delete;
		UpdaterGraph& operator=(const UpdaterGraph&) = delete;

		/// @brief EntitySystem / EntityLogic / Serviceのフェーズ関数の集合からグラフを組み直す。
		/// @details 登録順のキーは「servicesの並び → systemsの並び → storagesの並び」(各メソッド表の並び)。
		///          Terminateだけは Service を最後に回す(System / EntityLogic の終了処理が Service を使えるように)。
		///          Service のフェーズの After / Before があれば、その指定を満たすよう登録順を並べ替える。
		///
		///          フェーズ関数の引数の Service は、services の中から型で探してここで一度だけ解決する。
		///          参照で受けた引数が見つからないフェーズ関数はノードにしない(アサートで知らせる)。
		void Rebuild(
			std::span<nox::legacy::EntitySystemBase* const> systems,
			std::span<nox::EntityLogicStorage* const> storages,
			std::span<const nox::ServiceInstance> services = {});

		/// @brief フェーズ内の全ノード。(レイヤー, 登録順)で整列済み。
		[[nodiscard]] std::span<const nox::UpdaterNode> GetNodes(nox::SystemPhaseType phase_type)const noexcept;

		[[nodiscard]] nox::uint32 GetLayerCount(nox::SystemPhaseType phase_type)const noexcept;

		/// @brief フェーズ内で遅延構造変更を出しうるノードの数(= 必要なコマンドバッファの本数)。
		[[nodiscard]] nox::uint32 GetCommandBufferCount(nox::SystemPhaseType phase_type)const noexcept;

		/// @brief 指定レイヤーのノード群。互いに衝突しないため、そのまま並列に配れる。
		[[nodiscard]] std::span<const nox::UpdaterNode> GetLayerNodes(
			nox::SystemPhaseType phase_type,
			nox::uint32 layer_index)const noexcept;

#if !NOX_MASTER
		/// @brief グラフをログへ書き出す。ノードのレイヤー・宣言・衝突辺・順序の指定が読める。
		void Trace()const;
#endif // !NOX_MASTER

	private:
		void RebuildPhase(
			nox::SystemPhaseType phase_type,
			std::span<nox::legacy::EntitySystemBase* const> systems,
			std::span<nox::EntityLogicStorage* const> storages,
			std::span<const nox::ServiceInstance> services);

	private:
		/// @brief フェーズごとのノード列。(レイヤー, 登録順)で整列済み。
		std::array<nox::Vector<nox::UpdaterNode>, nox::util::ToUnderlying(nox::SystemPhaseType::_Max)> phase_nodes_;
		/// @brief フェーズごとのレイヤー開始位置。要素数は レイヤー数 + 1(末尾番兵)。
		std::array<nox::Vector<nox::uint32>, nox::util::ToUnderlying(nox::SystemPhaseType::_Max)> phase_layer_offsets_;
		/// @brief フェーズごとの、遅延構造変更を出しうるノード数。
		std::array<nox::uint32, nox::util::ToUnderlying(nox::SystemPhaseType::_Max)> phase_command_buffer_counts_;
		/// @brief フェーズごとの、Serviceのフェーズ関数の解決済みの引数。ノードの service_arguments が指す。
		std::array<nox::Vector<void*>, nox::util::ToUnderlying(nox::SystemPhaseType::_Max)> phase_service_arguments_;
	};
}
