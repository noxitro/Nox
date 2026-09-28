//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	garbage_collector.cpp
///	@brief	garbage_collector
#include	"pch.h"
#include	"garbage_collector.h"

#include	"object.h"

namespace nox
{
	class GarbageCollector::Impl
	{
	public:
		nox::os::Mutex mutex_;
		nox::Vector<std::reference_wrapper<class nox::Object>> destroy_objects_;
		nox::Vector<std::reference_wrapper<class nox::Object>> managed_objects_;
	};
}

void	nox::GarbageCollector::Register(nox::Object& managed_object)
{
	//	任意のスレッドから呼ばれるので、managed_objects_ への追加はロックする。
	//	(以前は nox::os::Mutex をコピーして作っていたので、実際にはロックしていなかった)
	NOX_LOCAL_SCOPE(nox::os::ScopedLock(impl_->mutex_));
	impl_->managed_objects_.emplace_back(managed_object);
}

bool	nox::GarbageCollector::OnInitialize([[maybe_unused]] nox::ServiceContext& context)noexcept
{
	//	登録先はクラスに1つ。同じ World の中の二重登録は DuplicateService で弾かれるので、
	//	ここへ来るのは別の World が GarbageCollector を初期化したままのときだけ。登録先を取り合わないよう起動を止める。
	if (impl_ != nullptr)
	{
		NOX_ASSERT(false, u8"GarbageCollectorが2つ初期化されました(登録先はクラスに1つしか持てません)");
		return false;
	}
	impl_ = new Impl();
	return true;
}

void	nox::GarbageCollector::OnShutdown()noexcept
{
	delete impl_;
	impl_ = nullptr;
}

void	nox::GarbageCollector::FrameGC([[maybe_unused]] nox::World& world)
{
	//	destroy_objects_ に触れるのは FrameGC だけなのでロックは要らない。
	//	参照が戻っていた Object は Release の中から Register が呼ばれうるので、ロックを持ったまま Release しない。
	if (impl_->destroy_objects_.size() > 0)
	{
		for (nox::Object& managed_object : impl_->destroy_objects_)
		{
			nox::detail::ObjectImpl::Release(managed_object);
		}
		impl_->destroy_objects_.clear();
	}
	
	//	managed_objects_ は Register が任意のスレッド(ロードスレッドなど、フェーズの外も含む)から積むので、
	//	走査して移す間はロックする。排他ノードで防げるのは同じフェーズのノードとの競合だけ。
	NOX_LOCAL_SCOPE(nox::os::ScopedLock(impl_->mutex_));
	const auto result = std::ranges::remove_if(impl_->managed_objects_, +[](const nox::Object& managed_object)noexcept 
		{
			return nox::detail::ObjectImpl::GetRefCount(managed_object) < 0;
		});
	
	impl_->destroy_objects_.insert(impl_->destroy_objects_.end(), result.begin(), result.end());
	impl_->managed_objects_.erase(result.begin(), result.end());
}