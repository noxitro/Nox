//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	service_graph_test.cpp
///	@brief	Service の寿命(ServiceGraph)の検証。時間にもスレッドにも依存しない。
///	@details	Service は Update を持たないが、寿命(OnInitialize / OnShutdown)は持つ。
///				保証しているのは次のとおりで、ここで全て固定する。
///
///				  ・初期化は Depends のトポロジカル順、決まらない箇所は完全修飾型名順(登録順は使わない)
///				  ・終了は初期化の逆順
///				  ・未登録の型への Depends / 循環は、どの OnInitialize よりも前に失敗として返る
///				  ・OnInitialize の false は、それまでに初期化した Service だけを逆順に終了させて失敗を返す
///				  ・nox::ServiceContext::Get は Depends に並べた型しか引けない(引いたら失敗)
///
///				失敗経路は abort せずに結果を返す nox::World::TryInitializeServices で見る
///				(nox::UpdaterGraph::TryRebuild と同じ考え)。起動経路の World::Init は失敗で abort する。
///
///	@note		テスト用の Service はこの翻訳単位にだけ置く。リフレクション生成器は
///				reflect.cpp から辿れるヘッダしか見ないので、ここに置いた型は生成の対象にならず、
///				Master でテスト型のリフレクションが生成されない事情とも無関係でいられる
///				(updater_graph_layering_test.cpp の LayerServiceX と同じ扱い)。

#include	"pch.h"

//	core_test の pch.h は gtest しか載せていない (test_new_delete.cpp の都合)。
//	core のヘッダは kernel / reflection の基盤型に依存するので、main.cpp と同じ順で先に入れる。
#include	"../../kernel/kernel.h"
#include	"../../reflection/reflection.h"

#include	"../world.h"
#include	"../service.h"

namespace nox::test::service_graph
{
	//	=================================================================================
	//	寿命の記録
	//	Service は World が解放するので、記録は World より長く生きる側(テスト本体)が持つ。
	//	=================================================================================

	enum class LifecycleEventKind : nox::uint8
	{
		Initialize,
		Shutdown,
	};

	struct LifecycleEvent final
	{
		nox::test::service_graph::LifecycleEventKind kind = nox::test::service_graph::LifecycleEventKind::Initialize;
		std::string_view name;
	};

	struct LifecycleLog final
	{
		std::array<nox::test::service_graph::LifecycleEvent, 32> events{};
		nox::uint32 count = 0u;

		void Record(const nox::test::service_graph::LifecycleEventKind kind, const std::string_view name)noexcept
		{
			if (count < events.size())
			{
				events[count] = nox::test::service_graph::LifecycleEvent{ .kind = kind, .name = name };
				++count;
			}
		}
	};

	/// @brief 寿命を記録する Service の共通部。
	/// @details 派生型は private な OnInitialize / OnShutdown を上書きするだけでよい(friend は書かない)、
	///          という規則をテスト側でもそのまま使っている。
	class RecordingService : public nox::Service
	{
	public:
		nox::test::service_graph::LifecycleLog* log = nullptr;
		/// @brief OnInitialize が返す値。失敗経路の検証で false にする。
		bool initialize_result = true;
		/// @brief OnInitialize が true を返したか。依存先が先に初期化されていることの確認に使う。
		bool initialized = false;

	protected:
		inline explicit RecordingService(const std::string_view name)noexcept :
			name_(name)
		{
		}

		/// @brief Depends に並べた Service を引く。依存を持つ派生型が上書きする。
		virtual bool AcquireDependencies([[maybe_unused]] nox::ServiceContext& context)noexcept { return true; }

	private:
		bool OnInitialize(nox::ServiceContext& context)noexcept override
		{
			log->Record(nox::test::service_graph::LifecycleEventKind::Initialize, name_);
			const bool acquired = AcquireDependencies(context);
			initialized = acquired && initialize_result;
			return initialized;
		}

		void OnShutdown()noexcept override
		{
			log->Record(nox::test::service_graph::LifecycleEventKind::Shutdown, name_);
		}

		std::string_view name_;
	};

	//	=================================================================================
	//	依存の形
	//	  SgBravo  … 依存なし
	//	  SgZulu   … 依存なし。型名順では最後
	//	  SgAlpha  … SgZulu に依存。型名順では先頭だが SgZulu の後ろへ回る
	//	  SgMike   … SgAlpha に依存
	//	型名順だけなら Alpha, Bravo, Mike, Zulu。Depends を入れると Bravo, Zulu, Alpha, Mike になる。
	//	=================================================================================

	class SgBravo final : public nox::test::service_graph::RecordingService
	{
	public:
		inline SgBravo()noexcept : nox::test::service_graph::RecordingService(nox::util::GetTypeName<SgBravo>()) {}
	};

	class SgZulu final : public nox::test::service_graph::RecordingService
	{
	public:
		inline SgZulu()noexcept : nox::test::service_graph::RecordingService(nox::util::GetTypeName<SgZulu>()) {}
	};

	class SgAlpha final : public nox::test::service_graph::RecordingService
	{
	public:
		using Depends = nox::TypeList<nox::test::service_graph::SgZulu>;

		inline SgAlpha()noexcept : nox::test::service_graph::RecordingService(nox::util::GetTypeName<SgAlpha>()) {}

		nox::test::service_graph::SgZulu* zulu = nullptr;
		bool zulu_was_initialized = false;

	private:
		bool AcquireDependencies(nox::ServiceContext& context)noexcept override
		{
			zulu = context.Get<nox::test::service_graph::SgZulu>();
			zulu_was_initialized = (zulu != nullptr) && zulu->initialized;
			return zulu != nullptr;
		}
	};

	class SgMike final : public nox::test::service_graph::RecordingService
	{
	public:
		using Depends = nox::TypeList<nox::test::service_graph::SgAlpha>;

		inline SgMike()noexcept : nox::test::service_graph::RecordingService(nox::util::GetTypeName<SgMike>()) {}

		nox::test::service_graph::SgAlpha* alpha = nullptr;
		bool alpha_was_initialized = false;

	private:
		bool AcquireDependencies(nox::ServiceContext& context)noexcept override
		{
			alpha = context.Get<nox::test::service_graph::SgAlpha>();
			alpha_was_initialized = (alpha != nullptr) && alpha->initialized;
			return alpha != nullptr;
		}
	};

	/// @brief SgAlpha に依存し、OnInitialize で失敗する。
	/// @details 型名順で SgMike より前 ("SgFailing" < "SgMike") なので、SgAlpha の直後に初期化される。
	///          つまり失敗した時点で SgMike はまだ初期化されていない。
	class SgFailing final : public nox::test::service_graph::RecordingService
	{
	public:
		using Depends = nox::TypeList<nox::test::service_graph::SgAlpha>;

		inline SgFailing()noexcept : nox::test::service_graph::RecordingService(nox::util::GetTypeName<SgFailing>())
		{
			initialize_result = false;
		}
	};

	/// @brief SgZulu だけを宣言しておきながら、宣言していない SgBravo も引こうとする。
	class SgGreedy final : public nox::test::service_graph::RecordingService
	{
	public:
		using Depends = nox::TypeList<nox::test::service_graph::SgZulu>;

		inline SgGreedy()noexcept : nox::test::service_graph::RecordingService(nox::util::GetTypeName<SgGreedy>()) {}

		nox::test::service_graph::SgZulu* zulu = nullptr;
		nox::test::service_graph::SgBravo* bravo = nullptr;

	private:
		bool AcquireDependencies(nox::ServiceContext& context)noexcept override
		{
			zulu = context.Get<nox::test::service_graph::SgZulu>();
			bravo = context.Get<nox::test::service_graph::SgBravo>();
			//	宣言外の Get を呼んだこと自体が失敗なので、戻り値を true にしても起動は止まる。
			return true;
		}
	};

	//	---------------------------------------------------------------------------------
	//	構築失敗の検証用
	//	---------------------------------------------------------------------------------

	/// @brief 前方宣言だけで、どこにも登録されない型。Depends に並べるのは名前だけなので定義は要らない。
	class SgNeverRegistered;

	class SgDangling final : public nox::test::service_graph::RecordingService
	{
	public:
		using Depends = nox::TypeList<nox::test::service_graph::SgNeverRegistered>;

		inline SgDangling()noexcept : nox::test::service_graph::RecordingService(nox::util::GetTypeName<SgDangling>()) {}
	};

	class SgCycleY;

	/// @brief SgCycleY と互いに依存し合う(閉路)。
	class SgCycleX final : public nox::test::service_graph::RecordingService
	{
	public:
		using Depends = nox::TypeList<nox::test::service_graph::SgCycleY>;

		inline SgCycleX()noexcept : nox::test::service_graph::RecordingService(nox::util::GetTypeName<SgCycleX>()) {}
	};

	class SgCycleY final : public nox::test::service_graph::RecordingService
	{
	public:
		using Depends = nox::TypeList<nox::test::service_graph::SgCycleX>;

		inline SgCycleY()noexcept : nox::test::service_graph::RecordingService(nox::util::GetTypeName<SgCycleY>()) {}
	};

	/// @brief 閉路の下流にあるだけで、閉路には乗っていない型。
	/// @details 型名順では閉路の2つより前 ("SgCycleAfterX" < "SgCycleX") なので、
	///          報告が閉路の上を指すこと(下流を指さないこと)の検証になる。
	class SgCycleAfterX final : public nox::test::service_graph::RecordingService
	{
	public:
		using Depends = nox::TypeList<nox::test::service_graph::SgCycleX>;

		inline SgCycleAfterX()noexcept : nox::test::service_graph::RecordingService(nox::util::GetTypeName<SgCycleAfterX>()) {}
	};

	/// @brief 自分自身に依存する(長さ1の閉路)。
	class SgSelf final : public nox::test::service_graph::RecordingService
	{
	public:
		using Depends = nox::TypeList<nox::test::service_graph::SgSelf>;

		inline SgSelf()noexcept : nox::test::service_graph::RecordingService(nox::util::GetTypeName<SgSelf>()) {}
	};
}

namespace
{
	using namespace nox::test::service_graph;

	/// @brief Service を生成して World へ登録する。所有権は World へ移る。
	template<class T>
	T& Register(nox::World& world, LifecycleLog& log)
	{
		T* const service = new T();
		service->log = &log;
		world.RegisterService(*service);
		return *service;
	}

	template<class T>
	[[nodiscard]] LifecycleEvent Initialized()noexcept
	{
		return LifecycleEvent{ .kind = LifecycleEventKind::Initialize, .name = nox::util::GetTypeName<T>() };
	}

	template<class T>
	[[nodiscard]] LifecycleEvent ShutDown()noexcept
	{
		return LifecycleEvent{ .kind = LifecycleEventKind::Shutdown, .name = nox::util::GetTypeName<T>() };
	}

	/// @brief 記録が期待どおりの並びであること。
	void ExpectLog(const LifecycleLog& log, const std::initializer_list<LifecycleEvent> expected)
	{
		ASSERT_EQ(static_cast<size_t>(log.count), expected.size());
		size_t index = 0u;
		for (const LifecycleEvent& event : expected)
		{
			EXPECT_EQ(log.events[index].kind, event.kind) << "index=" << index;
			EXPECT_EQ(log.events[index].name, event.name) << "index=" << index;
			++index;
		}
	}
}

//	=====================================================================================
//	順序
//	=====================================================================================

///	@brief	初期化は Depends の順(決まらない箇所は型名順)、終了はその逆順。
///	@details	型名順だけなら Alpha が先頭だが、SgZulu に依存するので SgZulu の後ろへ回る。
///				依存先は OnInitialize の時点で初期化済みで、Get で登録済みの実体が返る。
///				終了は World の破棄(Exit を通らない経路)でも初期化の逆順に行われる。
TEST(ServiceGraph, InitializeFollowsDependsAndShutdownIsReversed)
{
	LifecycleLog log;
	{
		nox::World world;
		SgAlpha& alpha = Register<SgAlpha>(world, log);
		Register<SgBravo>(world, log);
		SgMike& mike = Register<SgMike>(world, log);
		SgZulu& zulu = Register<SgZulu>(world, log);

		const nox::ServiceInitializeResult result = world.TryInitializeServices();
		ASSERT_TRUE(result.IsSuccess());

		ExpectLog(log, {
			Initialized<SgBravo>(),
			Initialized<SgZulu>(),
			Initialized<SgAlpha>(),
			Initialized<SgMike>(),
			});

		EXPECT_EQ(alpha.zulu, &zulu);
		EXPECT_TRUE(alpha.zulu_was_initialized);
		EXPECT_EQ(mike.alpha, &alpha);
		EXPECT_TRUE(mike.alpha_was_initialized);
	}

	ExpectLog(log, {
		Initialized<SgBravo>(),
		Initialized<SgZulu>(),
		Initialized<SgAlpha>(),
		Initialized<SgMike>(),
		ShutDown<SgMike>(),
		ShutDown<SgAlpha>(),
		ShutDown<SgZulu>(),
		ShutDown<SgBravo>(),
		});
}

///	@brief	登録順を入れ替えても、初期化順も終了順も変わらない。
TEST(ServiceGraph, RegistrationOrderDoesNotChangeTheOrder)
{
	LifecycleLog log;
	{
		nox::World world;
		Register<SgZulu>(world, log);
		Register<SgMike>(world, log);
		Register<SgBravo>(world, log);
		Register<SgAlpha>(world, log);

		const nox::ServiceInitializeResult result = world.TryInitializeServices();
		ASSERT_TRUE(result.IsSuccess());
	}

	ExpectLog(log, {
		Initialized<SgBravo>(),
		Initialized<SgZulu>(),
		Initialized<SgAlpha>(),
		Initialized<SgMike>(),
		ShutDown<SgMike>(),
		ShutDown<SgAlpha>(),
		ShutDown<SgZulu>(),
		ShutDown<SgBravo>(),
		});
}

///	@brief	初期化しなかった World を破棄しても、OnShutdown は呼ばれない。
TEST(ServiceGraph, ServicesThatWereNeverInitializedAreNotShutDown)
{
	LifecycleLog log;
	{
		nox::World world;
		Register<SgBravo>(world, log);
		Register<SgZulu>(world, log);
	}

	EXPECT_EQ(log.count, 0u);
}

//	=====================================================================================
//	失敗
//	=====================================================================================

///	@brief	登録されていない型を Depends に並べると失敗する。どの OnInitialize よりも前に検出される。
TEST(ServiceGraph, UnresolvedDependencyFailsBeforeAnyInitialize)
{
	LifecycleLog log;
	{
		nox::World world;
		Register<SgBravo>(world, log);
		Register<SgDangling>(world, log);

		const nox::ServiceInitializeResult result = world.TryInitializeServices();
		EXPECT_EQ(result.error, nox::ServiceInitializeError::UnresolvedDependency);
		EXPECT_EQ(result.service_type_name, nox::util::GetTypeName<SgDangling>());
		EXPECT_EQ(result.related_type_name, nox::util::GetTypeName<SgNeverRegistered>());
	}

	EXPECT_EQ(log.count, 0u);
}

///	@brief	Depends の循環は失敗する。報告は閉路の上の2つを指し、下流にあるだけの型は指さない。
TEST(ServiceGraph, CycleFailsBeforeAnyInitialize)
{
	LifecycleLog log;
	{
		nox::World world;
		Register<SgBravo>(world, log);
		Register<SgCycleAfterX>(world, log);
		Register<SgCycleX>(world, log);
		Register<SgCycleY>(world, log);

		const nox::ServiceInitializeResult result = world.TryInitializeServices();
		EXPECT_EQ(result.error, nox::ServiceInitializeError::DependencyCycle);

		const std::string_view cycle_x = nox::util::GetTypeName<SgCycleX>();
		const std::string_view cycle_y = nox::util::GetTypeName<SgCycleY>();
		EXPECT_TRUE((result.service_type_name == cycle_x) || (result.service_type_name == cycle_y))
			<< result.service_type_name;
		EXPECT_TRUE((result.related_type_name == cycle_x) || (result.related_type_name == cycle_y))
			<< result.related_type_name;
		EXPECT_NE(result.service_type_name, result.related_type_name);
	}

	//	SgBravo は依存を持たないが、閉路の検出は初期化より前なので初期化されない。
	EXPECT_EQ(log.count, 0u);
}

///	@brief	自分自身を Depends に並べると循環として失敗する。
TEST(ServiceGraph, SelfDependencyFails)
{
	LifecycleLog log;
	{
		nox::World world;
		Register<SgSelf>(world, log);

		const nox::ServiceInitializeResult result = world.TryInitializeServices();
		EXPECT_EQ(result.error, nox::ServiceInitializeError::DependencyCycle);
		EXPECT_EQ(result.service_type_name, nox::util::GetTypeName<SgSelf>());
		EXPECT_EQ(result.related_type_name, nox::util::GetTypeName<SgSelf>());
	}

	EXPECT_EQ(log.count, 0u);
}

///	@brief	OnInitialize が false を返すと、それまでに初期化した Service だけが逆順に終了する。
///	@details	順序は Bravo, Zulu, Alpha, Failing, (Mike)。Failing で止まるので Mike は初期化も終了もされない。
///				Failing 自身も初期化済みではないので終了されない。World の破棄で二重に終了されることもない。
TEST(ServiceGraph, InitializeFailureShutsDownOnlyInitializedServicesInReverse)
{
	LifecycleLog log;
	{
		nox::World world;
		Register<SgMike>(world, log);
		Register<SgFailing>(world, log);
		Register<SgAlpha>(world, log);
		Register<SgZulu>(world, log);
		Register<SgBravo>(world, log);

		const nox::ServiceInitializeResult result = world.TryInitializeServices();
		EXPECT_EQ(result.error, nox::ServiceInitializeError::InitializeFailed);
		EXPECT_EQ(result.service_type_name, nox::util::GetTypeName<SgFailing>());

		ExpectLog(log, {
			Initialized<SgBravo>(),
			Initialized<SgZulu>(),
			Initialized<SgAlpha>(),
			Initialized<SgFailing>(),
			ShutDown<SgAlpha>(),
			ShutDown<SgZulu>(),
			ShutDown<SgBravo>(),
			});
	}

	//	World の破棄で追加の終了は起きない。
	EXPECT_EQ(log.count, 7u);
}

///	@brief	Depends に並べていない型を ServiceContext::Get で引くと nullptr が返り、起動は失敗する。
///	@details	登録済みの型であっても、宣言していなければ渡さない。宣言した型は同じ呼び出しの中で引ける。
///				OnInitialize 自体は true を返しているので、SgGreedy も初期化済みとして逆順に終了される。
TEST(ServiceGraph, GetOfUndeclaredServiceFails)
{
	LifecycleLog log;
	{
		nox::World world;
		SgBravo& bravo = Register<SgBravo>(world, log);
		SgZulu& zulu = Register<SgZulu>(world, log);
		SgGreedy& greedy = Register<SgGreedy>(world, log);

		const nox::ServiceInitializeResult result = world.TryInitializeServices();
		EXPECT_EQ(result.error, nox::ServiceInitializeError::UndeclaredServiceAccess);
		EXPECT_EQ(result.service_type_name, nox::util::GetTypeName<SgGreedy>());
		EXPECT_EQ(result.related_type_name, nox::util::GetTypeName<SgBravo>());

		EXPECT_EQ(greedy.zulu, &zulu);
		EXPECT_EQ(greedy.bravo, nullptr);
		EXPECT_TRUE(bravo.initialized);

		ExpectLog(log, {
			Initialized<SgBravo>(),
			Initialized<SgZulu>(),
			Initialized<SgGreedy>(),
			ShutDown<SgGreedy>(),
			ShutDown<SgZulu>(),
			ShutDown<SgBravo>(),
			});
	}

	EXPECT_EQ(log.count, 6u);
}
