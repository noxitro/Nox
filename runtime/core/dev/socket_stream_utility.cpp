//	Copyright (c) 2026 NOX ENGINE All rights reserved.

///	@file	socket_stream_utility.cpp
///	@brief	socket_stream_utility
#include	"pch.h"
#include	"socket_stream_utility.h"
#include	"../attribute_common.h"

#if NOX_DEVELOP
#include	"../managed_object.h"
#include	"net/dev_net_log_id.h"

namespace nox::dev::editor_remote
{
	namespace
	{
		
	}
}

bool nox::dev::editor_remote::IsRemoteVariable(const nox::reflection::VariableInfo& variable_info)noexcept
{
	//	シリアライズ対象か
	if (variable_info.GetAttribute<nox::attr::DataMember>() == nullptr &&
		variable_info.GetAttribute<nox::attr::IgnoreDataMember>() == nullptr)
	{
		return false;
	}
	return true;

}

bool nox::dev::editor_remote::IsRemoteFunction(const nox::reflection::FunctionInfo& function_info)noexcept
{
	//	シリアライズ対象か
	if (function_info.GetAttribute<nox::attr::DataMember>() == nullptr &&
		function_info.GetAttribute<nox::attr::IgnoreDataMember>() == nullptr)
	{
		return false;
	}

	return true;
}

std::span<nox::uint8> nox::dev::editor_remote::GetPropertiesBytes(std::span<nox::uint8> buffer, const nox::ManagedObject& obj)
{
	//	型情報の取得
	const nox::reflection::ClassInfo& class_info = nox::util::Deref(nox::reflection::FindClassInfo(obj.GetType()));

	//	書き込み位置カーソル
	nox::uint8* cursor = buffer.data();
	nox::uint8* const buffer_end = buffer.data() + buffer.size();

	const auto write = [&cursor, buffer_end]<class T>(T v)
	{
		NOX_ASSERT(cursor + sizeof(T) <= buffer_end, u8"バッファ不足");
		nox::memory::Copy(cursor, &v, sizeof(T));
		cursor += sizeof(T);
	};

	//	メンバ変数
	for (const nox::reflection::VariableInfo& variable_info : class_info.GetVariableList())
	{
		if (nox::dev::editor_remote::IsRemoteVariable(variable_info) == false)
		{
			continue;
		}

		const nox::reflection::Type& type = variable_info.GetType();
		switch (type.GetTypeKind())
		{
		case nox::reflection::TypeKind::Bool:	
			write(variable_info.GetValue<bool>(obj));
			break;
		case nox::reflection::TypeKind::Int8:	
			write(variable_info.GetValue<nox::int8>(obj));
			break;
		case nox::reflection::TypeKind::Int16:	
			write(variable_info.GetValue<nox::int16>(obj));	
			break;
		case nox::reflection::TypeKind::Int32:	
			write(variable_info.GetValue<nox::int32>(obj));	
			break;
		case nox::reflection::TypeKind::Int64:
			write(variable_info.GetValue<nox::int64>(obj));	
			break;
		case nox::reflection::TypeKind::UInt8:	
			write(variable_info.GetValue<nox::uint8>(obj));
			break;
		case nox::reflection::TypeKind::UInt16:
			write(variable_info.GetValue<nox::uint16>(obj));
			break;
		case nox::reflection::TypeKind::UInt32:	
			write(variable_info.GetValue<nox::uint32>(obj));
			break;
		case nox::reflection::TypeKind::UInt64:
			write(variable_info.GetValue<nox::uint64>(obj));
			break;
		case nox::reflection::TypeKind::Float:	
			write(variable_info.GetValue<nox::float_t>(obj));
			break;
		case nox::reflection::TypeKind::Double:	
			write(variable_info.GetValue<nox::double_t>(obj));
			break;
		case nox::reflection::TypeKind::Class:
			//	クラスはremote_instance_idを書き込む

			break;

		default:
			auto name = nox::reflection::GetEnumFullName(type.GetTypeKind());
			//	未対応
			NOX_WARNING_LINE(nox::dev::net::log_id::DevNet, u8"未対応の型:{0} {1}", name, type.GetTypeName());
			break;
		}
	}

	//	書き込んだ範囲のみ返す
	return std::span(buffer.data(), cursor);
}

void nox::dev::editor_remote::SetPropertiesFromBytes(const std::span<const nox::uint8> bytes, nox::ManagedObject& obj)
{
	//	型情報の取得
	const nox::reflection::ClassInfo& class_info = nox::util::Deref(nox::reflection::FindClassInfo(obj.GetType()));

	//	読み取り位置カーソル
	const nox::uint8* cursor = bytes.data();
	const nox::uint8* const bytes_end = bytes.data() + bytes.size();

	const auto read = [&cursor, bytes_end]<class T>(T& v) -> void
	{
		NOX_ASSERT(cursor + sizeof(T) <= bytes_end, u8"バッファ不足");
		nox::memory::Copy(&v, cursor, sizeof(T));
		cursor += sizeof(T);
	};

	//	メンバ変数
	for (const nox::reflection::VariableInfo& variable_info : class_info.GetVariableList())
	{
		if (nox::dev::editor_remote::IsRemoteVariable(variable_info) == false)
		{
			continue;
		}

		const nox::reflection::Type& type = variable_info.GetType();
		switch (type.GetTypeKind())
		{
		case nox::reflection::TypeKind::Bool:
			{ bool v;				read(v); variable_info.SetValue(obj, v); } break;
		case nox::reflection::TypeKind::Int8:
			{ nox::int8 v;			read(v); variable_info.SetValue(obj, v); } break;
		case nox::reflection::TypeKind::Int16:
			{ nox::int16 v;			read(v); variable_info.SetValue(obj, v); } break;
		case nox::reflection::TypeKind::Int32:
			{ nox::int32 v;			read(v); variable_info.SetValue(obj, v); } break;
		case nox::reflection::TypeKind::Int64:
			{ nox::int64 v;			read(v); variable_info.SetValue(obj, v); } break;
		case nox::reflection::TypeKind::UInt8:
			{ nox::uint8 v;			read(v); variable_info.SetValue(obj, v); } break;
		case nox::reflection::TypeKind::UInt16:
			{ nox::uint16 v;		read(v); variable_info.SetValue(obj, v); } break;
		case nox::reflection::TypeKind::UInt32:
			{ nox::uint32 v;		read(v); variable_info.SetValue(obj, v); } break;
		case nox::reflection::TypeKind::UInt64:
			{ nox::uint64 v;		read(v); variable_info.SetValue(obj, v); } break;
		case nox::reflection::TypeKind::Float:
			{ nox::float_t v;		read(v); variable_info.SetValue(obj, v); } break;
		case nox::reflection::TypeKind::Double:
			{ nox::double_t v;		read(v); variable_info.SetValue(obj, v); } break;

		case nox::reflection::TypeKind::Class:
			//	クラスはremote_instance_idを読み取る
			break;

		default:
			auto name = nox::reflection::GetEnumFullName(type.GetTypeKind());
			//	未対応
			NOX_WARNING_LINE(nox::dev::net::log_id::DevNet, u8"未対応の型:{0} {1}", name, type.GetTypeName());
			break;
		}
	}
}
#endif // NOX_DEVELOP