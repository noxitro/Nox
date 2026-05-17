#pragma once
#include	<string_view>
#include	<functional>

#include	"../basic_definition.h"
#include	"memory_definition.h"
#include	"../reflection_attribute.h"

namespace nox::memory
{
	namespace detail
	{
		/// @brief		メモリセグメント指定開始
		/// @details	ScopeMemorySegmentから呼ばれることを想定
		/// @param segment 
		void    BeginMemorySegment(const SegmentType segment);

		/// @brief		メモリセグメント指定終了
		/// @details	ScopeMemorySegmentから呼ばれることを想定
		void    EndMemorySegment();
	}

	/// @brief 初期化
	void	Initialize(std::size_t total_memory_size, bool enabled_profile = false);

	/// @brief 終了処理
	void	Finialize();

	/// @brief ブートメモリの解放
	void	ReleaseBootMemory();

	/// @brief メモリリークチェック
	void	CheckMemoryLeak();

	/// @brief メモリ確保
	/// @param size 要求サイズ
	/// @param alignment アライメント
	/// @param areaType 要求者の識別
	/// @return 確保したアドレス
	void* Allocate(const std::size_t size, std::size_t alignment, const InstanceType instance_type);

	/// @brief メモリ確保 (デフォルトアライメント16)
	inline void* Allocate(const std::size_t size, const InstanceType instance_type)
	{
		return nox::memory::Allocate(size, 16, instance_type);
	}

	/// @brief メモリ解放
	/// @param ptr 解放するアドレス
	void	Deallocate(nox::not_null<void*> ptr);
	void	Deallocate(nox::not_null<void*> ptr, size_t alignment);

	/// @brief メモリ確保したアドレスからヒープ情報を取得
	nox::memory::InstanceType GetInstanceType(nox::not_null<const void*> ptr);
	const nox::memory::HeapInfo& GetHeapInfo(nox::not_null<const void*> ptr)noexcept;

	/// @brief ヒープ情報リストを収集
	/// @param evaluate 評価用関数
	NOX_ATTR(nox::reflection::attr::IgnoreReflection())
	void CollectHeapInfoList(std::move_only_function<void(const nox::memory::HeapInfo&)> evaluate);

	inline constexpr std::u32string_view GetSegmentTypeNameU32(SegmentType segment_type)noexcept
	{
		switch (segment_type)
		{
		case SegmentType::Default: return U"Default";
		case SegmentType::Boot: return U"Boot";
		case SegmentType::Resource: return U"Resource";
		case SegmentType::Render: return U"Render";
		case SegmentType::Develop: return U"Develop";
		default:
			return U"Unknown";
		}
	}

	inline constexpr std::u16string_view GetSegmentTypeNameU16(SegmentType segment_type)noexcept
	{
		switch (segment_type)
		{
		case SegmentType::Default: return u"Default";
		case SegmentType::Boot: return u"Boot";
		case SegmentType::Resource: return u"Resource";
		case SegmentType::Render: return u"Render";
		case SegmentType::Develop: return u"Develop";
		case SegmentType::_Max:
		default:
			return u"Unknown";
		}
	}

	nox::uint32 GetMemorySize(const nox::memory::SegmentType segment)noexcept;
	bool IsHeapPtr(nox::not_null<const void*> ptr)noexcept;

	/// @brief メモリ破壊チェック
	void VerifyMemory();

	/// @brief		スレッド単位のセグメント指定
	/// @details	Defaultセグメントは明示的に指定できません
	template<nox::memory::SegmentType segment> 
		requires(segment != nox::memory::SegmentType::Default)
	struct ScopeMemorySegment
	{
		inline ScopeMemorySegment()
		{
			nox::memory::detail::BeginMemorySegment(segment);
		}

		inline ~ScopeMemorySegment()
		{
			nox::memory::detail::EndMemorySegment();
		}
	};
}