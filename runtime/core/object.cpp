//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	object.cpp
///	@brief	object
#include	"pch.h"
#include	"object.h"

#include	"garbage_collector.h"
#include	"log_id.h"

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

void nox::Object::AddRef()
{
	NOX_ASSERT(nox::memory::IsHeapPtr(this), u"Object is not allocated on the heap");
	this->ref_count_.fetch_add(1, std::memory_order_relaxed);
}

void nox::Object::Release()
{
	NOX_ASSERT(nox::memory::IsHeapPtr(this), u"Object is not allocated on the heap");
	const auto ref_count = this->ref_count_.fetch_sub(1, std::memory_order_acq_rel) - 1;

	NOX_ASSERT(ref_count >= -2, u"");

	switch (ref_count)
	{
	case -1:
		nox::GarbageCollector::Register(*this);
		break;

	case -2:
		const auto name = GetType().GetTypeName();
		NOX_INFO_LINE(nox::log_id::CoreCommon, u8"delete this:{0}", name);
		delete this;
		break;
	}
}