//	Copyright (c) 2023-$year$ noxitro
//	SPDX-License-Identifier: MIT

///	@file	$module$_module_test.cpp
///	@brief	$module$ モジュールのテスト
///	@details	生成直後のひな形。Module のコンストラクタと仮想関数表を参照するので、
///				$module$.lib と、それが依存するライブラリを実際にリンクできることの確認を兼ねる。
///				テストを足すときは、このファイルに書き足すか、新しい *_test.cpp を作って
///				$module$_test.vcxproj (と .filters) に追加する。
#include	"pch.h"

#include	"$runtimeinclude$kernel/kernel.h"
#include	"$runtimeinclude$reflection/reflection.h"
#include	"$runtimeinclude$core/core.h"
#include	"../$module$.h"
#include	"../$module$_module.h"

static_assert(std::is_base_of_v<nox::EngineModule, nox::$module$::Module>);

TEST($modulepascal$ModuleTest, Construct)
{
	const nox::$module$::Module module;
	const nox::reflection::ReflectionObject& object = module;
	EXPECT_TRUE(object.GetType() == nox::reflection::Typeof<nox::$module$::Module>());
}
