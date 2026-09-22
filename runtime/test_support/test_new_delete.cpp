//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	test_new_delete.cpp
///	@brief	test_new_delete
///
///	@details
///		kernel は memory/new_delete.h で グローバル operator new / delete を
///		nox::memory::Allocate / Deallocate に差し替えている。
///		一方 gtest は vcpkg の x64-windows トリプレットにより DLL (gtest.dll) として
///		リンクされ、DLL 側は CRT 標準の operator new / delete を使う。
///
///		この状態で exe 側だけ独自アロケータになると、
///		gtest.dll が確保したメモリを exe が解放する (またはその逆) 経路で
///		ヘッダ付きブロックと生ブロックが混ざり、ヒープが壊れてハングする。
///		(実際 gtest の TEST 登録経路でこれが起きる)
///
///		そこでテスト実行ファイルでは、このTUで標準の operator new / delete を
///		定義しておく。リンカはライブラリ (kernel.lib の new_delete.obj) より
///		先にオブジェクトファイル側の定義を採用するため、
///		exe と gtest.dll が同じ CRT ヒープを共有する状態になる。
///
///	@note
///		そのため kernel_test は nox::memory のグローバルアロケータ自体は検証しない。
///		nox::memory::Allocate / Deallocate を直接呼ぶ経路 (pmr, stl_allocate_adapter)
///		は差し替えの影響を受けないので、そちらは通常どおり動作する。

#include	<cstdlib>
#include	<new>

_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size) _VCRT_ALLOCATOR
void* __CRTDECL operator new(std::size_t size)
{
	void* const ptr = std::malloc(size != 0 ? size : 1);
	if (ptr == nullptr)
	{
		throw std::bad_alloc();
	}
	return ptr;
}

_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size) _VCRT_ALLOCATOR
void* __CRTDECL operator new[](std::size_t size)
{
	return ::operator new(size);
}

_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size) _VCRT_ALLOCATOR
void* __CRTDECL operator new(std::size_t size, std::align_val_t align)
{
	void* const ptr = ::_aligned_malloc(size != 0 ? size : 1, static_cast<std::size_t>(align));
	if (ptr == nullptr)
	{
		throw std::bad_alloc();
	}
	return ptr;
}

_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size) _VCRT_ALLOCATOR
void* __CRTDECL operator new[](std::size_t size, std::align_val_t align)
{
	return ::operator new(size, align);
}

_NODISCARD _Ret_maybenull_ _Success_(return != NULL) _Post_writable_byte_size_(size) _VCRT_ALLOCATOR
void* __CRTDECL operator new(std::size_t size, const std::nothrow_t&) noexcept
{
	return std::malloc(size != 0 ? size : 1);
}

_NODISCARD _Ret_maybenull_ _Success_(return != NULL) _Post_writable_byte_size_(size) _VCRT_ALLOCATOR
void* __CRTDECL operator new[](std::size_t size, const std::nothrow_t&) noexcept
{
	return std::malloc(size != 0 ? size : 1);
}

_NODISCARD _Ret_maybenull_ _Success_(return != NULL) _Post_writable_byte_size_(size) _VCRT_ALLOCATOR
void* __CRTDECL operator new(std::size_t size, std::align_val_t align, const std::nothrow_t&) noexcept
{
	return ::_aligned_malloc(size != 0 ? size : 1, static_cast<std::size_t>(align));
}

_NODISCARD _Ret_maybenull_ _Success_(return != NULL) _Post_writable_byte_size_(size) _VCRT_ALLOCATOR
void* __CRTDECL operator new[](std::size_t size, std::align_val_t align, const std::nothrow_t&) noexcept
{
	return ::_aligned_malloc(size != 0 ? size : 1, static_cast<std::size_t>(align));
}

void __CRTDECL operator delete(void* ptr) noexcept
{
	std::free(ptr);
}

void __CRTDECL operator delete[](void* ptr) noexcept
{
	std::free(ptr);
}

void __CRTDECL operator delete(void* ptr, std::size_t) noexcept
{
	std::free(ptr);
}

void __CRTDECL operator delete[](void* ptr, std::size_t) noexcept
{
	std::free(ptr);
}

void __CRTDECL operator delete(void* ptr, const std::nothrow_t&) noexcept
{
	std::free(ptr);
}

void __CRTDECL operator delete[](void* ptr, const std::nothrow_t&) noexcept
{
	std::free(ptr);
}

void __CRTDECL operator delete(void* ptr, std::align_val_t) noexcept
{
	::_aligned_free(ptr);
}

void __CRTDECL operator delete[](void* ptr, std::align_val_t) noexcept
{
	::_aligned_free(ptr);
}

void __CRTDECL operator delete(void* ptr, std::size_t, std::align_val_t) noexcept
{
	::_aligned_free(ptr);
}

void __CRTDECL operator delete[](void* ptr, std::size_t, std::align_val_t) noexcept
{
	::_aligned_free(ptr);
}

void __CRTDECL operator delete(void* ptr, std::align_val_t, const std::nothrow_t&) noexcept
{
	::_aligned_free(ptr);
}

void __CRTDECL operator delete[](void* ptr, std::align_val_t, const std::nothrow_t&) noexcept
{
	::_aligned_free(ptr);
}
