#include	"stdafx.h"
#include	"nox_memory.h"

#include	"../algorithm.h"

#include	"../os/thread.h"
#include    "../os/mutex.h"
#include    "../os/atomic.h"
#include    "../os/os_utility.h"
#include    "../log_id.h"
#include    "memory_profile.h"
#include    "../bit_flag.h"
#include    "../stack.h"
#include    "../stack_trace.h"

namespace nox::memory
{
	//  ヒープ情報は32byte以下であることを保証
	static_assert(sizeof(nox::memory::HeapInfo) <= 32);

    constexpr nox::uint8 k_heap_shift = 4;
    constexpr nox::uint8 k_heap_block = 1 << k_heap_shift;

	/// @brief 1スレッドでセグメント指定できる数
	constexpr nox::uint8 k_max_segment_stack = 1 << (std::numeric_limits<nox::uint8>::digits - 1);

	/// @brief 有効なヒープ情報か判定するためのマジックナンバー
	constexpr nox::uint8 k_heap_info_magic = 0x5A;

    /// @brief  スレッドに紐づく情報
    struct ThreadMemoryInfo
    {
        //  セグメントスタック
        //  MEMO:   デフォルトを指定できるようにカウンタを1にしておく
        nox::uint8 segment_counter : 7 = 1;

		/// @brief      セグメント指定後にメモリ確保が行われたかどうか
        /// @details    意味のあるセグメント指定だったか判定する
        bool is_dirty : 1 = false;

		/// @brief セグメントスタック
        std::array<nox::memory::SegmentType, k_max_segment_stack> segment_stack{ nox::memory::SegmentType::Default };
    };

    /// @brief セグメントごとの情報
	struct InfoWithSegment
    {
        /// @brief 合計サイズ
        nox::uint32 total_size = 0;

        /// @brief 
        std::array<nox::uint32, nox::util::ToUnderlying(nox::memory::InstanceType::_Max)> size_with_instance_type;
    };

	/// @brief      memory::Initializeが呼ばれたかどうか
	/// @details    Initializeが呼ばれるまでBootセグメントに属する
    enum class UniqueFlag : nox::uint8
    {
        None,
        Initialized,
		Finalized,
    };
  
	constinit nox::BitFlag<UniqueFlag> g_unique_flag;

	/// @brief ヒープ情報を接続する時用のMutex
    nox::os::Mutex mutex_;

	/// @brief ヒープ情報の先頭
	constinit HeapInfo* head_heap_info_ptr_ = nullptr;

	/// @brief ヒープ情報の末尾
	constinit HeapInfo* tail_heap_info_ptr_ = nullptr;

	/// @brief スレッドごとの情報テーブル
	constinit std::array<ThreadMemoryInfo, nox::os::MAX_THREAD_ID> k_thread_memory_info_table_{};

	/// @brief セグメントごとの情報テーブル
	constinit std::array< InfoWithSegment, nox::util::ToUnderlying(nox::memory::SegmentType::_Max)> info_with_segment_table_{};

    inline void* HeapInfoToPtr(const nox::memory::HeapInfo& heap_info_ptr)noexcept
    {
		return reinterpret_cast<void*>(reinterpret_cast<nox::uintptr>(&heap_info_ptr) + sizeof(nox::memory::HeapInfo));
    }

    inline nox::memory::SegmentType GetSegmentType(bool on_dirty_flag = false)
    {
		//  初期化処理が呼ばれるまでに走ったメモリ確保はBootセグメントに属する
		if (g_unique_flag.IsOn(nox::memory::UniqueFlag::Initialized))
		{
			return nox::memory::SegmentType::Boot;
		}

        const nox::int8 thread_id = nox::os::Thread::GetThreadId();
        nox::memory::ThreadMemoryInfo& thread_memory_info = k_thread_memory_info_table_[thread_id];
        if (on_dirty_flag)
        {
            thread_memory_info.is_dirty = on_dirty_flag;
        }
		return thread_memory_info.segment_stack[thread_memory_info.segment_counter - 1];
    }

	/// @brief 有効なHeapInfoか
    /// @param heap_info 
    /// @return 
    inline constexpr bool IsValidHeapInfo(const nox::memory::HeapInfo& heap_info)noexcept
    {
		return heap_info.magic == k_heap_info_magic;
    }

	/// @brief アドレスからHeapInfoを取得
    /// @param ptr 
    /// @return 
    inline  nox::memory::HeapInfo& GetHeapInfo(nox::not_null<void*> ptr)noexcept
    {
        nox::memory::HeapInfo& heapInfo = *reinterpret_cast<nox::memory::HeapInfo*>((nox::uint8*)ptr.get() - sizeof(nox::memory::HeapInfo));
        //NOX_ASSERT(nox::memory::IsValidHeapInfo(heapInfo) == true, U"Invalid Heap Info");
        return heapInfo;
    }

  //  inline nox::memory::HeapInfo* GetNextHeapInfo(const nox::memory::HeapInfo& heap_info)noexcept
  //  {
		//if (heap_info.next_offset == 0)
		//{
		//	return nullptr;
		//}
		//return reinterpret_cast<nox::memory::HeapInfo*>(reinterpret_cast<nox::uintptr>(&heap_info) + heap_info.next_offset);
  //  }

  //  inline nox::memory::HeapInfo* GetPrevHeapInfo(const nox::memory::HeapInfo& heap_info)noexcept
  //  {
  //      if (heap_info.prev_offset == 0)
  //      {
		//	return nullptr;
  //      }
		//return reinterpret_cast<nox::memory::HeapInfo*>(reinterpret_cast<nox::uint8>(&heap_info) - heap_info.prev_offset);
  //  }
}

void    nox::memory::Initialize(bool enabled_profile)
{
	//  stack_walker::Initialize();
    nox::stack_walker::Initialize();

	//  初期化処理が呼ばれるまでに走ったメモリ確保はBootセグメントに属する
	g_unique_flag.On(UniqueFlag::Initialized);

	if (enabled_profile)
	{
		nox::memory::profile::EnableMemoryProfile();
	}
}

void    nox::memory::Finialize()
{
    nox::memory::profile::DisableMemoryProfile();
	g_unique_flag.On(UniqueFlag::Finalized);

    nox::stack_walker::Finalize();
}

void	nox::memory::ReleaseBootMemory()
{
    return;
#if false 

    //  先頭から末尾までチェック
	for (const nox::memory::HeapInfo* work_ptr = nox::memory::head_heap_info_ptr_; work_ptr != nullptr;)
	{
		//  Deallocateで解放されるので、次のアドレスを取得しておく
		nox::memory::HeapInfo*const next_heap_info_ptr = work_ptr->next;

		if (work_ptr->segment_type == nox::memory::SegmentType::Boot)
		{
            void* addr = nox::memory::HeapInfoToPtr(*work_ptr);
            delete addr;
		}
        work_ptr = next_heap_info_ptr;
	}
#endif // false
}

void	nox::memory::CheckMemoryLeak()
{
	nox::memory::CollectHeapInfoList(+[](const nox::memory::HeapInfo& heap_info)
		{
			const nox::memory::SegmentType segment_type = heap_info.segment_type;
			NOX_ERROR_LINE(nox::log_id::Memory, nox::util::Format(u"Memory Leak Detected\n\tsegment:{0}, size:{1}", nox::memory::GetSegmentTypeNameU32(segment_type), heap_info.size));

            if (heap_info.profile_handle == 0)
            {
                return;
            }

            //  プロファイラに登録されているならコールスタックを出力
            if (nox::memory::profile::EnabledMemoryProfile())
            {
				const nox::memory::profile::ProfileData& profile_data = nox::memory::profile::FindProfileData(heap_info.profile_handle);
				const nox::uint8 stack_length = static_cast<nox::uint8>(profile_data.call_stack_address_table.size());
                //  有効なスタックトレースの数
                nox::uint8 enabledStackCount = 0;

                for (nox::int32 i = 0; i < stack_length; ++i)
                {
                    //  無効なコールスタックアドレスがあれば、そこで終了
					if (profile_data.call_stack_address_table[i] == 0)
					{
						break;
					}

                    ++enabledStackCount;
                }

                const std::span<const size_t> address_list = std::span<const size_t>(profile_data.call_stack_address_table.data(), enabledStackCount);
                nox::stack_walker::Trace(address_list);
            }
		});
}

void nox::memory::CollectHeapInfoList(nox::not_null<void(*)(const nox::memory::HeapInfo&)> evaluate)
{
	for (const nox::memory::HeapInfo* work_ptr = nox::memory::head_heap_info_ptr_; work_ptr != nullptr; work_ptr = work_ptr->next)
	{
		evaluate(*work_ptr);
	}
}

void    nox::memory::detail::BeginMemorySegment(const SegmentType segment)
{
    const auto thread_id = nox::os::Thread::GetThreadId();
	nox::memory::ThreadMemoryInfo& thread_memory_info = k_thread_memory_info_table_[thread_id];
	thread_memory_info.segment_stack[thread_memory_info.segment_counter++] = segment;

    NOX_ASSERT(thread_memory_info.segment_counter < nox::memory::k_max_segment_stack, u"segment stack over");

    thread_memory_info.is_dirty = false;
}

void    nox::memory::detail::EndMemorySegment()
{
    const auto thread_id = nox::os::Thread::GetThreadId();
    nox::memory::ThreadMemoryInfo& thread_memory_info = k_thread_memory_info_table_[thread_id];
    --thread_memory_info.segment_counter;

    if (thread_memory_info.is_dirty==false)
    {
        NOX_WARNING_LINE(nox::log_id::Memory, u"セグメント指定されましたが、メモリ確保はされませんでした");
    }
}

nox::memory::InstanceType nox::memory::GetInstanceType(nox::not_null<const void*> ptr)
{
	return nox::memory::GetHeapInfo(ptr).instance_type;
}

const nox::memory::HeapInfo& nox::memory::GetHeapInfo(nox::not_null<const void*> ptr)noexcept
{
    return static_cast<nox::memory::HeapInfo&(*)(nox::not_null<void*>)>(&nox::memory::GetHeapInfo)(const_cast<void*>(ptr.get()));
}

void* nox::memory::Allocate(const size_t size, size_t align_mask, const InstanceType instance_type)
{
	NOX_ASSERT(g_unique_flag.IsOn(UniqueFlag::Finalized) == false, u"メモリアロケータの終了処理後にメモリ確保が行われました");

    const nox::uint32 align_size = align_mask <= sizeof(HeapInfo) ? sizeof(HeapInfo) : static_cast<nox::uint32>(sizeof(HeapInfo)) + align_mask;
  //  const nox::uint32 align_size = sizeof(HeapInfo) + align_mask;
    const nox::uint32 use_block = (size + align_size + (k_heap_block - 1)) >> k_heap_shift;
    const nox::uint32 alloc_size = use_block << k_heap_shift;

    void* const heap_info_ptr = std::malloc(alloc_size);

    nox::memory::SegmentType segment_type;
    //  初期化処理が呼ばれるまでに走ったメモリ確保はBootセグメントに属する
    if (g_unique_flag.IsOn(nox::memory::UniqueFlag::Initialized) == false)
    {
        segment_type = nox::memory::SegmentType::Boot;
    }
    else
    {
        const nox::int8 thread_id = nox::os::Thread::GetThreadId();
        nox::memory::ThreadMemoryInfo& thread_memory_info = k_thread_memory_info_table_[thread_id];
        thread_memory_info.is_dirty = true;
        segment_type = thread_memory_info.segment_stack[thread_memory_info.segment_counter - 1];
    }

    HeapInfo& heap_info = *reinterpret_cast<HeapInfo*>(heap_info_ptr);
    heap_info.size = alloc_size;
	heap_info.align_size = align_size;
    heap_info.instance_type = instance_type;
	heap_info.segment_type = segment_type;
	heap_info.magic = k_heap_info_magic;

    void*const aligned_ptr = reinterpret_cast<void*>(reinterpret_cast<nox::uint8*>(heap_info_ptr) + sizeof(HeapInfo));

	info_with_segment_table_[nox::util::ToUnderlying(heap_info.segment_type)].total_size += heap_info.size;

    //  Profilerへの登録
    if (segment_type != nox::memory::SegmentType::Develop && memory::profile::EnabledMemoryProfile())
    {
		heap_info.profile_handle = memory::profile::Register(heap_info);
    }
    else 
    {
        heap_info.profile_handle = 0;
    }

    //  連結
    {
		nox::os::ScopedLock lock(mutex_);

        if (nox::memory::head_heap_info_ptr_ == nullptr)
        {
            head_heap_info_ptr_ = &heap_info;
			tail_heap_info_ptr_ = &heap_info;
            heap_info.next = heap_info.prev = nullptr;
		}
        else
        {
            heap_info.next = nullptr;
            heap_info.prev = tail_heap_info_ptr_;

            tail_heap_info_ptr_->next = &heap_info;
            tail_heap_info_ptr_ = &heap_info;
        }
    }

    return aligned_ptr;
}

void	nox::memory::Deallocate(nox::not_null<void*> ptr)
{
    HeapInfo& heap_info = GetHeapInfo(ptr);

    info_with_segment_table_[nox::util::ToUnderlying(heap_info.segment_type)].total_size -= heap_info.size;

    if (
        heap_info.segment_type != nox::memory::SegmentType::Develop && 
        heap_info.segment_type != nox::memory::SegmentType::Boot && 
        memory::profile::EnabledMemoryProfile()
        )
    {
		//  profile_handleが0の時は、プロファイラに登録されていない
        memory::profile::Unregister(heap_info);
    }

    {
        nox::os::ScopedLock lock(mutex_);

        if (&heap_info == head_heap_info_ptr_)
        {
			head_heap_info_ptr_ = heap_info.next;
        }
        if (&heap_info == tail_heap_info_ptr_)
        {
			tail_heap_info_ptr_ = heap_info.prev;
        }

        //  連結解除
		if (heap_info.prev != nullptr)
		{
            heap_info.prev->next = heap_info.next;
		}
        if (heap_info.next != nullptr)
        {
            heap_info.next->prev = heap_info.prev;
        }
    }
	std::free(&heap_info);
}

void	nox::memory::Deallocate(nox::not_null<void*> ptr, [[maybe_unused]] size_t alignment)
{
    Deallocate(ptr);

    //HeapInfo* heap_info_ptr = &GetHeapInfo(ptr);
	//::_aligned_free(heap_info_ptr);
}

nox::uint32 nox::memory::GetMemorySize(const nox::memory::SegmentType segment)noexcept
{
	return nox::os::atomic::Read(info_with_segment_table_[nox::util::ToUnderlying(segment)].total_size);
}

void nox::memory::VerifyMemory()
{
    nox::memory::CollectHeapInfoList([](const nox::memory::HeapInfo& heap_info)noexcept {
		NOX_ASSERT(nox::memory::IsValidHeapInfo(heap_info) == true, u"Invalid Heap Info");
        });
}