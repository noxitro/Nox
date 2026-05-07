//	Copyright (C) 2026 NOX ENGINE All rights reserved.

///	@file	pmr_buffer.h
///	@brief	PMR アリーナアロケータ
#pragma once
#include	"pmr.h"
#include	"advanced_type.h"

namespace nox
{
	template<class T>
	using PmrVector = std::pmr::vector<T>;

	template<class T>
	using PmrDeque = std::pmr::deque<T>;

	template<class T>
	using PmrList = std::pmr::list<T>;

	template<class T>
	using PmrStlString = std::pmr::basic_string<T>;

	/// @brief アリーナのバッファ枯渇時の振る舞い
	enum class ArenaOverflow : nox::uint8
	{
		Fallback,  ///< エンジンのヒープアロケータにフォールバック（デフォルト）
		Throw,     ///< std::bad_alloc を送出
	};

	/// @brief  スコープ付きモノトニック・アリーナアロケータ
	///
	/// @details
	///   事前確保した固定バッファ上で PMR vector を高速に構築する。
	///   monotonic_buffer_resource を使用するため個別の deallocate は行われず、
	///   Reset() または破棄時にまとめて解放される。
	///
	///   1 つのアリーナにつき 1 つの vector を構築する設計。
	///   MakeVector は capacity を省略するとバッファ全体を使い切る。
	///
	///   バッファ枯渇時の振る舞いは ArenaOverflow で指定する。
	///   - Fallback: エンジンのヒープアロケータにフォールバック
	///   - Throw:    std::bad_alloc を送出（固定予算の厳守）
	///
	/// @warning スレッドセーフではない。複数スレッドからの同時アクセスには外部同期が必要。
	/// @warning 生成したコンテナのライフタイムは本オブジェクトおよび元の storage を
	///          超えてはならない。違反すると未定義動作。
	///
	/// @code
	///   // 外部ストレージ版
	///   std::array<nox::uint8, 4096> storage{};
	///   nox::PmrArena arena(storage, nox::ArenaOverflow::Throw);
	///   auto vec = arena.MakeVector<int>();   // capacity = 4096 / sizeof(int)
	///
	///   // 内包ストレージ版
	///   nox::FixedPmrArena<4096> fixed(nox::ArenaOverflow::Throw);
	///   auto vec = fixed.MakeVector<int>();
	/// @endcode
	class PmrArena
	{
	public:
		/// @param storage  バッファ領域。本オブジェクトより長く生存すること。
		/// @param overflow バッファ枯渇時のポリシー。
		explicit PmrArena(
			std::span<nox::uint8> storage,
			ArenaOverflow overflow = ArenaOverflow::Fallback) noexcept
			: storage_size_(storage.size())
			, resource_(storage.data(), storage.size(), SelectUpstream(overflow))
		{
		}

		/// @param storage  バッファ領域。本オブジェクトより長く生存すること。
		/// @param upstream バッファ枯渇時のフォールバック先（上級者向け）。
		explicit PmrArena(
			std::span<nox::uint8> storage,
			std::pmr::memory_resource* upstream) noexcept
			: storage_size_(storage.size())
			, resource_(storage.data(), storage.size(), upstream)
		{
		}

		PmrArena(const PmrArena&) = delete;
		PmrArena(PmrArena&&) = delete;
		PmrArena& operator=(const PmrArena&) = delete;
		PmrArena& operator=(PmrArena&&) = delete;

		/// @brief 全割り当てを解放しバッファを初期状態に戻す。
		/// @warning 既存のコンテナは全て無効化される。Reset 後のアクセスは未定義動作。
		void Reset() noexcept
		{
			resource_.release();
		}

		/// @brief PMR アロケータを取得。
		[[nodiscard]] std::pmr::polymorphic_allocator<> GetAllocator() noexcept
		{
			return std::pmr::polymorphic_allocator<>(&resource_);
		}

		/// @brief 初期バッファサイズ（バイト数）
		[[nodiscard]] std::size_t GetStorageSize() const noexcept
		{
			return storage_size_;
		}

		// ── ファクトリメソッド ──────────────────────────

		/// @brief  PMR vector を構築する。
		/// @param  capacity 事前確保する要素数。省略時はバッファ全体から算出。
		template<class T>
		[[nodiscard]] std::pmr::vector<T> MakeVector(std::size_t capacity = 0)
		{
			std::pmr::vector<T> v(&resource_);
			v.reserve(capacity > 0 ? capacity : storage_size_ / sizeof(T));
			return v;
		}

	private:
		static std::pmr::memory_resource* SelectUpstream(ArenaOverflow overflow) noexcept
		{
			switch (overflow)
			{
			case ArenaOverflow::Throw:
				return std::pmr::null_memory_resource();
			case ArenaOverflow::Fallback:
			default:
				return &nox::memory::detail::GetPmrMemoryResource();
			}
		}

		std::size_t storage_size_;
		std::pmr::monotonic_buffer_resource resource_;
	};

	/// @brief ストレージ内包型アリーナ。外部バッファの寿命管理が不要。
	///
	/// @details
	///   テンプレート引数 N バイトの内部バッファを持つ。
	///   storage のライフタイム問題を構造的に排除する。
	///   PmrArena& への暗黙変換により、PmrArena& を受け取る関数にそのまま渡せる。
	///
	/// @code
	///   nox::FixedPmrArena<4096> arena(nox::ArenaOverflow::Throw);
	///   auto vec = arena.MakeVector<int>();  // capacity = 4096 / sizeof(int)
	/// @endcode
	template<std::size_t N>
	class FixedPmrArena
	{
		// ⚠️ 宣言順序が重要: storage_ → arena_ の順で初期化される
	public:
		/// @param overflow バッファ枯渇時のポリシー。
		explicit FixedPmrArena(ArenaOverflow overflow = ArenaOverflow::Fallback) noexcept
			: arena_(storage_, overflow)
		{
		}

		/// @param upstream バッファ枯渇時のフォールバック先（上級者向け）。
		explicit FixedPmrArena(std::pmr::memory_resource* upstream) noexcept
			: arena_(storage_, upstream)
		{
		}

		FixedPmrArena(const FixedPmrArena&) = delete;
		FixedPmrArena(FixedPmrArena&&) = delete;
		FixedPmrArena& operator=(const FixedPmrArena&) = delete;
		FixedPmrArena& operator=(FixedPmrArena&&) = delete;

		// ── PmrArena との統一インターフェース ──────────────

		/// @brief 内部の PmrArena への暗黙変換。PmrArena& を受け取る関数に直接渡せる。
		operator PmrArena&() noexcept { return arena_; }

		/// @brief 内部の PmrArena への明示的アクセス。
		[[nodiscard]] PmrArena& GetArena() noexcept { return arena_; }

		// ── 委譲メソッド（利便性のため） ────────────────

		void Reset() noexcept { arena_.Reset(); }

		[[nodiscard]] std::pmr::polymorphic_allocator<> GetAllocator() noexcept
		{
			return arena_.GetAllocator();
		}

		/// @brief 内部バッファサイズ（バイト数、コンパイル時定数）
		[[nodiscard]] static constexpr std::size_t GetStorageSize() noexcept { return N; }

		template<class T>
		[[nodiscard]] std::pmr::vector<T> MakeVector(std::size_t capacity = 0) { return arena_.MakeVector<T>(capacity); }

	private:
		alignas(std::max_align_t) std::array<nox::uint8, N> storage_{};
		PmrArena arena_;
	};
}