//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	object.cpp
///	@brief	object
#include	"stdafx.h"
#include	"object.h"

nox::String	nox::Object::ToString()const
{
	//	1024文字のバッファを確保
	std::array<nox::char32, 1024> buffer = {U'\0'};
	return ToString(buffer);
}

nox::StringView	nox::Object::ToString(std::span<nox::char32> dest_buffer)const
{
	const nox::reflection::Type& type = GetType();
	auto class_info = nox::reflection::FindClassInfo(type);

	//	クラス情報が見つからない場合はコンパイル時の型名を返す
	if (class_info == nullptr)
	{
		return nox::unicode::ConvertU32String(type.GetTypeName(), dest_buffer);
	}

	nox::util::StrCopy(class_info->GetFullName(), dest_buffer);
	return nox::StringView(dest_buffer);
}
//
//bool	nox::Object::IsOverride(const nox::uint64 function_id, std::span<void(*)()> vtable)const noexcept
//{
//	return false;
//}