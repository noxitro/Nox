#include	"pch.h"
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
#include    "../math/math.h"

namespace nox::memory
{
    namespace
    {
        //  ヒープ情報は32byte以下であることを保証
        static_assert(sizeof(nox::memory::HeapInfo) <= 32);

        constexpr nox::uint8 k_heap_shift = 4;
        constexpr nox::uint8 k_heap_block = 1 << k_heap_shift;

        /// @brief 1スレッドでセグメント指定できる数
        constexpr nox::uint8 k_max_segment_stack = 1 << (std::numeric_limits<nox::uint8>::digits - 1);

        /// @brief 有効なヒープ情報か判定するためのマジックナンバー
        constexpr nox::uint8 k_heap_info_magic = 0x5A;

		/// @brief 大きな割り当てブロックとみなされるしきい値 (アリーナサイズの1/8)
		constexpr nox::uint16 k_large_allocation_block_threshold = nox::memory::Arena::size / 8;

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

		/// @brief エンジンで確保するメモリ領域
        constinit nox::uint8* g_memory_storage = nullptr;

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

        constinit nox::memory::Arena* head_arena_ptr_ = nullptr;

        constinit nox::memory::HeapInfo2* current_zct_heap_info_ = nullptr;

        inline constexpr std::size_t MaskToAlignment(std::size_t align_mask)noexcept
        {
            const std::size_t alignment = (align_mask == 0) ? alignof(std::max_align_t) : (align_mask + 1);
			return alignment;
		}

        inline void* HeapInfoToPtr(const nox::memory::HeapInfo& heap_info_ptr)noexcept
        {
            return reinterpret_cast<void*>(reinterpret_cast<nox::uintptr>(&heap_info_ptr) + sizeof(nox::memory::HeapInfo));
        }

        inline nox::memory::SegmentType GetSegmentType(bool on_dirty_flag = false)
        {
            //  初期化処理が呼ばれるまでに走ったメモリ確保はBootセグメントに属する
            if (g_unique_flag.IsOn(nox::memory::UniqueFlag::Initialized) == false)
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
        inline  nox::memory::HeapInfo& GetHeapInfoImpl(void* ptr)noexcept
        {
            nox::memory::HeapInfo& heapInfo = *reinterpret_cast<nox::memory::HeapInfo*>((nox::uint8*)ptr - sizeof(nox::memory::HeapInfo));
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

        inline  void* GetRawPtrFromHeapInfo(HeapInfo& heap_info)
        {
            const auto alignment = static_cast<size_t>(heap_info.align_size);
            auto* const hip = &heap_info;

            // hip は raw_ptr 以上のアドレス（先頭 or 先頭+α）なので、
            // alignment で下方向に揃えれば raw_ptr に戻せる
            auto addr = reinterpret_cast<nox::uintptr>(hip);

            // align_down
            addr &= ~(static_cast<nox::uintptr>(alignment - 1));

            return reinterpret_cast<void*>(addr);
        }
    }
}

void    nox::memory::Initialize(std::size_t total_memory_sizse, bool enabled_profile)
{
//    g_memory_storage = static_cast<nox::uint8*>(::VirtualAlloc(nullptr, total_memory_sizse, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));

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

    if (g_memory_storage != nullptr)
    {
        ::VirtualFree(g_memory_storage, 0, MEM_RELEASE);
        g_memory_storage = nullptr;
    }
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
	nox::uint32 leak_count = 0;

	nox::memory::CollectHeapInfoList([&leak_count](const nox::memory::HeapInfo& heap_info)
		{
			//  bootセグメントのリークは無視
            if (heap_info.segment_type == nox::memory::SegmentType::Boot)
            {
                return;
			}

			const nox::memory::SegmentType segment_type = heap_info.segment_type;
			NOX_ERROR_LINE(nox::log_id::Memory, u"Memory Leak Detected\n\tsegment:{0}, size:{1}", nox::memory::GetSegmentTypeNameU32(segment_type), heap_info.size);

            if (heap_info.profile_handle == 0)
            {
                return;
            }

            ++leak_count;

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

    if (leak_count > 0)
    {
		NOX_ERROR_LINE(nox::log_id::Memory, u"Total Memory Leak Count: {0}", leak_count);
    }
}

void nox::memory::CollectHeapInfoList(std::move_only_function<void(const nox::memory::HeapInfo&)> evaluate)
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
    return static_cast<nox::memory::HeapInfo&(*)(void*)>(&nox::memory::GetHeapInfoImpl)(const_cast<void*>(ptr.get()));
}

namespace nox::memory
{
    namespace
    {
        /// @brief 1MB以上の大きなメモリ確保用
        /// @param size 
        /// @param align 
        /// @return 
        inline void* AllocateGlobal(const std::size_t size, const std::size_t align)
        {
            //  アライメントチェック
            NOX_ASSERT(align >= 1 || nox::math::IsPow2(align), u"アライメントサイズは2の累乗で指定してください");

            const auto align_mask = align > 0 ? align - 1 : 0;


        }


        inline void* Allocate2(const size_t size, size_t align_mask, const InstanceType instance_type)
        {
            NOX_ASSERT(g_unique_flag.IsOn(UniqueFlag::Finalized) == false,
                u"メモリアロケータの終了処理後にメモリ確保が行われました");

            // HeapInfo 分 + ユーザ要求サイズ分
            const size_t header_size = sizeof(HeapInfo);
            const size_t raw_size = header_size + size;

            // ヒープの最小ブロック単位で丸める（既存ロジックを維持）
            const nox::uint32 use_block =
                static_cast<nox::uint32>((raw_size + (k_heap_block - 1)) >> k_heap_shift);
            const nox::uint32 alloc_size = use_block << k_heap_shift;

            void* const raw_ptr = std::malloc(alloc_size);
            NOX_ASSERT(raw_ptr != nullptr, u"メモリ確保に失敗しました");

            const nox::memory::SegmentType segment_type = GetSegmentType(true);

            // HeapInfo は常に先頭に配置
            auto* const heap_info_ptr = static_cast<HeapInfo*>(raw_ptr);
            HeapInfo& heap_info = *heap_info_ptr;

            // ユーザポインタ（HeapInfo 直後）
            auto* const user_ptr_u8 = reinterpret_cast<nox::uint8*>(heap_info_ptr) + sizeof(HeapInfo);
            void* const aligned_ptr = static_cast<void*>(user_ptr_u8);

            // HeapInfo 設定
            heap_info.size = alloc_size;
            heap_info.align_size = 0; // 追加アラインをしていないので 0 か sizeof(HeapInfo) 程度に
            heap_info.instance_type = instance_type;
            heap_info.segment_type = segment_type;
            heap_info.magic = k_heap_info_magic;

            info_with_segment_table_[nox::util::ToUnderlying(heap_info.segment_type)].total_size
                += heap_info.size;

            if (segment_type != nox::memory::SegmentType::Develop &&
                memory::profile::EnabledMemoryProfile())
            {
                heap_info.profile_handle = memory::profile::Register(heap_info);
            }
            else
            {
                heap_info.profile_handle = 0;
            }

            // 連結リストに接続
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

        /// @brief ヒープ情報からユーザサイズを取得
        inline constexpr std::size_t GetUserSize(const nox::memory::HeapInfo2& heap_info)noexcept
        {
            //  block数からbyte数に変換
            const std::size_t use_byte_size = static_cast<std::size_t>(heap_info.use_block) << k_heap_shift;
            const std::size_t header_padding = static_cast<std::size_t>(heap_info.align_block + 1) * k_heap_block;
            const std::size_t user_byte_size = use_byte_size - header_padding - sizeof(HeapInfo2);
            return user_byte_size;
        }


        inline void* AllocateLocal(const std::size_t size, const std::size_t align_mask)
        {
            return nullptr;
        }
    }
}

#if false

void* nox::memory::Allocate(const std::size_t size, std::size_t alignment, const InstanceType instance_type)
{
    //  メモリはblock(16byte)単位で確保する
	//  つまり最小確保サイズはheaderを含めて32byte以上になる
    
    //  アライメントチェック
    NOX_ASSERT(alignment >= 1 || nox::math::IsPow2(alignment), u"アライメントサイズは2の累乗で指定してください");

    const auto align_mask = alignment > 0 ? alignment - 1 : 0;

    void* raw_ptr;

    if (size >= nox::memory::k_large_allocation_block_threshold)
    {
		//  大きいメモリ確保
        raw_ptr = nullptr;
    }
    else
    {
        raw_ptr = nox::memory::AllocateLocal(size, align_mask);
    }


   // constexpr nox::uint32 arena_body_size = sizeof(Arena::body);
//	constexpr nox::uint32 arena_body_block = arena_body_size >> k_heap_shift;

    constexpr nox::uint32 block = 16;


//    const nox::uint32 align_block = 

	//const nox::uint32 use_block = (size + sizeof(HeapInfo) + (k_heap_block - 1)) >> k_heap_shift;

    //  必要ブロックを計算
    //  header + padding + user_size
    
    //TODO:  大きなメモリ割り当ては未実装
    return nullptr;
}

#else
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
#endif // false

void	nox::memory::Deallocate(nox::not_null<void*> ptr)
{
    HeapInfo& heap_info = GetHeapInfoImpl(ptr.get());

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