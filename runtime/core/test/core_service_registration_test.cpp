//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	core_service_registration_test.cpp
///	@brief	Core が登録する Service (SceneManager / AssetManager / GarbageCollector) の Depends の宣言と解決の検証。
///	@details	AssetManager は開発ビルドでだけ EditorRemoteServer に依存する (Depends を #if NOX_DEVELOP で囲んである)。
///				宣言の #if を取り違えると、Master で未定義の型を並べてビルドが壊れるか、
///				開発ビルドで依存が黙って消えて初期化順が崩れる。どちらの構成でも宣言どおりに解決されることを固定する。
///
///				実際の Service は外部資源に触れるので、初期化まで走らせるのは副作用の小さいものだけにする。
///				  ・SceneManager の OnInitialize はウィンドウを作る (UI スレッドのメッセージループが要る) ので、宣言だけを見る
///				  ・開発ビルドの EditorRemoteServer / SocketScheduler は待ち受けと受信スレッドを起こすので登録しない。
///				    AssetManager だけを登録し、未登録の依存として EditorRemoteServer が報告されることで宣言を確かめる
///				    (どの OnInitialize よりも前に失敗するので、ロードスレッドも起きない)
///				  ・Master の AssetManager は依存を持たないので単独で初期化が通る (ロードスレッドを起こし、World の破棄で止める)
///				  ・GarbageCollector は依存を持たず、初期化は登録先を作るだけなので単独で初期化まで走らせる。
///				    FrameGC が生成コードの表に Presentation の排他ノードとして載ることもここで見る
///				    (garbage_collector.h が core.h 経由で生成器に見えていないと、表に載らず黙って走らなくなる)
///				3 つが揃った構成で実際に起動・終了できることは、CI の「Run runtime.exe」ステップが確かめる。

#include	"pch.h"

//	core_test の pch.h は gtest しか載せていない (test_new_delete.cpp の都合)。
//	core のヘッダは kernel / reflection の基盤型に依存するので、main.cpp と同じ順で先に入れる。
#include	"../../kernel/kernel.h"
#include	"../../reflection/reflection.h"

#include	"../world.h"
#include	"../asset_manager.h"
#include	"../scene_manager.h"
#include	"../garbage_collector.h"
#include	"../service_method.h"

///	@brief	SceneManager は他の Service に依存しない。
TEST(CoreServiceRegistration, SceneManagerHasNoDepends)
{
	const nox::ServiceTypeDescriptor descriptor = nox::MakeServiceTypeDescriptor<nox::SceneManager>();
	EXPECT_EQ(descriptor.type, &nox::reflection::Typeof<nox::SceneManager>());
	EXPECT_TRUE(descriptor.depends.empty());
}

#if NOX_DEVELOP
///	@brief	開発ビルドの AssetManager は EditorRemoteServer だけに依存する。
TEST(CoreServiceRegistration, AssetManagerDependsOnEditorRemoteServerInDevelop)
{
	const nox::ServiceTypeDescriptor descriptor = nox::MakeServiceTypeDescriptor<nox::AssetManager>();
	ASSERT_EQ(descriptor.depends.size(), 1u);
	EXPECT_EQ(descriptor.depends[0], nox::util::GetTypeName<nox::dev::editor_remote::EditorRemoteServer>());
}

///	@brief	開発ビルドで EditorRemoteServer を登録せずに AssetManager を初期化すると、未登録の依存として失敗する。
///	@details	宣言した依存が名前で解決されに行くことの確認。どの OnInitialize よりも前に検出される。
TEST(CoreServiceRegistration, AssetManagerWithoutEditorRemoteServerFailsInDevelop)
{
	nox::World world;
	world.RegisterService(*new nox::AssetManager());

	const nox::ServiceInitializeResult result = world.TryInitializeServices();
	EXPECT_EQ(result.error, nox::ServiceInitializeError::UnresolvedDependency);
	EXPECT_EQ(result.service_type_name, nox::util::GetTypeName<nox::AssetManager>());
	EXPECT_EQ(result.related_type_name, nox::util::GetTypeName<nox::dev::editor_remote::EditorRemoteServer>());
}
#else
///	@brief	Master の AssetManager は依存を持たない (EditorRemoteServer は開発ビルドにしか無い)。
TEST(CoreServiceRegistration, AssetManagerHasNoDependsInMaster)
{
	const nox::ServiceTypeDescriptor descriptor = nox::MakeServiceTypeDescriptor<nox::AssetManager>();
	EXPECT_TRUE(descriptor.depends.empty());
}

///	@brief	Master では AssetManager を単独で初期化でき、World の破棄で終了する (ロードスレッドを止めて join する)。
TEST(CoreServiceRegistration, AssetManagerInitializesAloneInMaster)
{
	nox::World world;
	nox::AssetManager& asset_manager = *new nox::AssetManager();
	world.RegisterService(asset_manager);

	const nox::ServiceInitializeResult result = world.TryInitializeServices();
	ASSERT_TRUE(result.IsSuccess());
	EXPECT_EQ(world.TryGetService<nox::AssetManager>(), &asset_manager);
}
#endif // NOX_DEVELOP

namespace
{
	[[nodiscard]] const nox::ServiceMethodTypeDescriptor* FindGeneratedServiceType(const std::string_view name)noexcept
	{
		for (const nox::ServiceMethodTypeDescriptor* const descriptor : nox::GetServiceMethodTypes())
		{
			if (descriptor->name == name)
			{
				return descriptor;
			}
		}
		return nullptr;
	}
}

///	@brief	GarbageCollector は他の Service に依存しない。
TEST(CoreServiceRegistration, GarbageCollectorHasNoDepends)
{
	const nox::ServiceTypeDescriptor descriptor = nox::MakeServiceTypeDescriptor<nox::GarbageCollector>();
	EXPECT_EQ(descriptor.type, &nox::reflection::Typeof<nox::GarbageCollector>());
	EXPECT_TRUE(descriptor.depends.empty());
}

///	@brief	GarbageCollector の FrameGC は生成コードの表に、Presentation の排他ノード(nox::World& を取る)として載る。
///	@details	解放待ちの一覧を走査・解放している間に、同じフェーズの他のノードが Object を Release したり
///				生のポインタで触れたりしないよう、単独のレイヤーでフェーズを回しているスレッド上で走らせる。
TEST(CoreServiceRegistration, GarbageCollectorFrameGCIsAnExclusivePresentationNode)
{
	const nox::ServiceMethodTypeDescriptor* const descriptor =
		FindGeneratedServiceType(nox::util::GetTypeName<nox::GarbageCollector>());
	ASSERT_NE(descriptor, nullptr) << "GarbageCollector の FrameGC が生成コードの表に載っていません(core.h から見えているか)";
	EXPECT_EQ(descriptor->type, &nox::reflection::Typeof<nox::GarbageCollector>());
	EXPECT_TRUE(descriptor->run_after.empty());
	EXPECT_TRUE(descriptor->run_before.empty());

	const std::span<const nox::ServiceMethodDescriptor> methods = descriptor->get_methods();
	ASSERT_EQ(methods.size(), 1u);
	EXPECT_EQ(methods[0].name, "FrameGC");
	EXPECT_EQ(methods[0].phase, nox::SystemPhaseType::Presentation);
	EXPECT_TRUE(methods[0].exclusive);
	EXPECT_TRUE(methods[0].main_thread_only);
	EXPECT_FALSE(methods[0].emits_structural_change);
	//	World はアクセス宣言に載らない。残るのは自己書き込みだけ。
	ASSERT_EQ(methods[0].get_service_accesses().size(), 1u);
	EXPECT_EQ(methods[0].get_service_accesses()[0].type, &nox::reflection::Typeof<nox::GarbageCollector>());
	EXPECT_TRUE(methods[0].get_service_accesses()[0].write);
}

///	@brief	GarbageCollector は単独で初期化でき、World の破棄で終了する(登録先を作って破棄するだけ)。
TEST(CoreServiceRegistration, GarbageCollectorInitializesAlone)
{
	nox::World world;
	nox::GarbageCollector& garbage_collector = *new nox::GarbageCollector();
	world.RegisterService(garbage_collector);

	const nox::ServiceInitializeResult result = world.TryInitializeServices();
	ASSERT_TRUE(result.IsSuccess());
	EXPECT_EQ(world.TryGetService<nox::GarbageCollector>(), &garbage_collector);
}
