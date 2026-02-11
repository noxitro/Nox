//	Copyright (c) 2025 NOX ENGINE All rights reserved.

///	@file	memory_profile.cpp
///	@brief	memory_profile
#include	"stdafx.h"
#include	"memory_profile.h"

#include	"nox_memory.h"

#include	"../stack_trace.h"
#include	"../algorithm.h"
#include	"../os/atomic.h"
#include	"../string_format.h"
#include	"../stack.h"
#include	"../log_trace.h"
#include	"../log_id.h"

namespace nox::memory::profile
{
	namespace
	{
		class Profiler
		{
		private:

		};

		constinit Profiler* g_memory_profiler = nullptr;

		/// @brief プロファイラが初期化済みかどうか
		constinit bool g_is_enabled = false;

		/// @brief 確保済みのプロファイルデータリスト
		inline nox::Vector<ProfileData>& GetProfileDataTable()
		{
			static nox::Vector<ProfileData> profile_data_table;
			return profile_data_table;
		}
//		nox::Vector<ProfileData> g_profile_data_table;

		/// @brief プロファイラハンドルの最大値
		constexpr nox::uint16 k_max_handle = std::numeric_limits<nox::uint16>::max();

		/// @brief プロファイラハンドルのカウンタ
		constinit nox::int32 g_handle_counter = 0;

		/// @brief		プロファイラハンドルスタック
		/// @details	
		nox::FixedStack<nox::uint16, k_max_handle> g_profile_handle_stack = []() {
			nox::FixedStack<nox::uint16, k_max_handle> stack;
			for (nox::uint16 i = k_max_handle - 1; i > 0; --i)
			{
				stack.PushAsync(i);
			}
			return stack;
			}();

		/// @brief ID発行
		/// @return 
		inline nox::uint16 IssueHandle()
		{
			nox::os::atomic::Increment(g_handle_counter);
			const nox::uint16 handle = g_profile_handle_stack.PopAsync();

			//NOX_LOCAL_SCOPE(nox::memory::ScopeMemorySegment<nox::memory::SegmentType::Develop>);
			//NOX_INFO_LINE(nox::log_id::Memory, nox::util::Format(U"MemoryProfileHandle Issue:{0}", handle));
			return handle;
		}

		/// @brief ID返却
		/// @param handle 
		inline void ReleaseHandle(nox::uint16 handle)
		{
			//	NOX_LOCAL_SCOPE(nox::memory::ScopeMemorySegment<nox::memory::SegmentType::Develop>);
			//	NOX_INFO_LINE(nox::log_id::Memory, U"MemoryProfileHandle Release:{0}", handle);

			nox::os::atomic::Decrement(g_handle_counter);
			g_profile_handle_stack.PushAsync(handle);
		}
	}
}

nox::uint16 nox::memory::profile::Register(const nox::memory::HeapInfo& heap_info)
{
	const nox::uint16 handle = nox::memory::profile::IssueHandle();
	nox::memory::profile::ProfileData& profile_data = GetProfileDataTable().at(handle);
	profile_data.handle = heap_info.profile_handle;


	//	コールスタックの取得
	//	アドレスのみ保持しておく
	{
		nox::stack_walker::StackWalkerSlim walker;
		walker.Collect(1);

		const std::span<const nox::stack_walker::SlimStackFrame> stack_list = walker.GetStackList();
		for (nox::uint8 i = 0; i < stack_list.size(); ++i)
		{
			profile_data.call_stack_address_table[i] = stack_list[i].GetAddress();
		}
	}

	return handle;
}

void nox::memory::profile::Unregister(const nox::memory::HeapInfo& heap_info)
{
	const nox::uint16 profiler_handle = heap_info.profile_handle;
	nox::memory::profile::ProfileData& handle_data = GetProfileDataTable().at(profiler_handle);
	nox::memory::profile::ReleaseHandle(profiler_handle);
	handle_data.handle = 0;	//	未使用にする
}

void nox::memory::profile::EnableMemoryProfile()
{
	g_is_enabled = true;

	NOX_LOCAL_SCOPE(nox::memory::ScopeMemorySegment<nox::memory::SegmentType::Develop>{});
	GetProfileDataTable().resize(1024 * 1024);
}

void nox::memory::profile::DisableMemoryProfile()
{
	GetProfileDataTable() = {};
	g_is_enabled = false;
}

bool nox::memory::profile::EnabledMemoryProfile()
{
	return g_is_enabled;
}

const nox::memory::profile::ProfileData& nox::memory::profile::FindProfileData(nox::not_null<const void*> addr)
{
	return nox::memory::profile::FindProfileData(nox::memory::GetHeapInfo(addr).profile_handle);
}

const nox::memory::profile::ProfileData& nox::memory::profile::FindProfileData(nox::uint16 profiler_handle)
{
	//	線形検索
	decltype(auto) handle_data = GetProfileDataTable().at(profiler_handle);
	if (handle_data.handle != 0)
	{
		return handle_data;
	}

	NOX_ASSERT(false, nox::util::Format(u"プロファイルデータが見つかりませんでした handle:{0}", profiler_handle));
	return *(ProfileData*)nullptr;
}