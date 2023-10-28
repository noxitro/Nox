#pragma once
#include	"../if/basic_type.h"
#include	"../if/basic_definition.h"

#include	"memory_definition.h"

namespace nox::memory
{
	/// <summary>
	/// 
	/// </summary>
	/// <param name="size"></param>
	/// <param name="areaType"></param>
	/// <returns></returns>
	not_null<void*> Allocate(const size_t size, const AreaType areaType);

	/// @brief メモリ解放
	/// @param ptr 解放するアドレス
	void	Deallocate(not_null<void*> ptr);


}