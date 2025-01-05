///	@file	mutex_x64.h
///	@brief	mutex_x64
#pragma once
#include	"../windows.h"
#include	"mutex_base.h"
#if NOX_WIN64
namespace nox::os::detail
{
	class MutexWin64 final: public nox::os::detail::MutexBase
	{
	public:
		inline	MutexWin64() :
			criticalSection_(::CRITICAL_SECTION())
		{
			::InitializeCriticalSection(&criticalSection_);
		}

		inline ~MutexWin64()
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
#endif // NOX_WIN64
