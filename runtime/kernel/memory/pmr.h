//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	pmr.h
///	@brief	pmr
#pragma once
#include	<memory_resource>
//#include    "../advanced_type.h"

namespace nox::memory::detail
{
    class PmrMemoryResource final : public std::pmr::memory_resource
    {
    private:
        void* do_allocate(std::size_t bytes, std::size_t alignment) override;
        void do_deallocate(void* ptr, std::size_t bytes, std::size_t alignment) override;

        inline constexpr bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override
        {
            return this == &other;
        }
    };

	nox::memory::detail::PmrMemoryResource& GetPmrMemoryResource()noexcept;
}