//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	hid_module_test.cpp
///	@brief	hid モジュールのテスト
///	@details	生成直後のひな形。Module のコンストラクタと仮想関数表を参照するので、
///				hid.lib と、それが依存するライブラリを実際にリンクできることの確認を兼ねる。
///				テストを足すときは、このファイルに書き足すか、新しい *_test.cpp を作って
///				hid_test.vcxproj (と .filters) に追加する。
#include	"pch.h"

#include	"../../../kernel/kernel.h"
#include	"../../../reflection/reflection.h"
#include	"../../../core/core.h"
#include	"../hid.h"
#include	"../hid_module.h"

static_assert(std::is_base_of_v<nox::EngineModule, nox::hid::Module>);

TEST(HidModuleTest, Construct)
{
	const nox::hid::Module module;
	const nox::reflection::ReflectionObject& object = module;
	EXPECT_TRUE(object.GetType() == nox::reflection::Typeof<nox::hid::Module>());
}
