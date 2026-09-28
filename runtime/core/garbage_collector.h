//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	garbage_collector.h
///	@brief	garbage_collector
#pragma once
#include	"service.h"
#include	"service_attribute.h"

namespace nox
{
	/// @brief		参照が無くなった nox::Object を遅延して解放するService(Object 基盤用。ECS の entity の寿命とは別)。
	/// @details	nox::Object::Release が参照を失った Object を Register で登録し、Presentation の FrameGC が
	///				次のフレームでまとめて解放する。OnInitialize で登録先を作り、OnShutdown で破棄する。
	///
	///				登録先(impl_)は static。Register は Object 基盤から型で引かずに呼ばれるので(nox::Object::Release は
	///				World を知らない)、インスタンスではなくクラスに置いてある。そのため Service のインスタンスが 2 つ
	///				初期化されると登録先を取り合って壊れる。同じ型の二重登録は World が
	///				nox::ServiceInitializeError::DuplicateService で起動前に弾くので、1 つの World の中では起きない
	///				(別の World が初期化したままのときは OnInitialize が false を返して起動を止める)。
	///				OnInitialize より前・OnShutdown より後に Register が呼ばれると登録先が無い(以前の Init / Terminate
	///				フェーズの時点でも同じ制約があった)。
	///
	///				FrameGC は nox::World& を取る排他ノード。Register は任意のスレッド(ノードを実行しているワーカーを含む)から
	///				呼ばれ、登録先の一覧を FrameGC が走査・解放している間に、同じフェーズの他のノードが Object を
	///				Release(= Register) したり、生のポインタで持っている Object に触れたりすると競合する。
	///				同じフェーズの他のノードと並ばないよう、単独のレイヤーで、フェーズを回しているスレッド上で走らせる。
	class GarbageCollector final : public nox::Service
	{
		NOX_DECLARE_OBJECT(nox::GarbageCollector, nox::Service);
	public:
		/// @brief 参照を失った Object を登録する。nox::Object::Release から呼ばれる。任意のスレッドから呼べる。
		static void	Register(class nox::Object& managed_object);

	private:
		/// @brief 登録先を作る。
		bool OnInitialize(nox::ServiceContext& context)noexcept override;
		/// @brief 登録先を破棄する。
		void OnShutdown()noexcept override;

		/// @brief 前のフレームで解放待ちにした Object を解放し、参照を失った Object を解放待ちへ移す。Presentation に1フレーム1回。
		/// @details nox::World& を取る排他ノード(同じフェーズの他のノードと並ばず単独で、フェーズを回しているスレッドで走る)。
		///          World 自体には触れない。排他にするためだけに受け取る(理由はクラスの説明を参照)。
		NOX_ATTR(nox::attr::ServiceMethod(nox::SystemPhaseType::Presentation))
		void FrameGC(nox::World& world);

	private:
		class Impl;
		NOX_ATTR(nox::reflection::attr::IgnoreReflection())
		static inline constinit Impl* impl_ = nullptr;
	};
}