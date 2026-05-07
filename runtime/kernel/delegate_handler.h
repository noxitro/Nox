// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	delegate_handler.h
/// @brief	delegate_handler
#pragma once
#include	"advanced_type.h"
//#include	"os/mutex.h"
#include	"os/read_write_lock.h"

namespace nox
{
	class DelegateHandlerBase;
	struct DelegateHandle
	{
	public:
		inline constexpr DelegateHandle() noexcept :
			owner(nullptr)
		{
		}

		~DelegateHandle();
		void Dispose();

	private:
		nox::DelegateHandlerBase* owner;
	};

	class DelegateHandlerBase
	{
		friend struct DelegateHandle;
	private:
		virtual void Release(DelegateHandle& handle) = 0;
	};

	template<class F>
	class DelegateHandler : public DelegateHandlerBase
	{
	public:
		nox::DelegateHandle Add()
		{
		}

		void Release()
		{

		}

	private:
		nox::Vector<DelegateHandle> handle_list_;
		nox::Vector<std::function<F>> delegate_list_;
		nox::os::ReadWriteLock rw_lock_;
	};
}