//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	singleton.cpp
///	@brief	singleton
#include	"stdafx.h"
#include	"singleton.h"

#include	"assertion.h"
#include	"convert_string.h"
#include	"string_format.h"

#include	"os/mutex.h"
#include	"preprocessor/util.h"

namespace
{
	nox::os::Mutex singleton_mutex_;
}

void	nox::detail::SingletonManager::Register(nox::detail::ISingletonBase& obj)
{
	NOX_LOCAL_SCOPE(nox::os::ScopedLock, singleton_mutex_);
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
		}
	}
}

void	nox::detail::SingletonManager::Unregister(nox::detail::ISingletonBase& obj)
{
	NOX_LOCAL_SCOPE(nox::os::ScopedLock, singleton_mutex_);
	NOX_ASSERT(root_ != nullptr, U"シングルトンが登録されていません");

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
		}
	}
}

void	nox::detail::SingletonManager::CheckReak()
{
	ISingletonBase* current = root_;
	while (current != nullptr)
	{
		NOX_ASSERT(current->prev_ == nullptr, U"リークしているシングルトンがあります");
		current = current->next_;
	}
}

void	nox::detail::CheckSingletonCreateInstance(void* instance_ptr, std::string_view type_name)noexcept(false)
{
	NOX_ASSERT(instance_ptr == nullptr, util::Format(U"インスタンスを生成済みです:{0}", type_name.data()));
}

void	nox::detail::CheckSingletonDeleteInstance(void* instance_ptr, std::string_view type_name)noexcept(false)
{
	NOX_ASSERT(instance_ptr != nullptr, util::Format(U"インスタンスを破棄済みです:{0}", type_name.data()));
}

