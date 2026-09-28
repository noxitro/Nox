//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	core_service_registration_test.cpp
///	@brief	Core が登録する Service (SceneManager / AssetManager) の Depends の宣言と解決の検証。
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
///				3 つが揃った構成で実際に起動・終了できることは、CI の「Run runtime.exe」ステップが確かめる。

#include	"pch.h"

//	core_test の pch.h は gtest しか載せていない (test_new_delete.cpp の都合)。
//	core のヘッダは kernel / reflection の基盤型に依存するので、main.cpp と同じ順で先に入れる。
#include	"../../kernel/kernel.h"
#include	"../../reflection/reflection.h"

#include	"../world.h"
#include	"../asset_manager.h"
#include	"../scene_manager.h"

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
