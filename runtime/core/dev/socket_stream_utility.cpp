//	Copyright (c) 2026 NOX ENGINE All rights reserved.

///	@file	socket_stream_utility.cpp
///	@brief	socket_stream_utility
#include	"pch.h"
#include	"socket_stream_utility.h"
#include	"../attribute_common.h"
#include	"../attribute_dev_common.h"
#include	"../../kernel/fixed_vector.h"
#include	<cstdio>
#include	<malloc.h>

#if NOX_DEVELOP
#include	"../managed_object.h"
#include	"net/dev_net_log_id.h"

namespace nox::dev::editor_remote
{
	namespace
	{
		constexpr nox::uint32 k_remote_property_capacity = 64;

		struct RemotePropertyInfo
		{
			std::u8string_view name{};
			const nox::reflection::VariableInfo* variable_info{};
			const nox::reflection::FunctionInfo* getter_function{};
			const nox::reflection::FunctionInfo* setter_function{};
			const nox::reflection::Type* value_type{};
		};

		class AlignedTypeStorage
		{
		public:
			explicit AlignedTypeStorage(const nox::reflection::Type& type) noexcept :
				size_(type.GetTypeSize()),
				alignment_(type.GetAlignmentOf() > alignof(void*) ? type.GetAlignmentOf() : alignof(void*)),
				storage_(size_ == 0 ? nullptr : static_cast<nox::uint8*>(::_aligned_malloc(size_, alignment_)))
			{
			}

			AlignedTypeStorage(const AlignedTypeStorage&) = delete;
			AlignedTypeStorage& operator=(const AlignedTypeStorage&) = delete;

			~AlignedTypeStorage()
			{
				if (storage_ != nullptr)
				{
					::_aligned_free(storage_);
				}
			}

			[[nodiscard]] bool IsValid() const noexcept { return storage_ != nullptr; }
			[[nodiscard]] void* Get() const noexcept { return storage_; }
			[[nodiscard]] std::size_t GetSize() const noexcept { return size_; }

		private:
			std::size_t size_{};
			std::size_t alignment_{};
			nox::uint8* storage_{};
		};

		template<class T>
		void WriteValue(nox::uint8*& cursor, nox::uint8* const buffer_end, T v)
		{
			NOX_ASSERT(cursor + sizeof(T) <= buffer_end, u8"バッファ不足");
			nox::memory::Copy(static_cast<void*>(cursor), static_cast<const void*>(std::addressof(v)), sizeof(T));
			cursor += sizeof(T);
		}

		template<class T>
		void ReadValue(const nox::uint8*& cursor, const nox::uint8* const bytes_end, T& v)
		{
			NOX_ASSERT(cursor + sizeof(T) <= bytes_end, u8"バッファ不足");
			nox::memory::Copy(static_cast<void*>(std::addressof(v)), static_cast<const void*>(cursor), sizeof(T));
			cursor += sizeof(T);
		}

		template<class T>
		void WriteBytes(nox::uint8*& cursor, nox::uint8* const buffer_end, const T& value)
		{
			NOX_ASSERT(cursor + sizeof(T) <= buffer_end, u8"バッファ不足");
			nox::memory::Copy(static_cast<void*>(cursor), static_cast<const void*>(std::addressof(value)), sizeof(T));
			cursor += sizeof(T);
		}

		template<class T>
		void ReadBytes(const nox::uint8*& cursor, const nox::uint8* const bytes_end, T& value)
		{
			NOX_ASSERT(cursor + sizeof(T) <= bytes_end, u8"バッファ不足");
			nox::memory::Copy(static_cast<void*>(std::addressof(value)), static_cast<const void*>(cursor), sizeof(T));
			cursor += sizeof(T);
		}

		const nox::reflection::Type& NormalizePropertyType(const nox::reflection::Type& type) noexcept
		{
			const nox::reflection::Type* current = &type;
			while (current->GetTypeKind() == nox::reflection::TypeKind::Pointer ||
				current->GetTypeKind() == nox::reflection::TypeKind::LValueReference ||
				current->GetTypeKind() == nox::reflection::TypeKind::RValueReference)
			{
				current = &current->GetPointeeType();
			}

			if (const nox::reflection::Type& remove_const_type = current->GetRemoveConstType(); remove_const_type.IsValid())
			{
				current = &remove_const_type;
			}
			if (const nox::reflection::Type& remove_volatile_type = current->GetRemoveVolatileType(); remove_volatile_type.IsValid())
			{
				current = &remove_volatile_type;
			}

			return *current;
		}

		const nox::reflection::Type* TryGetBitCopyStorageType(const nox::reflection::Type& type) noexcept
		{
			const nox::reflection::Type* current = &type;
			while (current->GetTypeKind() == nox::reflection::TypeKind::LValueReference ||
				current->GetTypeKind() == nox::reflection::TypeKind::RValueReference)
			{
				current = &current->GetPointeeType();
			}

			if (current->GetTypeKind() == nox::reflection::TypeKind::Pointer)
			{
				return nullptr;
			}

			if (const nox::reflection::Type& remove_const_type = current->GetRemoveConstType(); remove_const_type.IsValid())
			{
				current = &remove_const_type;
			}
			if (const nox::reflection::Type& remove_volatile_type = current->GetRemoveVolatileType(); remove_volatile_type.IsValid())
			{
				current = &remove_volatile_type;
			}

			return current->IsValid() ? current : nullptr;
		}

		bool CanTransportByBitCopy(const nox::reflection::Type& type) noexcept
		{
			const nox::reflection::Type* const storage_type = TryGetBitCopyStorageType(type);
			if (storage_type == nullptr)
			{
				return false;
			}

			switch (storage_type->GetTypeKind())
			{
			case nox::reflection::TypeKind::Bool:
			case nox::reflection::TypeKind::Int8:
			case nox::reflection::TypeKind::Int16:
			case nox::reflection::TypeKind::Int32:
			case nox::reflection::TypeKind::Int64:
			case nox::reflection::TypeKind::UInt8:
			case nox::reflection::TypeKind::UInt16:
			case nox::reflection::TypeKind::UInt32:
			case nox::reflection::TypeKind::UInt64:
			case nox::reflection::TypeKind::Float:
			case nox::reflection::TypeKind::Double:
			case nox::reflection::TypeKind::Enum:
			case nox::reflection::TypeKind::ScopedEnum:
				return true;

			case nox::reflection::TypeKind::Class:
			case nox::reflection::TypeKind::Union:
				return storage_type->IsTypeAttributeFlag(nox::reflection::TypeAttributeFlag::TrivialCopyable);

			default:
				return false;
			}
		}

		std::u8string_view TrimPropertyFunctionPrefix(std::u8string_view name) noexcept
		{
			if (name.starts_with(u8"Get") && name.size() > 3)
			{
				return name.substr(3);
			}

			if (name.starts_with(u8"Set") && name.size() > 3)
			{
				return name.substr(3);
			}

			if (name.starts_with(u8"Is") && name.size() > 2)
			{
				return name.substr(2);
			}

			return name;
		}

		template<std::size_t Size>
		std::size_t NormalizePropertyNameToken(std::u8string_view name, std::array<char8_t, Size>& buffer) noexcept
		{
			if (name.starts_with(u8"Local"))
			{
				name.remove_prefix(5);
			}
			else if (name.starts_with(u8"World"))
			{
				name.remove_prefix(5);
			}

			std::size_t length = 0;
			for (char8_t ch : name)
			{
				if (ch == u8'_')
				{
					continue;
				}

				if (length >= buffer.size())
				{
					break;
				}

				if (ch >= u8'A' && ch <= u8'Z')
				{
					buffer[length++] = static_cast<char8_t>(ch - u8'A' + u8'a');
				}
				else
				{
					buffer[length++] = ch;
				}
			}

			return length;
		}

		bool IsPropertyNameMatch(std::u8string_view property_name, std::u8string_view variable_name) noexcept
		{
			std::array<char8_t, 96> normalized_property{};
			std::array<char8_t, 96> normalized_variable{};
			const std::size_t property_length = NormalizePropertyNameToken(property_name, normalized_property);
			const std::size_t variable_length = NormalizePropertyNameToken(variable_name, normalized_variable);
			if (property_length != variable_length)
			{
				return false;
			}

			for (std::size_t i = 0; i < property_length; ++i)
			{
				if (normalized_property[i] != normalized_variable[i])
				{
					return false;
				}
			}

			return property_length > 0;
		}

		bool IsEditorVisibleVariable(const nox::reflection::VariableInfo& variable_info) noexcept
		{
			return IsRemoteVariable(variable_info) && variable_info.GetAttribute<nox::attr::dev::Hide>() == nullptr;
		}

		bool TryGetPropertyFunctionInfo(
			const nox::reflection::FunctionInfo& function_info,
			std::u8string_view& property_name,
			const nox::reflection::Type*& value_type,
			bool& is_getter) noexcept
		{
			property_name = {};
			value_type = nullptr;
			is_getter = false;

			if (function_info.IsStatic())
			{
				return false;
			}

			std::u8string_view explicit_name;
			if (const auto* getter_attr = static_cast<const nox::attr::dev::PropertyGetter*>(function_info.GetAttribute<nox::attr::dev::PropertyGetter>()))
			{
				explicit_name = getter_attr->GetPropertyName();
				if (function_info.GetFunctionParamLength() != 0 || function_info.IsNoReturn())
				{
					return false;
				}
				is_getter = true;
				value_type = &NormalizePropertyType(function_info.GetResultType());
			}
			else if (const auto* setter_attr = static_cast<const nox::attr::dev::PropertySetter*>(function_info.GetAttribute<nox::attr::dev::PropertySetter>()))
			{
				explicit_name = setter_attr->GetPropertyName();
				if (function_info.GetFunctionParamLength() != 1)
				{
					return false;
				}
				value_type = &NormalizePropertyType(function_info.GetFunctionParam(0).GetType());
			}
			else if (const auto* property_attr = static_cast<const nox::attr::dev::Property*>(function_info.GetAttribute<nox::attr::dev::Property>()))
			{
				explicit_name = property_attr->GetPropertyName();
				if (function_info.GetFunctionParamLength() == 0 && function_info.IsNoReturn() == false)
				{
					is_getter = true;
					value_type = &NormalizePropertyType(function_info.GetResultType());
				}
				else if (function_info.GetFunctionParamLength() == 1)
				{
					value_type = &NormalizePropertyType(function_info.GetFunctionParam(0).GetType());
				}
				else
				{
					return false;
				}
			}
			else
			{
				return false;
			}

			property_name = explicit_name.empty() ? TrimPropertyFunctionPrefix(function_info.GetName()) : explicit_name;
			return property_name.empty() == false && value_type != nullptr;
		}

		nox::int32 FindRemotePropertyIndex(std::span<const RemotePropertyInfo> property_list, std::u8string_view name) noexcept
		{
			for (nox::int32 i = 0; i < static_cast<nox::int32>(property_list.size()); ++i)
			{
				if (property_list[static_cast<std::size_t>(i)].name == name)
				{
					return i;
				}
			}

			return -1;
		}

		const nox::reflection::VariableInfo* FindPropertyBackingVariable(
			const nox::reflection::ClassInfo& class_info,
			std::u8string_view property_name,
			const nox::reflection::Type& value_type) noexcept
		{
			for (const nox::reflection::VariableInfo& variable_info : class_info.GetVariableList())
			{
				if (IsRemoteVariable(variable_info) == false)
				{
					continue;
				}

				if (NormalizePropertyType(variable_info.GetType()) != value_type)
				{
					continue;
				}

				if (IsPropertyNameMatch(property_name, variable_info.GetName()))
				{
					return &variable_info;
				}
			}

			return nullptr;
		}

		std::span<const RemotePropertyInfo> CollectRemotePropertyInfoList(
			const nox::reflection::ClassInfo& class_info,
			nox::FixedVector<RemotePropertyInfo, k_remote_property_capacity>& storage) noexcept
		{
			for (const nox::reflection::VariableInfo& variable_info : class_info.GetVariableList())
			{
				if (IsEditorVisibleVariable(variable_info) == false)
				{
					continue;
				}

				storage.PushBack(RemotePropertyInfo
				{
					.name = variable_info.GetName(),
					.variable_info = &variable_info,
					.value_type = &NormalizePropertyType(variable_info.GetType()),
				});
			}

		for (const nox::reflection::FunctionInfo& function_info : class_info.GetFunctionList())
		{
			std::u8string_view property_name;
			const nox::reflection::Type* value_type = nullptr;
			bool is_getter = false;
			if (TryGetPropertyFunctionInfo(function_info, property_name, value_type, is_getter) == false)
			{
				continue;
			}

				const nox::int32 existing_index = FindRemotePropertyIndex(static_cast<std::span<const RemotePropertyInfo>>(storage), property_name);
				if (existing_index >= 0)
				{
					RemotePropertyInfo property_info = static_cast<std::span<const RemotePropertyInfo>>(storage)[static_cast<std::size_t>(existing_index)];
					if (property_info.value_type != nullptr)
					{
						NOX_ASSERT(*property_info.value_type == *value_type, u8"Property getter/setter type mismatch. property:{0}", property_name);
					}

					if (property_info.variable_info == nullptr)
					{
						property_info.variable_info = FindPropertyBackingVariable(class_info, property_name, *value_type);
					}
					property_info.value_type = value_type;
					if (is_getter)
					{
						property_info.getter_function = &function_info;
					}
					else
					{
						property_info.setter_function = &function_info;
					}
					std::span<RemotePropertyInfo> mutable_storage = storage;
					mutable_storage[static_cast<std::size_t>(existing_index)] = property_info;
					continue;
				}

				storage.PushBack(RemotePropertyInfo
				{
					.name = property_name,
					.variable_info = FindPropertyBackingVariable(class_info, property_name, *value_type),
					.getter_function = is_getter ? &function_info : nullptr,
					.setter_function = is_getter ? nullptr : &function_info,
					.value_type = value_type,
				});
			}

			return static_cast<std::span<const RemotePropertyInfo>>(storage);
		}

		const nox::uint8* TryGetFieldAddress(const nox::reflection::VariableInfo& variable_info, const nox::ManagedObject& obj) noexcept
		{
			if (variable_info.IsStatic())
			{
				return nullptr;
			}

			const std::int32_t offset_bits = variable_info.GetFieldOffsetBits();
			if (offset_bits < 0 || (offset_bits & 7) != 0 || variable_info.GetBitWidth() != 0)
			{
				return nullptr;
			}

			return reinterpret_cast<const nox::uint8*>(std::addressof(obj)) + (offset_bits / 8);
		}

		nox::uint8* TryGetFieldAddress(const nox::reflection::VariableInfo& variable_info, nox::ManagedObject& obj) noexcept
		{
			return const_cast<nox::uint8*>(TryGetFieldAddress(variable_info, static_cast<const nox::ManagedObject&>(obj)));
		}

		bool TryWriteBitCopyValue(
			nox::uint8*& cursor,
			nox::uint8* const buffer_end,
			const nox::reflection::VariableInfo& variable_info,
			const nox::ManagedObject& obj)
		{
			const nox::reflection::Type& type = variable_info.GetType();
			if (type.IsEnum() == false &&
				type.GetTypeKind() != nox::reflection::TypeKind::Class &&
				type.GetTypeKind() != nox::reflection::TypeKind::Union)
			{
				return false;
			}
			if (type.IsEnum() == false &&
				type.IsTypeAttributeFlag(nox::reflection::TypeAttributeFlag::TrivialCopyable) == false)
			{
				return false;
			}

			const nox::uint8* const field_address = TryGetFieldAddress(variable_info, obj);
			if (field_address == nullptr)
			{
				NOX_WARNING_LINE(nox::dev::net::log_id::DevNet, u8"ビットコピーできない変数です:{0}", variable_info.GetFullName());
				return true;
			}

			const std::size_t size = type.GetTypeSize();
			NOX_ASSERT(cursor + size <= buffer_end, u8"バッファ不足");
			nox::memory::Copy(
				static_cast<void*>(cursor),
				static_cast<const void*>(field_address),
				size);
			cursor += size;
			return true;
		}

		bool TryReadBitCopyValue(
			const nox::uint8*& cursor,
			const nox::uint8* const bytes_end,
			const nox::reflection::VariableInfo& variable_info,
			nox::ManagedObject& obj)
		{
			const nox::reflection::Type& type = variable_info.GetType();
			if (type.IsEnum() == false &&
				type.GetTypeKind() != nox::reflection::TypeKind::Class &&
				type.GetTypeKind() != nox::reflection::TypeKind::Union)
			{
				return false;
			}
			if (type.IsEnum() == false &&
				type.IsTypeAttributeFlag(nox::reflection::TypeAttributeFlag::TrivialCopyable) == false)
			{
				return false;
			}

			nox::uint8* const field_address = TryGetFieldAddress(variable_info, obj);
			if (field_address == nullptr)
			{
				NOX_WARNING_LINE(nox::dev::net::log_id::DevNet, u8"ビットコピーできない変数です:{0}", variable_info.GetFullName());
				return true;
			}

			const std::size_t size = type.GetTypeSize();
			NOX_ASSERT(cursor + size <= bytes_end, u8"バッファ不足");
			nox::memory::Copy(
				static_cast<void*>(field_address),
				static_cast<const void*>(cursor),
				size);
			cursor += size;
			return true;
		}

		template<class ResultType, class ValueType = std::remove_cvref_t<ResultType>>
		bool TryWriteInvokedValue(
			nox::uint8*& cursor,
			nox::uint8* const buffer_end,
			const nox::reflection::FunctionInfo& function_info,
			const nox::ManagedObject& obj)
		{
			std::array<void*, 1> args = { const_cast<nox::ManagedObject*>(std::addressof(obj)) };
			const auto result = static_cast<const nox::reflection::detail::FunctionInfoImpl<ResultType>&>(function_info).InvokeImpl(args);
			if (result.has_value() == false)
			{
				return false;
			}

			const ValueType& value = [&]() -> const ValueType&
			{
				if constexpr (std::is_reference_v<ResultType>)
				{
					return result->get();
				}
				else
				{
					return *result;
				}
			}();

			if constexpr (std::is_arithmetic_v<ValueType> || std::is_enum_v<ValueType>)
			{
				WriteValue(cursor, buffer_end, value);
			}
			else
			{
				WriteBytes(cursor, buffer_end, value);
			}
			return true;
		}

		bool TryWriteFunctionPropertyValue(
			nox::uint8*& cursor,
			nox::uint8* const buffer_end,
			const RemotePropertyInfo& property_info,
			const nox::ManagedObject& obj)
		{
			const nox::reflection::FunctionInfo* const getter_function = property_info.getter_function;
			const nox::reflection::Type* const value_type = property_info.value_type;
			if (getter_function == nullptr || value_type == nullptr)
			{
				return false;
			}

			switch (value_type->GetTypeKind())
			{
			case nox::reflection::TypeKind::Bool:
				return TryWriteInvokedValue<bool>(cursor, buffer_end, *getter_function, obj);
			case nox::reflection::TypeKind::Int8:
				return TryWriteInvokedValue<nox::int8>(cursor, buffer_end, *getter_function, obj);
			case nox::reflection::TypeKind::Int16:
				return TryWriteInvokedValue<nox::int16>(cursor, buffer_end, *getter_function, obj);
			case nox::reflection::TypeKind::Int32:
				return TryWriteInvokedValue<nox::int32>(cursor, buffer_end, *getter_function, obj);
			case nox::reflection::TypeKind::Int64:
				return TryWriteInvokedValue<nox::int64>(cursor, buffer_end, *getter_function, obj);
			case nox::reflection::TypeKind::UInt8:
				return TryWriteInvokedValue<nox::uint8>(cursor, buffer_end, *getter_function, obj);
			case nox::reflection::TypeKind::UInt16:
				return TryWriteInvokedValue<nox::uint16>(cursor, buffer_end, *getter_function, obj);
			case nox::reflection::TypeKind::UInt32:
				return TryWriteInvokedValue<nox::uint32>(cursor, buffer_end, *getter_function, obj);
			case nox::reflection::TypeKind::UInt64:
				return TryWriteInvokedValue<nox::uint64>(cursor, buffer_end, *getter_function, obj);
			case nox::reflection::TypeKind::Float:
				return TryWriteInvokedValue<nox::float_t>(cursor, buffer_end, *getter_function, obj);
			case nox::reflection::TypeKind::Double:
				return TryWriteInvokedValue<nox::double_t>(cursor, buffer_end, *getter_function, obj);
			case nox::reflection::TypeKind::Class:
			case nox::reflection::TypeKind::Union:
				if (*value_type == nox::reflection::Typeof<nox::Position>())
				{
					return TryWriteInvokedValue<const nox::Position&>(cursor, buffer_end, *getter_function, obj);
				}
				if (*value_type == nox::reflection::Typeof<nox::Vec3>())
				{
					return TryWriteInvokedValue<const nox::Vec3&>(cursor, buffer_end, *getter_function, obj);
				}
				if (*value_type == nox::reflection::Typeof<nox::Quat>())
				{
					return TryWriteInvokedValue<const nox::Quat&>(cursor, buffer_end, *getter_function, obj);
				}
				break;
			default:
				break;
			}

			return false;
		}

		bool TryInvokePropertySetter(const nox::reflection::FunctionInfo& function_info, nox::ManagedObject& obj, void* value_ptr)
		{
			if (function_info.IsNoReturn() == false)
			{
				return false;
			}

			std::array<void*, 2> args = { std::addressof(obj), value_ptr };
			return static_cast<const nox::reflection::detail::FunctionInfoImpl<void>&>(function_info).InvokeImpl(args).has_value();
		}

		bool TryReadFunctionPropertyValue(
			const nox::uint8*& cursor,
			const nox::uint8* const bytes_end,
			const RemotePropertyInfo& property_info,
			nox::ManagedObject& obj)
		{
			const nox::reflection::Type* const value_type = property_info.value_type;
			if (value_type == nullptr)
			{
				return false;
			}

			const nox::reflection::FunctionInfo* const setter_function = property_info.setter_function;
			if (setter_function == nullptr || CanTransportByBitCopy(setter_function->GetFunctionParam(0).GetType()) == false)
			{
				return false;
			}

			const nox::reflection::Type* const storage_type = TryGetBitCopyStorageType(setter_function->GetFunctionParam(0).GetType());
			if (storage_type == nullptr)
			{
				return false;
			}

			AlignedTypeStorage value_storage(*storage_type);
			if (value_storage.IsValid() == false)
			{
				return false;
			}

			const std::size_t value_size = value_storage.GetSize();
			NOX_ASSERT(cursor + value_size <= bytes_end, u8"バッファ不足");
			nox::memory::Copy(value_storage.Get(), cursor, value_size);
			cursor += value_size;
			return TryInvokePropertySetter(*setter_function, obj, value_storage.Get());
		}
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
	//	書き込み位置カーソル
	nox::uint8* cursor = buffer.data();
	nox::uint8* const buffer_end = buffer.data() + buffer.size();

	const auto write = [&cursor, buffer_end]<class T>(T v)
	{
		WriteValue(cursor, buffer_end, v);
	};

	//	型情報の取得
	const nox::reflection::ClassInfo& class_info = nox::util::Deref(nox::reflection::FindClassInfo(obj.GetType()));
	nox::FixedVector<RemotePropertyInfo, k_remote_property_capacity> property_storage;
	const std::span<const RemotePropertyInfo> property_list = CollectRemotePropertyInfoList(class_info, property_storage);

	for (const RemotePropertyInfo& property_info : property_list)
	{
		if (property_info.getter_function != nullptr)
		{
			if (TryWriteFunctionPropertyValue(cursor, buffer_end, property_info, obj))
			{
				continue;
			}

			if (property_info.variable_info == nullptr)
			{
				NOX_WARNING_LINE(nox::dev::net::log_id::DevNet, u8"未対応のプロパティGetterです:{0}", property_info.name);
				continue;
			}
		}
		if (property_info.variable_info == nullptr)
		{
			NOX_WARNING_LINE(nox::dev::net::log_id::DevNet, u8"プロパティの取得元が見つかりません:{0}", property_info.name);
			continue;
		}

		const nox::reflection::VariableInfo& variable_info = *property_info.variable_info;
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
		case nox::reflection::TypeKind::Enum:
		case nox::reflection::TypeKind::ScopedEnum:
		case nox::reflection::TypeKind::Class:
		case nox::reflection::TypeKind::Union:
			if (TryWriteBitCopyValue(cursor, buffer_end, variable_info, obj) == false)
			{
				NOX_WARNING_LINE(nox::dev::net::log_id::DevNet, u8"未対応の型:{0} {1}", nox::reflection::GetEnumFullName(type.GetTypeKind()), type.GetTypeName());
			}
			break;

		default:
			auto name = nox::reflection::GetEnumFullName(type.GetTypeKind());
			//	未対応
			NOX_WARNING_LINE(nox::dev::net::log_id::DevNet, u8"未対応の型:{0} {1}", name, type.GetTypeName());
			break;
		}
	}

	//	書き込んだ範囲のみ返す
	return std::span(buffer.data(), static_cast<std::size_t>(cursor - buffer.data()));
}

void nox::dev::editor_remote::SetPropertiesFromBytes(const std::span<const nox::uint8> bytes, nox::ManagedObject& obj)
{
	//	読み取り位置カーソル
	const nox::uint8* cursor = bytes.data();
	const nox::uint8* const bytes_end = bytes.data() + bytes.size();

	const auto read = [&cursor, bytes_end]<class T>(T& v) -> void
	{
		ReadValue(cursor, bytes_end, v);
	};

	//	型情報の取得
	const nox::reflection::ClassInfo& class_info = nox::util::Deref(nox::reflection::FindClassInfo(obj.GetType()));
	nox::FixedVector<RemotePropertyInfo, k_remote_property_capacity> property_storage;
	const std::span<const RemotePropertyInfo> property_list = CollectRemotePropertyInfoList(class_info, property_storage);

	for (const RemotePropertyInfo& property_info : property_list)
	{
		if (property_info.setter_function != nullptr)
		{
			if (TryReadFunctionPropertyValue(cursor, bytes_end, property_info, obj))
			{
				continue;
			}

			if (property_info.variable_info == nullptr)
			{
				NOX_WARNING_LINE(nox::dev::net::log_id::DevNet, u8"未対応のプロパティSetterです:{0}", property_info.name);
				continue;
			}
		}
		if (property_info.variable_info == nullptr)
		{
			NOX_WARNING_LINE(nox::dev::net::log_id::DevNet, u8"プロパティの設定先が見つかりません:{0}", property_info.name);
			continue;
		}

		const nox::reflection::VariableInfo& variable_info = *property_info.variable_info;
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
		case nox::reflection::TypeKind::Enum:
		case nox::reflection::TypeKind::ScopedEnum:

		case nox::reflection::TypeKind::Class:
		case nox::reflection::TypeKind::Union:
			if (TryReadBitCopyValue(cursor, bytes_end, variable_info, obj) == false)
			{
				NOX_WARNING_LINE(nox::dev::net::log_id::DevNet, u8"未対応の型:{0} {1}", nox::reflection::GetEnumFullName(type.GetTypeKind()), type.GetTypeName());
			}
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
