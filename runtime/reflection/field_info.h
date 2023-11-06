///	@file	field_info.h
///	@brief	field_info
#pragma once
#include	"type.h"

namespace nox::reflection
{
	//	前方宣言
	class ReflectionObject;

	class FieldInfo
	{
	public:
#pragma region 変数アクセスの型定義

		using SetterMemberFunc = void(*)(not_null<void*> instance_ptr, const void* const valuePtr);
		using GetterMemberFunc = void(*)(not_null<void*> outPtr, not_null<const void*> instance_ptr);
		using SetterArrayMemberFunc = bool(*)(not_null<void*> instance_ptr, const void* const valuePtr, const std::uint32_t index);
		using GetterArrayMemberFunc = bool(*)(not_null<void*> outPtr, not_null<const void*> instance_ptr, const std::uint32_t index);

		using SetterGlobalFunc = void(*)(const void* const value);
		using GetterGlobalFunc = void(*)(not_null<void*> outPtr);
		using SetterArrayGlobalFunc = bool(*)(const void* const value, const std::uint32_t index);
		using GetterArrayGlobalFunc = bool(*)(not_null<void*> outPtr, const std::uint32_t index);
#pragma endregion
	public:
		/// @brief メンバフィールド用のコンストラクタ
		/// @param name 
		/// @param fullname 
		/// @param _namespace 
		/// @param access_level 
		/// @param attribute_ptr_table 
		/// @param attribute_length 
		/// @param field_attribute_flgas 
		/// @param type 
		/// @param owner_class_type 
		/// @param setter_member_func 
		/// @param getter_member_func 
		/// @param setter_array_member_func 
		/// @param getter_array_member_func 
		/// @param setter_global_func 
		/// @param getter_global_func 
		/// @param setter_array_global_func 
		/// @param getter_array_global_func 
		inline	constexpr	explicit	FieldInfo(
			std::u8string_view name,
			std::u8string_view fullname,
			std::u8string_view _namespace,
			AccessLevel access_level,
			const class ReflectionObject* const* attribute_ptr_table,
			std::uint8_t	attribute_length,
			FieldAttributeFlag field_attribute_flgas,
			const Type& type,
			const Type& owner_class_type,
			const SetterMemberFunc setter_member_func = nullptr,
			const GetterMemberFunc getter_member_func = nullptr,
			const GetterMemberFunc getter_address_member_func = nullptr,
			const SetterArrayMemberFunc setter_array_member_func = nullptr,
			const GetterArrayMemberFunc getter_array_member_func = nullptr,
			const GetterArrayMemberFunc getter_array_address_member_func = nullptr
		)noexcept :
			name_(name),
			fullname_(fullname),
			namespace_(_namespace),
			access_level_(access_level),
			attribute_ptr_table_(attribute_ptr_table),
			attribute_length_(attribute_length),
			field_attribute_flgas_(field_attribute_flgas),
			type_(type),
			owner_class_type_(owner_class_type),
			setter_member_func_(setter_member_func),
			getter_member_func_(getter_member_func),
			getter_address_member_func_(getter_address_member_func),
			setter_array_member_func_(setter_array_member_func),
			getter_array_member_func_(getter_array_member_func),
			getter_array_address_member_func_(getter_array_address_member_func)
		{}

		/// @brief グローバルフィールド用のコンストラクタ
		/// @param name 
		/// @param fullname 
		/// @param _namespace 
		/// @param access_level 
		/// @param attribute_ptr_table 
		/// @param attribute_length 
		/// @param field_attribute_flgas 
		/// @param type 
		/// @param owner_class_type 
		/// @param setter_member_func 
		/// @param getter_member_func 
		/// @param setter_array_member_func 
		/// @param getter_array_member_func 
		/// @param setter_global_func 
		/// @param getter_global_func 
		/// @param setter_array_global_func 
		/// @param getter_array_global_func 
		inline	constexpr	explicit	FieldInfo(
			std::u8string_view name,
			std::u8string_view fullname,
			std::u8string_view _namespace,
			AccessLevel access_level,
			const class ReflectionObject* const* attribute_ptr_table,
			std::uint8_t	attribute_length,
			FieldAttributeFlag field_attribute_flgas,
			const Type& type,
			const Type& owner_class_type,
			const SetterGlobalFunc setter_global_func = nullptr,
			const GetterGlobalFunc getter_global_func = nullptr,
			const GetterGlobalFunc getter_address_global_func = nullptr,
			const SetterArrayGlobalFunc setter_array_global_func = nullptr,
			const GetterArrayGlobalFunc getter_array_global_func = nullptr,
			const GetterArrayGlobalFunc getter_array_address_global_func = nullptr
		)noexcept :
			name_(name),
			fullname_(fullname),
			namespace_(_namespace),
			access_level_(access_level),
			attribute_ptr_table_(attribute_ptr_table),
			attribute_length_(attribute_length),
			field_attribute_flgas_(field_attribute_flgas),
			type_(type),
			owner_class_type_(owner_class_type),
			setter_global_func_(setter_global_func),
			getter_global_func_(getter_global_func),
			getter_address_global_func_(getter_address_global_func),
			setter_array_global_func_(setter_array_global_func),
			getter_array_global_func_(getter_array_global_func),
			getter_array_address_global_func_(getter_array_address_global_func)
		{}


#pragma region アクセサ
		/// @brief 名前
		inline	constexpr	std::u8string_view	GetName()const noexcept { return name_; }

		/// @brief 完全な名前
		inline	constexpr	std::u8string_view	GetFullName()const noexcept { return fullname_; }

		/// @brief 名前空間
		inline	constexpr	std::u8string_view	GetNamespace()const noexcept { return namespace_; }

		/// @brief 所属するクラス情報を取得する
		const class ClassInfo* GetOwnerClass()const noexcept;

		inline	constexpr	std::uint32_t GetTypeID()const noexcept { return type_.GetTypeID(); }
		inline	constexpr	const Type& GetType()const noexcept { return type_; }
		inline	constexpr	AccessLevel	GetAccessLevel()const noexcept { return access_level_; }
		inline	constexpr	std::span<const class ReflectionObject*const>	GetAttributeList()const noexcept { return std::span(attribute_ptr_table_, attribute_length_); }


		inline constexpr	bool	IsFieldAttributeFlag(const FieldAttributeFlag flag)const noexcept { return util::IsBitAnd(field_attribute_flgas_, flag); }

		/// @brief メンバ変数か
		inline	constexpr	bool	IsMember()const noexcept { return IsFieldAttributeFlag(FieldAttributeFlag::Member); }

		/// @brief 読み取り専用
		inline	constexpr	bool	IsReadOnly()const noexcept { return type_.IsConstQualified(); }
#pragma endregion

#pragma region 変数の取得
		/// @brief メンバ変数を取得
		/// @tparam _ReturnType 
		/// @tparam _InstanceType 
		/// @param out_value 
		/// @param owner_instance 
		/// @return 
		template<class _ReturnType, concepts::ClassUnion _InstanceType>
		inline	constexpr	bool	TryGetValue(_ReturnType& out_value, const _InstanceType& owner_instance)const
		{
			return TryGetValueMemberImpl(
				static_cast<void*>(&out_value),
				Typeof<_ReturnType>(),
				static_cast<const void*>(&owner_instance),
				Typeof<_InstanceType>()
			);
		}

		/// @brief メンバ変数のアドレスを取得
		/// @tparam _InstanceType 
		/// @tparam _ReturnType 
		/// @param out_value 
		/// @param owner_instance 
		/// @return 
		template<concepts::Pointer _ReturnType, concepts::ClassUnion _InstanceType> 
		inline	constexpr	bool	TryGetValueAddress(_ReturnType& out_value, _InstanceType& owner_instance)const
		{
			return TryGetValueAddressMemberImpl(
				static_cast<void*>(&out_value),
				Typeof<std::remove_pointer_t<_ReturnType>>(),
				static_cast<const void*>(&owner_instance),
				Typeof<_InstanceType>()
			);
		}

		/// @brief グローバル変数を取得
		/// @tparam _ReturnType 
		/// @param out_value 
		/// @return 
		template<class _ReturnType>
		inline	constexpr	bool	TryGetValue(_ReturnType& out_value)const
		{
			return TryGetValueGlobalImpl(
				static_cast<void*>(&out_value),
				Typeof<_ReturnType>()
			);
		}

		template<concepts::Pointer _ReturnType>
		inline	constexpr	bool	TryGetValueAddress(_ReturnType& out_value)const
		{
			return TryGetValueAddressGlobalImpl(
				static_cast<void*>(&out_value),
				Typeof<_ReturnType>()
			);
		}
#pragma endregion
	private:

#pragma region 呼び出しチェック関数
		/// @brief メンバ関数アクセス時のチェック
		/// @return 
		inline	constexpr	bool	CheckMemberAccess(const Type& return_type, const Type& owner_class_type)const noexcept
		{
			if (IsMember() == false)
			{
				return false;
			}

			if (type_.IsConvertible(return_type) == false)
			{
				return false;
			}

			if (owner_class_type != owner_class_type_)
			{
				return false;
			}

			return true;
		}

		inline constexpr	bool	CHeckAccess(const Type& return_type)const noexcept
		{
			if (IsMember() == true)
			{
				return false;
			}

			if (type_.IsConvertible(return_type) == false)
			{
				return false;
			}

			return true;
		}
#pragma endregion


#pragma region 変数の取得の内部実装

		inline	constexpr	bool	TryGetValueMemberImpl(not_null<void*> out_ptr, const Type& out_type, not_null<const void*> ownerInstancePtr, const Type& owner_class_type)const
		{
			if (getter_member_func_ == nullptr)
			{
				return false;
			}

			if (CheckMemberAccess(out_type, owner_class_type) == false)
			{
				return false;
			}

			std::invoke(getter_member_func_, out_ptr, ownerInstancePtr);

			return true;
		}

		inline	constexpr	bool	TryGetValueAddressMemberImpl(not_null<void*> out_ptr,  const Type& out_pointee_type, not_null<const void*> ownerInstancePtr, const Type& owner_class_type)const
		{
			if (getter_address_member_func_ == nullptr)
			{
				return false;
			}

			if (CheckMemberAccess(out_pointee_type, owner_class_type) == false)
			{
				return false;
			}

			std::invoke(getter_address_member_func_, out_ptr, ownerInstancePtr);

			return true;
		}

		constexpr bool TryGetValueGlobalImpl(not_null<void*> outPtr, const Type& out_type, const Type& getsetParamCannonicalType)const
		{
			if (getter_global_func_ == nullptr)
			{
				return false;
			}

			if (CHeckAccess(out_type) == false)
			{
				return false;
			}

			std::invoke(getter_global_func_, outPtr);
			return true;
		}
#pragma endregion
	private:
		/// @brief 名前
		std::u8string_view name_;

		/// @brief 完全な名前
		std::u8string_view fullname_;

		/// @brief 名前空間
		std::u8string_view namespace_;

		FieldAttributeFlag field_attribute_flgas_;

		/// @brief 属性テーブル
		const class ReflectionObject*const* attribute_ptr_table_;

		/// @brief 属性テーブルの長さ
		std::uint8_t attribute_length_;

		/// @brief 自身のタイプ情報
		const Type& type_;

		/// @brief 保持クラスのタイプ情報
		const Type& owner_class_type_;

		/// @brief アクセスレベル
		AccessLevel access_level_;

		/// @brief Setter
		union
		{
			const SetterMemberFunc setter_member_func_;
			const SetterGlobalFunc setter_global_func_;
		};

		/// @brief Getter
		union
		{
			const GetterMemberFunc getter_member_func_;
			const GetterGlobalFunc getter_global_func_;
		};

		/// @brief getter address
		union
		{
			const GetterMemberFunc getter_address_member_func_;
			const GetterGlobalFunc getter_address_global_func_;
		};


		/// @brief 配列setter
		union
		{
			const SetterArrayMemberFunc setter_array_member_func_;
			const SetterArrayGlobalFunc setter_array_global_func_;
		};

		/// @brief 配列getter
		union
		{
			const GetterArrayMemberFunc getter_array_member_func_;
			const GetterArrayGlobalFunc getter_array_global_func_;
		};
		
		/// @brief 配列getter address
		union
		{
			const GetterArrayMemberFunc getter_array_address_member_func_;
			const GetterArrayGlobalFunc getter_array_address_global_func_;
		};
	};

	namespace detail
	{
		/// @brief メンバフィールドのフィールド情報を作成
		/// @tparam _OwnerType 
		/// @param name 
		/// @param fullname 
		/// @param _namespace 
		/// @param access_level 
		/// @param attribute_ptr_table 
		/// @param attribute_length 
		/// @param field_attribute_flgas 
		/// @param type 
		/// @param setter_member_func 
		/// @param getter_member_func 
		/// @param setter_array_member_func 
		/// @param getter_array_member_func 
		/// @return 
		template<concepts::ClassUnion _OwnerType>
		inline constexpr	FieldInfo	CreateFieldInfo(
			std::u8string_view name,
			std::u8string_view fullname,
			std::u8string_view _namespace,
			AccessLevel access_level,
			const class ReflectionObject* const* attribute_ptr_table,
			std::uint8_t	attribute_length,
			const Type& type,
			const FieldInfo::SetterMemberFunc setter_member_func = nullptr,
			const FieldInfo::GetterMemberFunc getter_member_func = nullptr,
			const FieldInfo::GetterMemberFunc getter_address_member_func = nullptr,
			const FieldInfo::SetterArrayMemberFunc setter_array_member_func = nullptr,
			const FieldInfo::GetterArrayMemberFunc getter_array_member_func = nullptr,
			const FieldInfo::GetterArrayMemberFunc getter_array_address_member_func = nullptr
		)noexcept
		{
			return FieldInfo(
				name,
				fullname,
				_namespace,
				access_level,
				attribute_ptr_table,
				attribute_length,
				FieldAttributeFlag::Member,
				type,
				Typeof< _OwnerType>(),
				setter_member_func,
				getter_member_func,
				getter_address_member_func,
				setter_array_member_func,
				getter_array_member_func,
				getter_array_address_member_func
			);
		}

		/*inline constexpr	FieldInfo	CreateFieldInfo(
			std::u8string_view name,
			std::u8string_view fullname,
			std::u8string_view _namespace,
			AccessLevel access_level,
			const class ReflectionObject* const* attribute_ptr_table,
			std::uint8_t	attribute_length,
			const Type& type,
			const FieldInfo::SetterGlobalFunc setter_member_func = nullptr,
			const FieldInfo::GetterGlobalFunc getter_member_func = nullptr,
			const FieldInfo::SetterArrayGlobalFunc setter_array_member_func = nullptr,
			const FieldInfo::GetterArrayGlobalFunc getter_array_member_func = nullptr
		)noexcept
		{
			return FieldInfo(
				name,
				fullname,
				_namespace,
				access_level,
				attribute_ptr_table,
				attribute_length,
				FieldAttributeFlag::None,
				type,
				InvalidType,
				setter_member_func,
				getter_member_func,
				setter_array_member_func,
				getter_array_member_func
			);
		}*/
	}
}

#pragma region マクロ関連
#pragma region メンバ関数
///	@brief	メンバ変数へのセット
#define	NOX_FIELD_INFO_LAMBDA_SETTER(ClassName, FieldName) [](nox::not_null<void*> instance_ptr, const void* const value){\
	static_cast<ClassName*>(instance_ptr.get())->FieldName = *static_cast<nox::AddConstPointerType<std::add_pointer_t<decltype(ClassName::FieldName)>>>(value);\
	}

///	@brief	メンバ変数の取得
#define	NOX_FIELD_INFO_LAMBDA_GETTER(ClassName, FieldName) [](nox::not_null<void*> outValue, nox::not_null<const void*> instance_ptr) {\
	*static_cast<nox::RemoveConstPointerReferenceType<std::add_pointer_t<decltype(ClassName::FieldName)>>>(outValue.get()) = static_cast<const ClassName*>(instance_ptr.get())->FieldName;\
}

///	@brief	メンバ変数アドレスの取得
#define	NOX_FIELD_INFO_LAMBDA_GETTER_ADDRESS(ClassName, FieldName) [](nox::not_null<void*> outValue, nox::not_null<const void*> instance_ptr) {\
	*static_cast<nox::RemoveConstPointerReferenceType<std::add_pointer_t<decltype(ClassName::FieldName)>>>(outValue.get()) = static_cast<const ClassName*>(instance_ptr.get())->FieldName;\
}


#define	NOX_FIELD_INFO_LAMBDA_SETTER_ARRAY(ClassName, FieldName) [](nox::not_null<void*> instance_ptr, const void* const value, const u32 index) {\
	if (nox::util::IsValidIndex(static_cast<const ClassName*>(instance_ptr.get())->FieldName, index) == false) {\
		return false;\
	}\
	static_cast<ClassName*>(instance_ptr.get())->FieldName[index] = *static_cast<nox::AddConstPointerType<std::add_pointer_t<nox::ContainerElementType<decltype(ClassName::FieldName)>>>>(value);\
	return true;\
	}

#define	NOX_FIELD_INFO_LAMBDA_GETTER_ARRAY(ClassName, FieldName) [](nox::not_null<void*> outValue, nox::not_null<const void*> instance_ptr, const u32 index) {\
	if (nox::util::IsValidIndex(static_cast<const ClassName*>(instance_ptr.get())->FieldName, index) == false) {\
		return false;\
	}\
	*static_cast<RemoveConstPointerReferenceType<std::add_pointer_t<nox::ContainerElementType<decltype(ClassName::FieldName)>>>>(outValue.get()) = static_cast<const ClassName*>(instance_ptr.get())->FieldName[index];\
	return true;\
	}

#define NOX_FIELD_INFO_LAMBDA_ADDRESS_GETTER(ClassName, FieldName) [](nox::not_null<void*> outValue, nox::not_null<const void*> instance_ptr) {\
	*static_cast<std::add_pointer_t<nox::AddConstPointerType<std::add_pointer_t<decltype(ClassName::FieldName)>>>>(outValue.get()) = &static_cast<const ClassName*>(instance_ptr.get())->FieldName;\
}


	//	グローバル版
#define	NOX_FIELD_INFO_LAMBDA_SETTER_GLOBAL(FieldName) []( const void* const value){\
	FieldName = *static_cast<nox::AddConstPointerType<std::add_pointer_t<decltype(FieldName)>>>(value);\
	}

#define	NOX_FIELD_INFO_LAMBDA_GETTER_GLOBAL(FieldName) [](not_null<void*> outValue) {\
	*static_cast<RemoveConstPointerReferenceType<std::add_pointer_t<decltype(FieldName)>>>(outValue.get()) = FieldName;\
}

#define	NOX_FIELD_INFO_LAMBDA_SETTER_ARRAY_GLOBAL(FieldName) [](const void* const value, const u32 index) {\
	if (nox::util::IsValidIndex(FieldName, index) == false) {\
		return false;\
	}\
	FieldName[index] = *static_cast<nox::AddConstPointerType<std::add_po1inter_t<nox::ContainerElementType<decltype(FieldName)>>>>(value);\
	return true;\
	}

#define	NOX_FIELD_INFO_LAMBDA_GETTER_ARRAY_GLOBAL(FieldName) [](not_null<void*> outValue, const u32 index) {\
	if (nox::util::IsValidIndex(FieldName, index) == false) {\
		return false;\
	}\
	*static_cast<RemoveConstPointerReferenceType<std::add_pointer_t<nox::ContainerElementType<decltype(FieldName)>>>>(outValue.get()) = FieldName[index];\
	return true;\
	}
#pragma endregion

#pragma region ラムダ式作成用ラムダ
//	メンバ
#define	NOX_FIELD_INFO_CREATE_LAMBDA_SETTER(ClassName, FieldName) []()constexpr{\
		using _T = decltype(ClassName::FieldName);\
		if constexpr (std::is_array_v<_T> || std::is_const_v<_T> == true)\
		{\
			return nullptr;\
		}\
		else\
		{\
			if constexpr (std::is_class_v<_T> == false)\
			{\
				return NOX_FIELD_INFO_LAMBDA_SETTER(ClassName, FieldName); \
			}\
			else if constexpr (nox::IsInvokableDefaultOperatorValue<_T> == true)\
			{\
				return NOX_FIELD_INFO_LAMBDA_SETTER(ClassName, FieldName); \
			}\
			else\
			{\
				return nullptr; \
			}\
		}\
		}()


#define	NOX_FIELD_INFO_CREATE_LAMBDA_GETTER(ClassName, FieldName) []()constexpr{\
		using _T = decltype(ClassName::FieldName);\
		if constexpr (std::is_array_v<_T>)\
		{\
			return nullptr;\
		}\
		else\
		{\
			if constexpr (std::is_class_v<_T> == false)\
			{\
				return NOX_FIELD_INFO_LAMBDA_GETTER(ClassName, FieldName);\
			}\
			else if constexpr (nox::IsInvokableDefaultOperatorValue<_T> == true)\
			{\
				return NOX_FIELD_INFO_LAMBDA_GETTER(ClassName, FieldName);\
			}\
			else\
			{\
				return nullptr;\
			}\
		}\
		}()

#define	NOX_FIELD_INFO_CREATE_LAMBDA_SETTER_ARRAY(ClassName, FieldName) []()constexpr{\
		using _T = decltype(ClassName::FieldName);\
		if constexpr ((std::is_array_v<_T> == false && nox::IsSequenceContainerClassValue<_T> == false) || std::is_const_v<RemoveExtentArraySequenceContainerT<_T>> == true)\
		{\
			return nullptr;\
		}\
		else\
		{\
			if constexpr (std::is_class_v<_T> == false)\
			{\
				return NOX_FIELD_INFO_LAMBDA_SETTER_ARRAY(ClassName, FieldName); \
			}\
			else if constexpr (nox::HasIndexOperatorValue<_T> == true)\
			{\
				return NOX_FIELD_INFO_LAMBDA_SETTER_ARRAY(ClassName, FieldName); \
			}\
			else\
			{\
				return nullptr; \
			}\
		}\
		}()

#define	NOX_FIELD_INFO_CREATE_LAMBDA_GETTER_ARRAY(ClassName, FieldName) []()constexpr{\
		using _T = decltype(ClassName::FieldName);\
		if constexpr (std::is_array_v<_T> == false && nox::IsSequenceContainerClassValue<_T> == false)\
		{\
			return nullptr;\
		}\
		else\
		{\
			if constexpr (std::is_class_v<_T> == false)\
			{\
				return NOX_FIELD_INFO_LAMBDA_GETTER_ARRAY(ClassName, FieldName); \
			}\
			else if constexpr (nox::HasIndexOperatorValue<_T> == true)\
			{\
				return NOX_FIELD_INFO_LAMBDA_GETTER_ARRAY(ClassName, FieldName); \
			}\
			else\
			{\
				return nullptr; \
			}\
		}\
		}()

//	global
#define	NOX_FIELD_INFO_CREATE_LAMBDA_SETTER_GLOBAL(FieldName) []()constexpr{\
		using _T = decltype(FieldName);\
		if constexpr (std::is_array_v<_T> || std::is_const_v<_T> == true)\
		{\
			return nullptr;\
		}\
		else\
		{\
			if constexpr (std::is_class_v<_T> == false)\
			{\
				return NOX_FIELD_INFO_LAMBDA_SETTER_GLOBAL(FieldName); \
			}\
			else if constexpr (nox::IsInvokableDefaultOperatorValue<_T> == true)\
			{\
				return NOX_FIELD_INFO_LAMBDA_SETTER_GLOBAL(FieldName); \
			}\
			else\
			{\
				return nullptr; \
			}\
		}\
		}()


#define	NOX_FIELD_INFO_CREATE_LAMBDA_GETTER_GLOBAL(FieldName) []()constexpr{\
		using _T = decltype(FieldName);\
		if constexpr (std::is_array_v<_T>)\
		{\
			return nullptr;\
		}\
		else\
		{\
			if constexpr (std::is_class_v<_T> == false)\
			{\
				return NOX_FIELD_INFO_LAMBDA_GETTER_GLOBAL(FieldName);\
			}\
			else if constexpr (nox::IsInvokableDefaultOperatorValue<_T> == true)\
			{\
				return NOX_FIELD_INFO_LAMBDA_GETTER_GLOBAL(FieldName);\
			}\
			else\
			{\
				return nullptr;\
			}\
		}\
		}()

#define	NOX_FIELD_INFO_CREATE_LAMBDA_SETTER_ARRAY_GLOBAL(FieldName) []()constexpr{\
		using _T = decltype(FieldName);\
		if constexpr ((std::is_array_v<_T> == false && nox::IsSequenceContainerClassValue<_T> == false) || std::is_const_v<RemoveExtentArraySequenceContainerT<_T>> == true)\
		{\
			return nullptr;\
		}\
		else\
		{\
			if constexpr (std::is_class_v<_T> == false)\
			{\
				return NOX_FIELD_INFO_LAMBDA_SETTER_ARRAY_GLOBAL(FieldName); \
			}\
			else if constexpr (nox::HasIndexOperatorValue<_T> == true)\
			{\
				return NOX_FIELD_INFO_LAMBDA_SETTER_ARRAY_GLOBAL(FieldName); \
			}\
			else\
			{\
				return nullptr; \
			}\
		}\
		}()

#define	NOX_FIELD_INFO_CREATE_LAMBDA_GETTER_ARRAY_GLOBAL(FieldName) []()constexpr{\
		using _T = decltype(FieldName);\
		if constexpr (std::is_array_v<_T> == false && nox::IsSequenceContainerClassValue<_T> == false)\
		{\
			return nullptr;\
		}\
		else\
		{\
			if constexpr (std::is_class_v<_T> == false)\
			{\
				return NOX_FIELD_INFO_LAMBDA_GETTER_ARRAY_GLOBAL(FieldName); \
			}\
			else if constexpr (nox::HasIndexOperatorValue<_T> == true)\
			{\
				return NOX_FIELD_INFO_LAMBDA_GETTER_ARRAY_GLOBAL(FieldName); \
			}\
			else\
			{\
				return nullptr; \
			}\
		}\
		}()

#pragma endregion


#pragma endregion
