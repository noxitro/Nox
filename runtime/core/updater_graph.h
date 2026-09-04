// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	updater_graph.h
/// @brief	同一フェーズ内の実行順(レイヤー)を、引数リストから導出した宣言だけで決めるグラフ。
/// @details SOL-AVESの規則そのまま:
///          「AがBの読み書きする対象へ書き込む(またはその逆)なら衝突。衝突するノードは登録順で直列化し、
///            衝突しないノードは同時実行してよい」。
///
///          衝突辺は必ず登録順の小さい方から大きい方へ張られるため、DAGに循環が生まれ得ない。
///          このためレイヤー計算はBuildExecuteNodeListと同じ最長経路レイヤリングでありながら、
///          登録順がそのままトポロジカル順になり、前方への一度の走査で閉じる(訪問状態も再帰も要らない)。
///
///          レイヤー分けは構築時に一度だけ行い、実行時はノード配列を順に舐めるだけ。
///          stage 2b では「同一レイヤーのノード群」をそのままワーカーへ配ればよい。
#pragma once
#include	"entity_system.h"
#include	"entity_logic.h"

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

	/// @brief 2つのノードが同一フェーズ内で同時実行できないか。
	/// @details (a) 同一ComponentDataにRWが絡む (b) 同一Serviceにwriteが絡む (c) インスタンス状態を共有する
	///          のいずれかで衝突する。read同士は衝突しない。
	[[nodiscard]] bool ConflictsUpdaterNodeAccess(
		const nox::UpdaterNodeAccess& a,
		const nox::UpdaterNodeAccess& b)noexcept;

	/// @brief 宣言リストからレイヤー番号を計算する。
	/// @details accessesは登録順に並んでいること。dest_layer_indicesはaccessesと同じ長さが必要。
	///          ヒープを一切使わないため、テストからスタック上の配列だけで呼べる。
	/// @return 使われたレイヤー数(最大レイヤー番号 + 1)。空なら0。
	nox::uint32 BuildUpdaterLayerIndices(
		std::span<const nox::UpdaterNodeAccess> accesses,
		std::span<nox::uint32> dest_layer_indices)noexcept;

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

		/// @brief EntitySystem / EntityLogicの集合からグラフを組み直す。
		/// @details 登録順のキーは「systemsの並び → storagesの並び → メソッド表の並び」。
		void Rebuild(
			std::span<nox::EntitySystemBase* const> systems,
			std::span<nox::EntityLogicStorage* const> storages);

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
		/// @brief グラフをログへ書き出す。ノードのレイヤー・宣言・衝突辺が読める。
		void Trace()const;
#endif // !NOX_MASTER

	private:
		void RebuildPhase(
			nox::SystemPhaseType phase_type,
			std::span<nox::EntitySystemBase* const> systems,
			std::span<nox::EntityLogicStorage* const> storages);

	private:
		/// @brief フェーズごとのノード列。(レイヤー, 登録順)で整列済み。
		std::array<nox::Vector<nox::UpdaterNode>, nox::util::ToUnderlying(nox::SystemPhaseType::_Max)> phase_nodes_;
		/// @brief フェーズごとのレイヤー開始位置。要素数は レイヤー数 + 1(末尾番兵)。
		std::array<nox::Vector<nox::uint32>, nox::util::ToUnderlying(nox::SystemPhaseType::_Max)> phase_layer_offsets_;
		/// @brief フェーズごとの、遅延構造変更を出しうるノード数。
		std::array<nox::uint32, nox::util::ToUnderlying(nox::SystemPhaseType::_Max)> phase_command_buffer_counts_;
	};
}
