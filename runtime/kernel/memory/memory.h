#pragma once
#include	"../basic_definition.h"
#include	"memory_definition.h"

namespace nox::memory
{
	/// <summary>
	/// 
	/// </summary>
	/// <param name="size"></param>
	/// <param name="areaType"></param>
	/// <returns></returns>
	void* Allocate(const size_t size, const nox::memory::AreaType areaType);
	void* Allocate(const size_t size, size_t alignment, const AreaType areaType);

	/// @brief メモリ解放
	/// @param ptr 解放するアドレス
	void	Deallocate(nox::not_null<void*> ptr);
	void	Deallocate(nox::not_null<void*> ptr, size_t alignment);


}