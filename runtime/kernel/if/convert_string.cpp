///	@file	convert_string.cpp
///	@brief	convert_string
#include	"stdafx.h"
#include	"convert_string.h"

#include	<codecvt>

#include	"../memory/stl_allocate_adapter.h"
#include    "assertion.h"
using namespace nox;

bool	nox::util::ConvertWString(std::span<wchar_t> buffer, const c8* from)
{
    std::mbstate_t state = std::mbstate_t();
    const char* ptr = CharCast<const char*>(from);
    size_t len = 0;
    const ::errno_t err = ::mbsrtowcs_s(&len, nullptr, 0, &ptr, 0, &state);
    NOX_ASSERT(err == 0, u"Invalid multibyte sequence");

    if (buffer.size() < len)
    {
        NOX_ASSERT(err == 0, u"Invalid multibyte sequence");
        return false;
    }

    const ::errno_t error = ::mbsrtowcs_s(&len, buffer.data(), len, &ptr, len, &state);
    return true;
}

nox::WString	nox::util::ConvertWString(const c8* const from)
{
    std::mbstate_t state = std::mbstate_t();
    const char* ptr = CharCast<const char*>(from);
    size_t len = 0;
    const ::errno_t err = ::mbsrtowcs_s(&len, nullptr, 0, &ptr, 0, &state);
    NOX_ASSERT(err == 0, u"Invalid multibyte sequence");

    nox::WString wstr(len, L'\0');
    const ::errno_t error = ::mbsrtowcs_s(&len, wstr.data(), len, &ptr, len, &state);
    return wstr;
}

nox::WString	nox::util::ConvertWString(const c16* const from)
{
	return nox::WString(CharCast<const w16*>(from));

}