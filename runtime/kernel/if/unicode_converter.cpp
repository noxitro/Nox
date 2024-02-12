//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	unicode_converter.cpp
///	@brief	unicode_converter
#include	"stdafx.h"
#include	"unicode_converter.h"

#include    <cuchar>

#include    "assertion.h"

#if NOX_WINDOWS
#include    "../os/windows.h"
#endif // NOX_WINDOWS

using namespace nox;

namespace
{

    inline nox::NString ToNString(const wchar16* str, const size_t length, const uint32 code_page)
    {
        if (length <= 0)
        {
            return nox::NString();
        }

#if NOX_WINDOWS
        const int32 requiredSize = ::WideCharToMultiByte(code_page, 0,
            str, static_cast<int32>(length),
            nullptr, 0, nullptr, nullptr);

        nox::NString result(requiredSize, '\0');

        ::WideCharToMultiByte(code_page, 0,
           str, static_cast<int32>(length),
            &result[0], requiredSize, nullptr, nullptr);

        return result;
#else

        return nox::NString(); 
#endif
    }

//    inline NString ToNString(const std::u32)

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

    inline constexpr size_t GetUTF16Length(const char32* str, const size_t length)noexcept
    {
        size_t result = 0;

        const char32* pSrc = str;
        const char32* const pSrcEnd = pSrc + length;

        while (pSrc != pSrcEnd)
        {
            result += GetUTF16Length(*pSrc++);
        }

        return result;
    }

    inline constexpr size_t GetUTF16Length(std::u32string_view str)noexcept { return GetUTF16Length(str.data(), str.length()); }

    inline constexpr void EncodeUTF16(char16*& s, const char32 code_point) noexcept
    {
        if (code_point < 0x10000)
        {
            *(s)++ = static_cast<char16>(code_point);
        }
        else if (code_point < 0x110000)
        {
            //TODO: 不正なビット列をはじく
            *(s)++ = static_cast<char16>(((code_point - 0x10000) >> 10) + 0xD800);
            *(s)++ = static_cast<char16>((code_point & 0x3FF) + 0xDC00);
        }
        else
        {
            // REPLACEMENT CHARACTER (0xFFFD)
            *(s)++ = static_cast<char16>(0xFFFD);
        }
    }

    inline constexpr void EncodeUTF16(wchar16*& s, char32 code_point) noexcept { EncodeUTF16(reinterpret_cast<char16*&>(s), code_point); }

    inline constexpr size_t GetUTF8Length(const char32 code_point)noexcept
    {
        if (code_point < 0x80) // 0x00 - 0x7F
        {
            return 1;
        }
        else if (code_point < 0x800) // 0x80 - 0x07FF
        {
            //TODO: 不正なコードポイントをはじく
            return 2;
        }
        else if (code_point < 0x10000) // 0x0800 - 0xFFFF
        {
            //TODO: 不正なコードポイントをはじく
            return 3;
        }
        else if (code_point < 0x110000) // 0x010000 - 0x10FFFF
        {
            //TODO: 不正なコードポイントをはじく
            return 4;
        }
        else // Invalid code point
        {
            return 3; // REPLACEMENT CHARACTER (0xEF 0xBF 0xBD)
        }
    }

    inline constexpr void EncodeUTF8(char8*& s, const char32 code_point)noexcept
    {
        if (code_point < 0x80) // 0x00 - 0x7F
        {
            *(s)++ = static_cast<char8>(code_point);
        }
        else if (code_point < 0x800) // 0x80 - 0x07FF
        {
            //TODO: 不正なコードポイントをはじく
            *(s)++ = (0xC0 | static_cast<char8>(code_point >> 6));
            *(s)++ = (0x80 | static_cast<char8>(code_point & 0x3F));
        }
        else if (code_point < 0x10000) // 0x0800 - 0xFFFF
        {
            //TODO: 不正なコードポイントをはじく
            *(s)++ = (0xE0 | static_cast<char8>(code_point >> 12));
            *(s)++ = (0x80 | static_cast<char8>((code_point >> 6) & 0x3F));
            *(s)++ = (0x80 | static_cast<char8>(code_point & 0x3F));
        }
        else if (code_point < 0x110000) // 0x010000 - 0x10FFFF
        {
            //TODO: 不正なコードポイントをはじく
            *(s)++ = (0xF0 | static_cast<char8>(code_point >> 18));
            *(s)++ = (0x80 | static_cast<char8>((code_point >> 12) & 0x3F));
            *(s)++ = (0x80 | static_cast<char8>((code_point >> 6) & 0x3F));
            *(s)++ = (0x80 | static_cast<char8>(code_point & 0x3F));
        }
        else // Invalid code point
        {
            // REPLACEMENT CHARACTER (0xEF 0xBF 0xBD)
            *(s)++ = static_cast<char8>(uint8(0xEF));
            *(s)++ = static_cast<char8>(uint8(0xBF));
            *(s)++ = static_cast<char8>(uint8(0xBD));
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

nox::NString	nox::unicode::ConvertNString(const char32* str, size_t length)
{
    const nox::WString wstr = ConvertWString(str, length);

    return nox::NString();
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


    const char32* const src_end = from + length;

    char16* begin = reinterpret_cast<char16*>(*result.begin());

    for (const char32* src = from; src != src_end; ++src)
    {
        EncodeUTF16(begin, *src);
    }

    return result;
}