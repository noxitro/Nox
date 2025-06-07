//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	new_delete.cpp
///	@brief	new_delete
#include	"stdafx.h"
#include	"new_delete.h"

//void* __CRTDECL operator new(std::size_t _Size)
//{
//	return nox::memory::Allocate(_Size, nox::memory::InstanceType::Other);
//}
//
//void* __CRTDECL operator new(size_t _Size, ::std::nothrow_t const&) noexcept
//{
//	return nox::memory::Allocate(_Size, nox::memory::InstanceType::Other);
//}
//
//void* __CRTDECL operator new(std::size_t _Size, ::std::align_val_t align)
//{
//	return nox::memory::Allocate(_Size, static_cast<size_t>(align), nox::memory::InstanceType::Other);
//}
//
//void* __CRTDECL operator new(size_t _Size, ::std::align_val_t      _Al, ::std::nothrow_t const&) noexcept
//{
//	return nox::memory::Allocate(_Size, static_cast<size_t>(_Al), nox::memory::InstanceType::Other);
//}
//
//void* __CRTDECL operator new[](std::size_t _Size)
//{
//	return nox::memory::Allocate(_Size, nox::memory::InstanceType::Other);
//}
//
//void* __CRTDECL operator new[](size_t _Size, ::std::nothrow_t const&) noexcept
//{
//	return nox::memory::Allocate(_Size, nox::memory::InstanceType::Other);
//}
//
//void* __CRTDECL operator new[](std::size_t _Size, ::std::align_val_t align)
//{
//	return nox::memory::Allocate(_Size, static_cast<size_t>(align), nox::memory::InstanceType::Other);
//}
//
//void __CRTDECL operator delete(void* blockPtr) noexcept
//{
//	nox::memory::Deallocate(blockPtr);
//}
//
//void __CRTDECL operator delete(void* _Block, ::std::align_val_t _Al) noexcept
//{
//	nox::memory::Deallocate(_Block, static_cast<size_t>(_Al));
//}
//
//void __CRTDECL operator delete(void* _Block, ::std::align_val_t      _Al, ::std::nothrow_t const&) noexcept
//{
//	nox::memory::Deallocate(_Block, static_cast<size_t>(_Al));
//}
//
//void __CRTDECL operator delete[](void* _Block) noexcept
//{
//	nox::memory::Deallocate(_Block);
//}
//
//void __CRTDECL operator delete[](void* _Block, ::std::align_val_t _Al) noexcept
//{
//	nox::memory::Deallocate(_Block, static_cast<size_t>(_Al));
//}
//
//void __CRTDECL operator delete[](void* _Block, ::std::align_val_t      _Al, ::std::nothrow_t const&) noexcept
//{
//	nox::memory::Deallocate(_Block, static_cast<size_t>(_Al));
//}