//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	behavior.cpp
///	@brief	behavior
#include	"stdafx.h"
#include	"behavior.h"

namespace
{
//	inline constexpr bool hasFunctionPointer(const std::span<void(*)()> v_table, const nox::uint64 function_id)noexcept
//	{
//		for (void(*v)() : v_table)
//		{
//			const nox::uint64 id = reinterpret_cast<nox::uint64>(v);
//
//			if (id == function_id)
//			{
//				return true;
//			}
//		}
//
//		return false;
//	}
//
//	template<nox::concepts::EveryFunctionType T>
//	inline constexpr bool hasFunctionPointer(const std::span<void(*)()> v_table, T func_addr)noexcept
//	{
//		return hasFunctionPointer(v_table, nox::util::GetFunctionPointerID(func_addr));
//	}
//
//	inline std::span<const void*const> GetVTable2(nox::not_null<const void*> ptr)
//	{
//		const void* const* vt = *reinterpret_cast<const void* const*const*>(ptr.get());
//		nox::int32 counter = 0;
//		while (vt[counter] != nullptr)
//		{
//			++counter;
//		}
//
//		return std::span(vt, counter);
//	}
}

nox::Behavior::Behavior():
	enabled_function_types_(FunctionType::NONE)
{

}

nox::Behavior::~Behavior()
{

}

void	nox::Behavior::Loaded()
{
}

void	nox::Behavior::UnLoaded()
{

}