#pragma once

#include	"memory.h"

namespace nox::memory
{
	/// @brief  STLアロケータアダプタ
	/// @tparam T 確保する型
	template<class T>
	class StlAllocateAdapter
	{
	public:
		/// @brief 確保する型
		using value_type = T;

	public:
		// デフォルトコンストラクタ、コピーコンストラクタ、ムーブコンストラクタ
		inline	constexpr StlAllocateAdapter()noexcept {}
		inline	constexpr StlAllocateAdapter(const StlAllocateAdapter&)noexcept {}
		inline	constexpr StlAllocateAdapter(const StlAllocateAdapter&& )noexcept {}
		inline ~StlAllocateAdapter()noexcept = default;

		// 別のテンプレート実引数から生成するためのコンストラクタ
		template <typename U>
		inline	constexpr explicit StlAllocateAdapter(const StlAllocateAdapter<U>&)noexcept {}
		
		template <typename U>
		inline	constexpr explicit StlAllocateAdapter(const StlAllocateAdapter<U>&&)noexcept {}

		/**
		 * @brief メモリ領域を確保
		 * @param num 個数
		 * @return アドレス
		*/
		inline T* allocate(const size_t num)
		{
			return static_cast<T*>(memory::Allocate(sizeof(T) * num, memory::AreaType::Stl).get());
		}

		/**
		 * @brief メモリ領域を解放
		 * @param ptr 解放アドレス
		 * @param num 個数
		*/
		inline void deallocate(T* const ptr, const size_t /*num*/)
		{
			memory::Deallocate(ptr);
		}
	};
}