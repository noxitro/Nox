//	Copyright (C) 2026 NOX ENGINE All rights reserved.

///	@file	pmr_buffer.h
///	@brief	pmr_buffer
#pragma once
#include    "pmr.h"
#include    "advanced_type.h"

namespace nox
{
    template<class T>
	using PmrVector = std::pmr::vector<T>;

    template<class T>
	class PmrBuffer
	{
        // ⚠️ 宣言順序が重要：mbr → vector の順で初期化される
    public:

        inline explicit PmrBuffer(std::span<nox::uint8> storage)
            : mbr_(storage.data(), storage.size(), &nox::memory::detail::GetPmrMemoryResource())
            , vector(&mbr_)
        {
        }

        // ムーブ・コピーするとvectorがダングリングになるので禁止
        inline constexpr PmrBuffer(const PmrBuffer&)noexcept = delete;
        inline constexpr PmrBuffer(PmrBuffer&&)noexcept = delete;
        inline constexpr PmrBuffer& operator=(const PmrBuffer&)noexcept = delete;
        inline constexpr PmrBuffer& operator=(PmrBuffer&&)noexcept = delete;

		inline constexpr auto& GetVector() noexcept { return vector; }
    private:
        std::pmr::monotonic_buffer_resource mbr_;
        nox::PmrVector<T> vector;
    };
}