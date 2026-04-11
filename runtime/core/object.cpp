//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	object.cpp
///	@brief	object
#include	"pch.h"
#include	"object.h"

nox::U8String	nox::Object::ToString()const
{
	//	1024文字のバッファを確保
	std::array<nox::char8, 1024> buffer = {U'\0'};
	return this->ToString(buffer);
}

nox::U8StringView	nox::Object::ToString(std::span<nox::char8> dest_buffer)const
{
	const nox::reflection::Type& type = this->GetType();
	auto class_info = nox::reflection::FindClassInfo(type);

	//	クラス情報が見つからない場合はコンパイル時の型名を返す
	if (class_info == nullptr)
	{
		return {};
//		return nox::unicode::ConvertU16String(type.GetTypeName(), dest_buffer);
	}

	return {};
}