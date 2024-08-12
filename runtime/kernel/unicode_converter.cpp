//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	unicode_converter.cpp
///	@brief	unicode_converter
#include	"stdafx.h"
#include	"unicode_converter.h"

#include    <cuchar>

#include    "assertion.h"

#if NOX_WINDOWS
#include    "os/windows.h"
#endif // NOX_WINDOWS

using namespace nox;
static_assert(sizeof(nox::wchar16) == sizeof(nox::char16), "wchar16 and char16 are not the same size");

namespace
{
    struct OffsetPoint
    {
        int32 offset;
        char32 codePoint;
    };

    inline constexpr OffsetPoint kInvalidPoint = { .offset = -1, .codePoint = 0 };

    inline constexpr bool IsHighSurrogate(const char16 c)noexcept { return (c >= 0xD800) && (c < 0xDC00); }

    inline constexpr bool IsLowSurrogate(const char16 c)noexcept { return (c >= 0xDC00) && (c < 0xE000); }

    inline constexpr OffsetPoint DecodeCheckUTF8(const std::u8string_view s)noexcept
    {
        const size_t length = s.length();

        uint32_t b0, b1, b2, b3;

        b0 = static_cast<uint8>(s[0]);

        if (b0 < 0x80)
        {
            // 1-byte character
            return{ 1, b0 };
        }
        else if (b0 < 0xC0)
        {
            // Unexpected continuation byte
            return kInvalidPoint;
        }
        else if (b0 < 0xE0)
        {
            if (length < 2)
            {
                return kInvalidPoint;
            }

            // 2-byte character
            if (((b1 = s[1]) & 0xC0) != 0x80)
            {
                return kInvalidPoint;
            }

            const char32 pt = (b0 & 0x1F) << 6 | (b1 & 0x3F);

            if (pt < 0x80)
            {
                return kInvalidPoint;
            }

            return{ 2, pt };
        }
        else if (b0 < 0xF0)
        {
            if (length < 3)
            {
                return kInvalidPoint;
            }

            // 3-byte character
            if (((b1 = s[1]) & 0xC0) != 0x80)
            {
                return kInvalidPoint;
            }

            if (((b2 = s[2]) & 0xC0) != 0x80)
            {
                return kInvalidPoint;
            }

            const char32 pt = (b0 & 0x0F) << 12 | (b1 & 0x3F) << 6 | (b2 & 0x3F);

            if (pt < 0x800)
            {
                return kInvalidPoint;
            }

            return{ 3, pt };
        }
        else if (b0 < 0xF8)
        {
            if (length < 4)
            {
                return kInvalidPoint;
            }

            // 4-byte character
            if (((b1 = s[1]) & 0xC0) != 0x80)
            {
                return kInvalidPoint;
            }

            if (((b2 = s[2]) & 0xC0) != 0x80)
            {
                return kInvalidPoint;
            }

            if (((b3 = s[3]) & 0xC0) != 0x80)
            {
                return kInvalidPoint;
            }

            const char32 pt = (b0 & 0x0F) << 18 | (b1 & 0x3F) << 12 | (b2 & 0x3F) << 6 | (b3 & 0x3F);

            if (pt < 0x10000 || pt >= 0x110000)
            {
                return kInvalidPoint;
            }

            return{ 4, pt };
        }
        else
        {
            // Codepoint out of range
            return kInvalidPoint;
        }
    }


    inline constexpr std::optional<OffsetPoint> DecodeCheckUTF16(const std::u16string_view s)noexcept
    {
        if (s.size() <= 0)
        {
            return std::nullopt;
        }

        if (IsHighSurrogate(s[0]) && s.size() >= 2 && IsLowSurrogate(s[1]))
        {
            // High surrogate followed by low surrogate
            const char32 pt = (((s[0] - 0xD800) << 10) | (s[1] - 0xDC00)) + 0x10000;

            return OffsetPoint{ 2, pt };
        }
        else if (IsHighSurrogate(s[0]) || IsLowSurrogate(s[0]))
        {
            // High surrogate *not* followed by low surrogate, or unpaired low surrogate
            return std::nullopt;
        }
        else
        {
            return OffsetPoint{ 1, s[0] };
        }
    }

    inline constexpr OffsetPoint DecodeUTF8(const std::u8string_view s)noexcept
    {
        const OffsetPoint res = DecodeCheckUTF8(s);

        if (res.offset < 0)
        {
            return OffsetPoint{ .offset = 1, .codePoint = 0xFFFD,  };
        }
        else
        {
            return res;
        }
    }

    inline constexpr OffsetPoint DecodeUTF16(const std::u16string_view s)noexcept
    {
        const std::optional<OffsetPoint> res = DecodeCheckUTF16(s);

        if (res.has_value() == false)
        {
            return OffsetPoint{ .offset = 1, .codePoint = 0xFFFD };
        }
        else
        {
            return res.value();
        }
    }


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

    template<class CharType = char16> requires(std::is_same_v<CharType, char16> || std::is_same_v<CharType, wchar16>)
    inline CharType EncodeUTF16(const char32 code_point)
    {
        if (code_point < 0x10000)
        {
            return static_cast<CharType>(code_point);
        }
        else if (code_point < 0x110000)
        {
            //TODO: 不正なビット列をはじく
            return static_cast<CharType>(((code_point - 0x10000) >> 10) + 0xD800);
            return static_cast<CharType>((code_point & 0x3FF) + 0xDC00);
        }
        else
        {
            // REPLACEMENT CHARACTER (0xFFFD)
            return static_cast<CharType>(0xFFFD);
        }
    }

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

    inline constexpr size_t GetUTF16Length(std::u8string_view str_view)noexcept
    {
        size_t length = 0;

        const char8* pSrc = str_view.data();
        const char8* const pSrcEnd = pSrc + str_view.size();

        std::mbstate_t state{};

        while (pSrc != pSrcEnd)
        {
            char32 out;
            const int32 offset = std::mbrtoc32(&out, reinterpret_cast<const char*>(pSrc), MB_CUR_MAX, &state);
            length += GetUTF16Length(out);

            pSrc += offset;
        }

        return length;
    }

    inline constexpr size_t GetUTF32Length(const std::u8string_view str_view)noexcept
    {
        size_t length = 0;

        const char8* pSrc = str_view.data();
        const char8* const pSrcEnd = pSrc + str_view.size();

        while (pSrc != pSrcEnd)
        {

            const OffsetPoint offset_point = DecodeUTF8({ pSrc, pSrcEnd });

            pSrc += offset_point.offset;

            ++length;
        }

        return length;
    }

    inline constexpr size_t GetUTF32Length(const std::u16string_view str_view)noexcept
    {
        const char16* pSrc = str_view.data();
        const char16* const pSrcEnd = pSrc + str_view.size();

        size_t length = 0;

        while (pSrc != pSrcEnd)
        {
            pSrc += DecodeUTF16(std::u16string_view{ pSrc, pSrcEnd }).offset;

            ++length;
        }

        return length;
    }
}


#pragma region char

nox::NString	nox::unicode::ConvertNString(std::u8string_view str_view)
{
    NOX_ASSERT(false, U"");
    return nox::NString();
}

nox::NString	nox::unicode::ConvertNString(std::u16string_view str_view)
{
    NOX_ASSERT(false, U"");
    return nox::NString();
}

nox::NString	nox::unicode::ConvertNString(std::wstring_view str_view)
{
    NOX_ASSERT(false, U"");
    return nox::NString();
}

nox::NString	nox::unicode::ConvertNString(std::u32string_view str_view)
{
    NOX_ASSERT(false, U"");
    return nox::NString();
}
#pragma endregion

#pragma region wchar16
nox::WString	nox::unicode::ConvertWString(std::u8string_view str_view)
{
    const size_t length = GetUTF16Length(str_view);
    nox::WString result(length, '0');
    return result;
}

nox::WString	nox::unicode::ConvertWString(std::u32string_view str_view)
{
    const size_t length = GetUTF16Length(str_view);
    nox::WString result(length, '0');
    return result;
}
#pragma endregion

#pragma region char8

nox::U8String	nox::unicode::ConvertU8String(std::string_view str_view)
{
    NOX_ASSERT(false, U"");
    return {};
}

nox::U8String	nox::unicode::ConvertU8String(std::u16string_view str_view)
{
    NOX_ASSERT(false, U"");
    return {};
}

nox::U8String	nox::unicode::ConvertU8String(std::u32string_view str_view)
{
    NOX_ASSERT(false, U"");
    return {};
}
#pragma endregion

#pragma region char16
namespace
{
    inline bool ConvertU16StringImpl(const std::u8string_view str_view, const size_t str_view_utf16_length, std::span<char16> dest_buffer)
    {
        NOX_ASSERT(dest_buffer.size() >= str_view_utf16_length, U"buffer size over");

        decltype(auto) it = str_view.begin();
        
        for (int32 i = 0; it != str_view.end(); ++i)
        {
            const OffsetPoint offset_point = DecodeUTF8(&*it);
            dest_buffer[i] = EncodeUTF16(offset_point.codePoint);
            std::advance(it, offset_point.offset);
        }

        return true;
    }

    template<concepts::Char DestCharType, concepts::Char SourceCharType>
    inline bool ConvertStringImpl(DestCharType(*encode_func)(SourceCharType), const std::basic_string_view<SourceCharType> str_view, const size_t str_view_length, std::span<DestCharType> dest_buffer)
    {
        NOX_ASSERT(dest_buffer.size() >= str_view_length, U"buffer size over");

        DestCharType* dest_ptr = &*dest_buffer.begin();

        for (SourceCharType s : str_view)
        {
            *dest_ptr++ = encode_func(s);
        }

        return true;
    }
}

std::span<char16>	nox::unicode::ConvertU16String(const std::u8string_view str_view, std::span<char16> dest_buffer)
{
    ConvertU16StringImpl(str_view, GetUTF16Length(str_view), dest_buffer);
    return dest_buffer;
}

std::span<char16>	nox::unicode::ConvertU16String(const std::u32string_view str_view, std::span<char16> dest_buffer)
{
    ConvertStringImpl(EncodeUTF16<char16>, str_view, GetUTF16Length(str_view), dest_buffer);
    return dest_buffer;
}

nox::U16String	nox::unicode::ConvertU16String(std::u8string_view str_view)
{
    const size_t length = GetUTF16Length(str_view);
    nox::U16String result(length, '0');

    ConvertU16StringImpl(str_view, length, result);

    return result;
}

nox::U16String	nox::unicode::ConvertU16String(std::u32string_view str_view)
{
    const size_t length = GetUTF16Length(str_view);
    nox::U16String result(length, '0');

    NOX_ASSERT(ConvertStringImpl(EncodeUTF16<char16>, str_view, length, std::span<char16>(result)) == true, U"");

    return result;
}
#pragma endregion

#pragma region char32
namespace
{

    inline bool ConvertU32String(const std::u8string_view str_view, const size_t str_view_utf32_length, std::span<char32> dest_buffer)
    {
        NOX_ASSERT(dest_buffer.size() >= str_view_utf32_length, U"buffer size over");

        char32* pDst = &*dest_buffer.begin();

        auto it = str_view.begin();
        while(it != str_view.end())
        {
            const OffsetPoint offset_point = DecodeUTF8(&*it);
            *pDst++ = offset_point.codePoint;

            std::advance(it, offset_point.offset);
        }

        return true;
    }

    inline bool	ConvertU32String(const std::u16string_view str_view, const size_t str_view_utf32_length, std::span<char32> dest_buffer)
    {
        NOX_ASSERT(dest_buffer.size() >= str_view_utf32_length, U"buffer size over");

        char32* pDst = &*dest_buffer.begin();

        auto it = str_view.begin();
        while (it != str_view.end())
        {
            const OffsetPoint offsetPoint = DecodeUTF16(&*it);
            *pDst++ = offsetPoint.codePoint;

            std::advance(it, offsetPoint.offset);
        }

        return true;
    }
}

std::span<char32>	nox::unicode::ConvertU32String(const std::u8string_view str_view, std::span<char32> dest_buffer)
{
    ::ConvertU32String(str_view, GetUTF32Length(str_view), dest_buffer);
    return dest_buffer;
}

std::span<char32>	nox::unicode::ConvertU32String(const std::u16string_view str_view, std::span<char32> dest_buffer)
{
    ::ConvertU32String(str_view, GetUTF32Length(str_view), dest_buffer);
    return dest_buffer;
}

nox::U32String	nox::unicode::ConvertU32String(std::u8string_view str_view)
{
    const size_t length = GetUTF32Length(str_view);
    nox::U32String result(length, '0');

    ::ConvertU32String(str_view, length, result);

    return result;
}

nox::U32String	nox::unicode::ConvertU32String(std::u16string_view str_view)
{
    const size_t length = GetUTF32Length(str_view);
    nox::U32String result(length, '0');

    ::ConvertU32String(str_view, length, result);

    return result;
}
#pragma endregion
