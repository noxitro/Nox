///	@file	mutex_x64.h
///	@brief	mutex_x64
#pragma once
#include	"x64.h"
#include	"mutex.h"

namespace nox::os::detail
{
	class MutexX64 : public MutexBase
	{
	public:
		inline	MutexX64() :
			criticalSection_(::CRITICAL_SECTION())
		{
			::InitializeCriticalSection(&criticalSection_);
		}

		inline ~MutexX64()
		{
			::DeleteCriticalSection(&criticalSection_);
		}

		inline	void	Lock()
		{
			::EnterCriticalSection(&criticalSection_);
		}

		inline	void	Unlock()
		{
			::LeaveCriticalSection(&criticalSection_);
		}

		_When_(return != 0, _Acquires_lock_(criticalSection_))
			inline	bool	TryLock()
		{
			return ::TryEnterCriticalSection(&criticalSection_) == TRUE;
		}


		[[nodiscard]]	inline const ::CRITICAL_SECTION& GetCriticalSection()const noexcept { return criticalSection_; }
	private:
		/**
		 * @brief
		*/
		::CRITICAL_SECTION criticalSection_;
	};
}