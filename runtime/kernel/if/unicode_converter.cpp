//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	unicode_converter.cpp
///	@brief	unicode_converter
#include	"stdafx.h"
#include	"unicode_converter.h"

#include    <cuchar>

#include    "assertion.h"
using namespace nox;

namespace
{


    inline constexpr size_t GetUTF16Length(const char32 str)noexcept
    {
        if (str < 0x10000) // 0x00 - 0xFFFF
        {
            return 1;
        }
        else if (str < 0x110000) // 0x010000 - 0x10FFFF
        {
            return 2;
        }
        else
        {
            return 1;
        }
    }

    inline constexpr size_t GetUTF16Length(std::u32string_view str)noexcept
    {
        size_t result = 0;

        const char32* pSrc = str.data();
        const char32* const pSrcEnd = pSrc + str.size();

        while (pSrc != pSrcEnd)
        {
            result += GetUTF16Length(*pSrc++);
        }

        return result;
    }

    template<class T> requires(sizeof(T) == sizeof(char16))
        inline constexpr void EncodeUTF16(T** s, char32 codePoint) noexcept
    {
        if (codePoint < 0x10000)
        {
            *(*s)++ = static_cast<T>(codePoint);
        }
        else if (codePoint < 0x110000)
        {
            // [Siv3D ToDo] 不正なビット列をはじく
            *(*s)++ = static_cast<T>(((codePoint - 0x10000) >> 10) + 0xD800);
            *(*s)++ = static_cast<T>((codePoint & 0x3FF) + 0xDC00);
        }
        else
        {
            // REPLACEMENT CHARACTER (0xFFFD)
            *(*s)++ = static_cast<T>(0xFFFD);
        }
    }

    inline constexpr size_t GetUTF16Length(const char* str, size_t str_length)noexcept
    {
        size_t length = 0;

        const char* pSrc = str;
        const char* const pSrcEnd = pSrc + str_length;

        std::mbstate_t state{};

        while (pSrc != pSrcEnd)
        {
            char32 out;
            const int32 offset = std::mbrtoc32(&out, pSrc, MB_CUR_MAX, &state);
            length += GetUTF16Length(out);

            pSrc += offset;
        }

        return length;
    }
}

bool	nox::unicode::ConvertWString(std::span<wchar_t> buffer, const c8* from, size_t length)
{
    std::mbstate_t state = std::mbstate_t();
    const char* ptr = util::CharCast<const char*>(from);
    size_t len = 0;
    const ::errno_t err = ::mbsrtowcs_s(&len, nullptr, 0, &ptr, length, &state);
    NOX_ASSERT(err == 0, u"Invalid multibyte sequence");

    if (buffer.size() < len)
    {
        NOX_ASSERT(err == 0, u"Invalid multibyte sequence");
        return false;
    }

    const ::errno_t error = ::mbsrtowcs_s(&len, buffer.data(), len, &ptr, length, &state);
    return true;
}

nox::WString	nox::unicode::ConvertWString(const char* str, size_t length)
{
    nox::WString result(GetUTF16Length(str, length), '0');

    const char* pSrc = str;
    const char* const pSrcEnd = pSrc + length;
    wchar16* pDst = &result[0];
    std::mbstate_t state{};

    while (pSrc != pSrcEnd)
    {
        char16_t out;
        const int32 offset = std::mbrtoc16(&out, pSrc, MB_CUR_MAX, &state);
        *pDst++ = out;
        pSrc += offset;
    }

    return result;


}

nox::WString	nox::unicode::ConvertWString(const char8* const str, size_t length)
{
    return nox::unicode::ConvertWString(reinterpret_cast<const char*>(str), length);
}

nox::WString	nox::unicode::ConvertWString(const char16* const from, size_t length)
{
    return nox::WString(util::CharCast<const w16*>(from), length);
}

nox::WString	nox::unicode::ConvertWString(const char32* const from, size_t length)
{
    nox::WString result(GetUTF16Length(from), L'0');

    const char32* pSrc = from;

    const char32* const pSrcEnd = pSrc + length;
    wchar_t* pDst = &result[0];

    while (pSrc != pSrcEnd)
    {
        EncodeUTF16(&pDst, *pSrc++);
    }

    ConvertString<nox::WString>(U"");

    return result;
}