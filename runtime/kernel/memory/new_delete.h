//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file		new_delete.h
///	@brief		new,deleteのオーバーロード
/// @details	自前のAllocatorを使用するためのオーバーロード
///				inline展開させると、ビルドエラーになるのでcppに分離
#pragma once
#include	"memory.h"
#if NOX_WIN64

#include	"../os/windows.h"
_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(_Size) _VCRT_ALLOCATOR
void* __CRTDECL operator new(std::size_t _Size);

_NODISCARD _Ret_maybenull_ _Success_(return != NULL) _Post_writable_byte_size_(_Size) _VCRT_ALLOCATOR
void* __CRTDECL operator new(size_t _Size, ::std::nothrow_t const&) noexcept;

_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(_Size) _VCRT_ALLOCATOR
void* __CRTDECL operator new(std::size_t _Size, ::std::align_val_t align);

_NODISCARD _Ret_maybenull_ _Success_(return != NULL) _Post_writable_byte_size_(_Size) _VCRT_ALLOCATOR
void* __CRTDECL operator new(size_t _Size, ::std::align_val_t      _Al, ::std::nothrow_t const&) noexcept;

_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(_Size) _VCRT_ALLOCATOR
void* __CRTDECL operator new[](std::size_t _Size);

_NODISCARD _Ret_maybenull_ _Success_(return != NULL) _Post_writable_byte_size_(_Size) _VCRT_ALLOCATOR
void* __CRTDECL operator new[](size_t _Size, ::std::nothrow_t const&) noexcept;

_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(_Size) _VCRT_ALLOCATOR
void* __CRTDECL operator new[](std::size_t _Size, ::std::align_val_t align);

void __CRTDECL operator delete(void* blockPtr) noexcept;

void __CRTDECL operator delete(void* _Block, ::std::align_val_t _Al) noexcept;

void __CRTDECL operator delete(void* _Block, ::std::align_val_t      _Al, ::std::nothrow_t const&) noexcept;

void __CRTDECL operator delete[](void* _Block) noexcept;

void __CRTDECL operator delete[](void* _Block, ::std::align_val_t _Al) noexcept;

void __CRTDECL operator delete[](void* _Block, ::std::align_val_t      _Al, ::std::nothrow_t const&) noexcept;

#endif // NOX_WIN64

