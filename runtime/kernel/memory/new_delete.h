//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file		new_delete.h
///	@brief		new,deleteのオーバーロード
/// @details	自前のAllocatorを使用するためのオーバーロード
///				inline展開させると、ビルドエラーになるのでcppに分離

#pragma once
#include	"nox_memory.h"
#if NOX_WIN64

#include	"../os/windows.h"
_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(_Size) _VCRT_ALLOCATOR
void* __CRTDECL operator new(std::size_t _Size)
{
	return nox::memory::Allocate(_Size, nox::memory::InstanceType::Other);
}

_NODISCARD _Ret_maybenull_ _Success_(return != NULL) _Post_writable_byte_size_(_Size) _VCRT_ALLOCATOR
void* __CRTDECL operator new(size_t _Size, ::std::nothrow_t const&) noexcept
{
	return nox::memory::Allocate(_Size, nox::memory::InstanceType::Other);
}

_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(_Size) _VCRT_ALLOCATOR
void* __CRTDECL operator new(std::size_t _Size, ::std::align_val_t align)
{
	return nox::memory::Allocate(_Size, static_cast<size_t>(align), nox::memory::InstanceType::Other);
}

_NODISCARD _Ret_maybenull_ _Success_(return != NULL) _Post_writable_byte_size_(_Size) _VCRT_ALLOCATOR
void* __CRTDECL operator new(size_t _Size, ::std::align_val_t      _Al, ::std::nothrow_t const&) noexcept
{
	return nox::memory::Allocate(_Size, static_cast<size_t>(_Al), nox::memory::InstanceType::Other);
}

_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(_Size) _VCRT_ALLOCATOR
void* __CRTDECL operator new[](std::size_t _Size)
{
	return nox::memory::Allocate(_Size, nox::memory::InstanceType::Other);
}

_NODISCARD _Ret_maybenull_ _Success_(return != NULL) _Post_writable_byte_size_(_Size) _VCRT_ALLOCATOR
void* __CRTDECL operator new[](size_t _Size, ::std::nothrow_t const&) noexcept
{
	return nox::memory::Allocate(_Size, nox::memory::InstanceType::Other);
}

_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(_Size) _VCRT_ALLOCATOR
void* __CRTDECL operator new[](std::size_t _Size, ::std::align_val_t align)
{
	return nox::memory::Allocate(_Size, static_cast<size_t>(align), nox::memory::InstanceType::Other);
}

void __CRTDECL operator delete(void* blockPtr) noexcept
{
	if (blockPtr == nullptr)
	{
		return;
	}
	nox::memory::Deallocate(blockPtr);
}

void __CRTDECL operator delete(void* _Block, ::std::align_val_t _Al) noexcept
{
	if (_Block == nullptr)
	{
		return;
	}
	nox::memory::Deallocate(_Block, static_cast<size_t>(_Al));
}

void __CRTDECL operator delete(void* _Block, ::std::align_val_t      _Al, ::std::nothrow_t const&) noexcept
{
	if (_Block == nullptr)
	{
		return;
	}
	nox::memory::Deallocate(_Block, static_cast<size_t>(_Al));
}

void __CRTDECL operator delete[](void* _Block) noexcept
{
	if (_Block == nullptr)
	{
		return;
	}
	nox::memory::Deallocate(_Block);
}

void __CRTDECL operator delete[](void* _Block, ::std::align_val_t _Al) noexcept
{
	if (_Block == nullptr)
	{
		return;
	}
	nox::memory::Deallocate(_Block, static_cast<size_t>(_Al));
}

void __CRTDECL operator delete[](void* _Block, ::std::align_val_t      _Al, ::std::nothrow_t const&) noexcept
{
	if (_Block == nullptr)
	{
		return;
	}
	nox::memory::Deallocate(_Block, static_cast<size_t>(_Al));
}

#endif // NOX_WIN64
