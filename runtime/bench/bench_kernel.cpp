//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	bench_kernel.cpp
///	@brief	kernel の基盤部品 (コンテナ・文字列・delegate・数学・ロック) のベンチマーク
///	@details	フレーム中の一時リストをどのコンテナで持つかで、確保回数も時間も桁で変わる。
///				同じ「256 個積む」操作を 6 通りで並べ、確保 0 回の書き方がいくら得かを数字で見せる。

#include	"pch.h"
#include	"bench.h"

#include	"kernel/kernel.h"

#include	<array>
#include	<functional>
#include	<span>
#include	<string_view>
#include	<utility>

namespace
{
	//	---------------------------------------------------------------------------------
	//	コンテナ: 256 個積む
	//	---------------------------------------------------------------------------------

	constexpr nox::uint32 kPushCount = 256u;

	/// @brief nox::Vector に reserve なしで積む (伸長のたびに確保し直す)
	void BenchNoxVectorGrow(nox::bench::State& state)
	{
		state.Run([](const nox::uint64 op_count)
			{
				for (nox::uint64 op = 0u; op < op_count; ++op)
				{
					nox::Vector<nox::uint32> values;
					for (nox::uint32 index = 0u; index < kPushCount; ++index)
					{
						values.push_back(index);
					}
					nox::bench::DoNotOptimize(values.data());
				}
			});
	}

	/// @brief nox::Vector に reserve してから積む (確保 1 回)
	void BenchNoxVectorReserve(nox::bench::State& state)
	{
		state.Run([](const nox::uint64 op_count)
			{
				for (nox::uint64 op = 0u; op < op_count; ++op)
				{
					nox::Vector<nox::uint32> values;
					values.reserve(kPushCount);
					for (nox::uint32 index = 0u; index < kPushCount; ++index)
					{
						values.push_back(index);
					}
					nox::bench::DoNotOptimize(values.data());
				}
			});
	}

	/// @brief clear して使い回す nox::Vector に積む (定常状態で確保 0 回)
	/// @details 使い回すバッファはラムダの外に持ち、計測中だけローカルの vector へムーブして使う。
	///          StlAllocateAdapter は状態を持たない (is_always_equal) ので、ムーブはバッファの付け替えだけで
	///          確保は起きない。swap は Debug で allocator の operator== を要求するので使わない。
	///          外の vector を参照越しに直接触ると、下の「参照越し」のベンチと同じコード生成の差が混ざり、
	///          確保方式の差が見えなくなる。
	void BenchNoxVectorReuse(nox::bench::State& state)
	{
		nox::Vector<nox::uint32> storage;
		storage.reserve(kPushCount);
		state.Run([&storage](const nox::uint64 op_count)
			{
				nox::Vector<nox::uint32> values(std::move(storage));
				for (nox::uint64 op = 0u; op < op_count; ++op)
				{
					values.clear();
					for (nox::uint32 index = 0u; index < kPushCount; ++index)
					{
						values.push_back(index);
					}
					nox::bench::DoNotOptimize(values.data());
				}
				storage = std::move(values);
			});
	}

	/// @brief 参照越しの nox::Vector (メンバ変数のように外にあるもの) へ積む (確保 0 回)
	/// @details 上の reuse と仕事は同じで、vector を参照越しに触るところだけが違う。
	///          MSVC は型によるエイリアス解析をしないので、要素の書き込みが vector 自身の
	///          末尾ポインタを書き換えうるとみなし、push_back のたびに読み直す。
	///          メンバの vector へループで積むエンジンのコードが実際に払っているコストを見るためのもの。
	void BenchNoxVectorPushViaRef(nox::bench::State& state)
	{
		nox::Vector<nox::uint32> values;
		values.reserve(kPushCount);
		state.Run([&values](const nox::uint64 op_count)
			{
				for (nox::uint64 op = 0u; op < op_count; ++op)
				{
					values.clear();
					for (nox::uint32 index = 0u; index < kPushCount; ++index)
					{
						values.push_back(index);
					}
					nox::bench::DoNotOptimize(values.data());
				}
			});
	}

	/// @brief スタック上のアリーナを使う StackAllocVector に積む (確保 0 回)
	void BenchStackAllocVector(nox::bench::State& state)
	{
		state.Run([](const nox::uint64 op_count)
			{
				for (nox::uint64 op = 0u; op < op_count; ++op)
				{
					//	Size はバイト数。Debug ではコンテナプロキシも同じアリーナから取るので余裕を持たせる
					nox::StackAllocVector<nox::uint32, (kPushCount * sizeof(nox::uint32)) + 256u> scratch;
					auto& values = scratch.GetContainer();
					values.reserve(kPushCount);
					for (nox::uint32 index = 0u; index < kPushCount; ++index)
					{
						values.push_back(index);
					}
					nox::bench::DoNotOptimize(values.data());
				}
			});
	}

	/// @brief 固定長の FixedVector に積む (確保 0 回。構築時に全要素をゼロ埋めする分も測る)
	void BenchFixedVector(nox::bench::State& state)
	{
		state.Run([](const nox::uint64 op_count)
			{
				for (nox::uint64 op = 0u; op < op_count; ++op)
				{
					nox::FixedVector<nox::uint32, kPushCount> values;
					for (nox::uint32 index = 0u; index < kPushCount; ++index)
					{
						values.PushBack(index);
					}
					nox::bench::DoNotOptimize(values.GetStorage());
				}
			});
	}

	//	---------------------------------------------------------------------------------
	//	文字列
	//	---------------------------------------------------------------------------------

	/// @brief 固定バッファへ書式化する (アサート・ログと同じ経路。確保 0 回)
	void BenchFormatSpan(nox::bench::State& state)
	{
		std::array<nox::char8, 256> buffer{};
		state.Run([&buffer](const nox::uint64 op_count)
			{
				for (nox::uint64 op = 0u; op < op_count; ++op)
				{
					const nox::uint32 capacity = static_cast<nox::uint32>(op);
					const nox::uint32 request = static_cast<nox::uint32>(op >> 1u);
					nox::util::Format(std::span<nox::char8>(buffer), u8"キュー容量={0} 要求={1}", capacity, request);
					nox::bench::DoNotOptimize(buffer);
				}
			});
	}

	/// @brief 書式化した文字列を返す版 (SSO に収まらないので毎回確保する)
	void BenchFormatHeap(nox::bench::State& state)
	{
		nox::float_t x = 1.25f;
		const nox::float_t y = -3.5f;
		const nox::float_t z = 1000.0625f;
		state.Run([&x, y, z](const nox::uint64 op_count)
			{
				for (nox::uint64 op = 0u; op < op_count; ++op)
				{
					x += 0.001f;
					const auto text = nox::util::Format(u8"({},{},{})", x, y, z);
					nox::bench::DoNotOptimize(text.data());
				}
			});
	}

	/// @brief nox::util::Crc32 (リフレクションの名前引きで使う)
	void BenchCrc32(nox::bench::State& state)
	{
		std::array<nox::char8, 256> input{};
		for (size_t index = 0u; index < input.size(); ++index)
		{
			input[index] = static_cast<nox::char8>(u8'a' + (index % 26u));
		}
		nox::uint64 checksum = 0u;
		state.Run([&input, &checksum](const nox::uint64 op_count)
			{
				for (nox::uint64 op = 0u; op < op_count; ++op)
				{
					std::u8string_view view(input.data(), input.size());
					//	入力を毎回「読み直させる」。定数だと分かるとループの外へ追い出される
					nox::bench::DoNotOptimize(view);
					checksum += nox::util::Crc32(view);
				}
			});
		nox::bench::DoNotOptimize(checksum);
	}

	/// @brief UTF-16 → UTF-8 変換 (ログの出力経路)
	void BenchUtf16ToUtf8(nox::bench::State& state)
	{
		//	64 文字。日本語と ASCII を混ぜ、1〜3 バイトの符号化を全部通す
		static constexpr std::u16string_view kText =
			u"エンジン起動 JobWorker#12 初期化完了 reflection initialize 完了しました。 OK 12345";
		std::array<nox::char8, 512> dest{};
		state.Run([&dest](const nox::uint64 op_count)
			{
				for (nox::uint64 op = 0u; op < op_count; ++op)
				{
					std::u16string_view input = kText;
					nox::bench::DoNotOptimize(input);
					const std::u8string_view output = nox::unicode::ConvertU8String(input, std::span<nox::char8>(dest));
					nox::bench::DoNotOptimize(output);
				}
			});
	}

	//	---------------------------------------------------------------------------------
	//	delegate
	//	---------------------------------------------------------------------------------

	/// @brief MoveOnlyDelegate を IDelegate 越しに呼ぶ (間接呼び出し 1 回)
	void BenchMoveOnlyDelegateInvoke(nox::bench::State& state)
	{
		nox::uint64 accumulator = 0u;
		nox::MoveOnlyDelegate<void(int)> delegate{ [&accumulator](int value)noexcept { accumulator += static_cast<nox::uint64>(value); } };
		//	volatile なポインタを一度通して、/GL の脱仮想化で呼び出しごと畳まれないようにする
		nox::IDelegate<void(int)>* volatile view_pointer = &delegate;
		state.Run([&view_pointer](const nox::uint64 op_count)
			{
				nox::IDelegate<void(int)>& view = *view_pointer;
				for (nox::uint64 op = 0u; op < op_count; ++op)
				{
					view(static_cast<int>(op));
				}
			});
		nox::bench::DoNotOptimize(accumulator);
	}

	/// @brief 比較用: std::function を呼ぶ
	void BenchStdFunctionInvoke(nox::bench::State& state)
	{
		nox::uint64 accumulator = 0u;
		std::function<void(int)> function{ [&accumulator](int value)noexcept { accumulator += static_cast<nox::uint64>(value); } };
		std::function<void(int)>* volatile function_pointer = &function;
		state.Run([&function_pointer](const nox::uint64 op_count)
			{
				std::function<void(int)>& view = *function_pointer;
				for (nox::uint64 op = 0u; op < op_count; ++op)
				{
					view(static_cast<int>(op));
				}
			});
		nox::bench::DoNotOptimize(accumulator);
	}

	//	---------------------------------------------------------------------------------
	//	数学
	//	---------------------------------------------------------------------------------

	constexpr nox::uint32 kVectorCount = 10000u;

	//	16 バイト x 1 万 x 2 = 320KB。スタックには置かない
	std::array<nox::Vec3, kVectorCount> g_positions;
	std::array<nox::Vec3, kVectorCount> g_velocities;

	/// @brief Vec3 の位置更新 (p += v * dt) を 1 万要素
	void BenchVec3Integrate(nox::bench::State& state)
	{
		for (nox::uint32 index = 0u; index < kVectorCount; ++index)
		{
			g_positions[index] = nox::Vec3(0.0f, 0.0f, 0.0f);
			g_velocities[index] = nox::Vec3(0.01f * static_cast<nox::float_t>(index % 7u), 0.5f, -0.25f);
		}
		state.SetItemsPerOp(kVectorCount);

		nox::float_t delta_time = 1.0f / 60.0f;
		state.Run([&delta_time](const nox::uint64 op_count)
			{
				for (nox::uint64 op = 0u; op < op_count; ++op)
				{
					nox::bench::DoNotOptimize(delta_time);
					for (nox::uint32 index = 0u; index < kVectorCount; ++index)
					{
						g_positions[index] += g_velocities[index] * delta_time;
					}
					nox::bench::ClobberMemory();
				}
				nox::bench::DoNotOptimize(g_positions[0]);
			});
	}

	//	---------------------------------------------------------------------------------
	//	ロック
	//	---------------------------------------------------------------------------------

	constinit nox::Mutex g_static_lock;

	/// @brief nox::Mutex (SRWLOCK) を競合なしで取って放す。nox::memory::Allocate が毎回取るロック
	void BenchStaticLockUncontended(nox::bench::State& state)
	{
		nox::uint64 counter = 0u;
		state.Run([&counter](const nox::uint64 op_count)
			{
				for (nox::uint64 op = 0u; op < op_count; ++op)
				{
					nox::ScopedLock lock(g_static_lock);
					++counter;
				}
				nox::bench::DoNotOptimize(counter);
			});
	}
}

std::span<const nox::bench::Definition> nox::bench::GetKernelBenchmarks()noexcept
{
	static constexpr std::array<nox::bench::Definition, 14> kDefinitions{ {
		{ .name = "container/nox_vector_grow/256", .title = "nox::Vector に 256 個 push_back (reserve なし)", .per = "op", .alloc_budget = nox::bench::kNoBudget, .function = &BenchNoxVectorGrow },
		{ .name = "container/nox_vector_reserve/256", .title = "nox::Vector に reserve してから 256 個 push_back", .per = "op", .alloc_budget = 1, .function = &BenchNoxVectorReserve },
		{ .name = "container/nox_vector_reuse/256", .title = "clear して使い回す nox::Vector に 256 個 push_back", .per = "op", .alloc_budget = 0, .function = &BenchNoxVectorReuse },
		{ .name = "container/nox_vector_push_via_ref/256", .title = "参照越しの nox::Vector に 256 個 push_back (メンバ変数に積む形)", .per = "op", .alloc_budget = 0, .function = &BenchNoxVectorPushViaRef },
		{ .name = "container/stack_alloc_vector/256", .title = "StackAllocVector に 256 個 push_back", .per = "op", .alloc_budget = 0, .function = &BenchStackAllocVector },
		{ .name = "container/fixed_vector/256", .title = "FixedVector に 256 個 PushBack", .per = "op", .alloc_budget = 0, .function = &BenchFixedVector },
		{ .name = "string/format_span/u8", .title = "nox::util::Format を固定バッファへ (uint32 × 2)", .per = "call", .alloc_budget = 0, .function = &BenchFormatSpan },
		{ .name = "string/format_heap/u8", .title = "nox::util::Format で文字列を返す (float × 3)", .per = "call", .alloc_budget = nox::bench::kNoBudget, .function = &BenchFormatHeap },
		{ .name = "string/crc32/256", .title = "nox::util::Crc32 (256 バイト)", .per = "call", .alloc_budget = 0, .function = &BenchCrc32 },
		{ .name = "string/utf16_to_utf8/64", .title = "UTF-16 → UTF-8 変換 (64 文字)", .per = "call", .alloc_budget = 0, .function = &BenchUtf16ToUtf8 },
		{ .name = "delegate/move_only_invoke", .title = "MoveOnlyDelegate の呼び出し", .per = "call", .alloc_budget = 0, .function = &BenchMoveOnlyDelegateInvoke },
		{ .name = "delegate/std_function_invoke", .title = "比較用: std::function の呼び出し", .per = "call", .alloc_budget = 0, .function = &BenchStdFunctionInvoke },
		{ .name = "math/vec3_integrate/10k", .title = "Vec3 の位置更新 (1 万要素)", .per = "entity", .alloc_budget = 0, .function = &BenchVec3Integrate },
		{ .name = "lock/srw_uncontended", .title = "Mutex (SRWLOCK) の Lock/Unlock (競合なし)", .per = "pair", .alloc_budget = 0, .function = &BenchStaticLockUncontended },
	} };
	return kDefinitions;
}
