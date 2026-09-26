//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	singleton.cpp
///	@brief	singleton
#include	"pch.h"
#include	"singleton.h"

#include	"assertion.h"
#include	"convert_string.h"
#include	"string_format.h"

#include	"os/static_lock.h"
#include	"preprocessor/util.h"

namespace nox
{
	namespace
	{
		/// @brief		シングルトン連結リスト用のロック
		/// @details	シングルトンは名前空間スコープのオブジェクトから生成され得るため、
		///				Register/Unregisterはこのモジュールの初期化子より前にも
		///				静的デストラクタの途中にも呼ばれる。動的初期化が必要な
		///				nox::os::Mutexではその時点でアクセス違反になっていた。
		///				MEMO:	ロック区間で行うのは連結リストのポインタ操作と
		///						Unregister先頭のNOX_ASSERTだけ。アサート経路は
		///						シングルトンの登録/解除へ戻ってこないため、
		///						非再帰ロックで問題ない。
		constinit nox::os::StaticLock singleton_mutex_;
	}
}

void	nox::detail::SingletonManager::Register(nox::detail::ISingletonBase& obj)
{
	NOX_LOCAL_SCOPE(nox::os::ScopedLock{ singleton_mutex_ });
	if (root_ == nullptr)
	{
		root_ = &obj;
	}
	else
	{
		nox::detail::ISingletonBase* current = root_;
		while (current != nullptr)
		{
			if (current->next_ == nullptr)
			{
				current->next_ = &obj;
				obj.prev_ = current;
				break;
			}
			current = current->next_;
		}
	}
}

void	nox::detail::SingletonManager::Unregister(nox::detail::ISingletonBase& obj)
{
	NOX_LOCAL_SCOPE(nox::os::ScopedLock{ singleton_mutex_ });
	NOX_ASSERT(root_ != nullptr, u"シングルトンが登録されていません");

	if (root_->next_ == nullptr)
	{
		root_ = nullptr;
	}
	else
	{
		nox::detail::ISingletonBase* current = root_->next_;
		while (current != nullptr)
		{
			if (current == &obj)
			{
				if (current->prev_ != nullptr)
				{
					current->prev_->next_ = current->next_;
				}
				if (current->next_ != nullptr)
				{
					current->next_->prev_ = current->prev_;
				}
				break;
			}

			current = current->next_;
		}
	}
}

void	nox::detail::SingletonManager::CheckReak()
{
	ISingletonBase* current = root_;
	while (current != nullptr)
	{
		NOX_ASSERT(current->prev_ == nullptr, u"リークしているシングルトンがあります");
		current = current->next_;
	}
}

void	nox::detail::CheckSingletonCreateInstance(void* instance_ptr, std::string_view type_name)noexcept(false)
{
	NOX_ASSERT(instance_ptr == nullptr, u"インスタンスを生成済みです:{0}", type_name.data());
}

void	nox::detail::CheckSingletonDeleteInstance(void* instance_ptr, std::string_view type_name)noexcept(false)
{
	NOX_ASSERT(instance_ptr != nullptr, u"インスタンスを破棄済みです:{0}", type_name.data());
}

