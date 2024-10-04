///	@file	field_info.h
///	@brief	field_info
#pragma once
#include	"type.h"
#include	"reflection_object.h"

namespace nox::reflection
{
	//	前方宣言

	class VariableInfo
	{
	public:
#pragma region 変数アクセスの型定義

		using SetterMemberFunc = void(*)(nox::not_null<void*> instance, nox::not_null<void*> value);
		using GetterMemberFunc = void(*)(not_null<void*> out, not_null<const void*> instance);
		using SetterArrayMemberFunc = bool(*)(not_null<void*> instance, const void* const valuePtr, const std::uint32_t index);
		using GetterArrayMemberFunc = bool(*)(not_null<void*> out, not_null<const void*> instance, const std::uint32_t index);

		using SetterGlobalFunc = void(*)(const void* const value);
		using GetterGlobalFunc = void(*)(not_null<void*> out);
		using SetterArrayGlobalFunc = bool(*)(const void* const value, const std::uint32_t index);
		using GetterArrayGlobalFunc = bool(*)(not_null<void*> out, const std::uint32_t index);
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
		inline	constexpr	explicit	VariableInfo(
			const ReflectionStringView name,
			const ReflectionStringView fullname,
			const ReflectionStringView _namespace,
			nox::reflection::AccessLevel access_level,
			const std::uint32_t bit_width,
			const std::uint32_t field_offset,
			const std::reference_wrapper<const class nox::reflection::ReflectionObject>* attribute_list,
			const std::uint8_t	attribute_list_length,
			const nox::reflection::FieldAttributeFlag field_attribute_flgas,
			const nox::reflection::Type& type,
			const nox::reflection::Type& owner_class_type,
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
			bit_width_(bit_width),
			field_offset_(field_offset),
			attribute_list_(attribute_list),
			attribute_list_length_(attribute_list_length),
			field_attribute_flgas_(field_attribute_flgas),
			type_(type),
			containing_type_(owner_class_type),
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
		inline	constexpr	explicit	VariableInfo(
			ReflectionStringView name,
			ReflectionStringView fullname,
			ReflectionStringView _namespace,
			nox::reflection::AccessLevel access_level,
			const std::uint32_t bit_width,
			const std::uint32_t field_offset,
			const std::reference_wrapper<const class nox::reflection::ReflectionObject>* attribute_list,
			std::uint8_t	attribute_list_length,
			nox::reflection::FieldAttributeFlag field_attribute_flgas,
			const nox::reflection::Type& type,
			const nox::reflection::Type& owner_class_type,
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
			bit_width_(bit_width),
			field_offset_(field_offset),
			attribute_list_(attribute_list),
			attribute_list_length_(attribute_list_length),
			field_attribute_flgas_(field_attribute_flgas),
			type_(type),
			containing_type_(owner_class_type),
			setter_global_func_(setter_global_func),
			getter_global_func_(getter_global_func),
			getter_address_global_func_(getter_address_global_func),
			setter_array_global_func_(setter_array_global_func),
			getter_array_global_func_(getter_array_global_func),
			getter_array_address_global_func_(getter_array_address_global_func)
		{}


#pragma region アクセサ
		/// @brief 名前
		inline	constexpr	ReflectionStringView	GetName()const noexcept { return name_; }

		/// @brief 完全な名前
		inline	constexpr	ReflectionStringView	GetFullName()const noexcept { return fullname_; }

		/// @brief 名前空間
		inline	constexpr	ReflectionStringView	GetNamespace()const noexcept { return namespace_; }

		/// @brief 所属するクラス情報を取得する
		const class nox::reflection::UserDefinedCompoundTypeInfo* GetContainingUserDefinedCompoundTypeInfo()const noexcept;

		inline	constexpr	const nox::reflection::Type& GetType()const noexcept { return type_; }
		inline	constexpr	nox::reflection::AccessLevel	GetAccessLevel()const noexcept { return access_level_; }

		/// @brief 属性リストを取得
		inline	constexpr	std::span<const std::reference_wrapper< const class nox::reflection::ReflectionObject>>	GetAttributeList()const noexcept { return std::span(attribute_list_, attribute_list_length_); }
		inline	constexpr	std::uint8_t	GetAttributeListLength()const noexcept { return attribute_list_length_; }
		inline	constexpr	const class nox::reflection::ReflectionObject& GetAttribute(const std::uint8_t index)const noexcept { return util::At(attribute_list_, attribute_list_length_, index); }

		inline constexpr	bool	IsFieldAttributeFlag(const FieldAttributeFlag flag)const noexcept { return util::IsBitAnd(field_attribute_flgas_, flag); }

		/// @brief メンバ変数か
		inline	constexpr	bool	IsStatic()const noexcept { return IsFieldAttributeFlag(FieldAttributeFlag::Static); }

		/// @brief 読み取り専用
		inline	constexpr	bool	IsReadOnly()const noexcept { return type_.IsConstQualified(); }

		virtual std::uint64_t GetID()const noexcept = 0;
#pragma endregion

#pragma region 変数の取得
		/// @brief メンバ変数を取得
		/// @tparam _ReturnType 
		/// @tparam _InstanceType 
		/// @param out_value 
		/// @param owner_instance 
		/// @return 
		template<class _ReturnType, concepts::UserDefinedCompoundType _InstanceType>
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
		template<concepts::Pointer _ReturnType, concepts::UserDefinedCompoundType _InstanceType> 
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
			if (IsStatic() == true)
			{
				return false;
			}

			if (type_.IsConvertible(return_type) == false)
			{
				return false;
			}

			if (owner_class_type != containing_type_)
			{
				return false;
			}

			return true;
		}

		inline constexpr	bool	CHeckAccess(const Type& return_type)const noexcept
		{
			if (IsStatic() == false)
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
		/// @brief 属性テーブルの長さ
		const std::uint8_t attribute_list_length_;

		/// @brief 値の属性
		const nox::reflection::FieldAttributeFlag field_attribute_flgas_;

		/// @brief アクセスレベル
		const nox::reflection::AccessLevel access_level_;

		const std::uint32_t field_offset_;
		const std::uint32_t bit_width_;

		/// @brief 属性テーブル
		const std::reference_wrapper<const class nox::reflection::ReflectionObject>* attribute_list_;

		/// @brief 自身のタイプ情報
		const nox::reflection::Type& type_;

		/// @brief 保持クラスのタイプ情報
		const nox::reflection::Type& containing_type_;
	
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

		/// @brief getter address_
		union
		{
			const GetterMemberFunc getter_address_member_func_;
			const GetterGlobalFunc getter_address_global_func_;
		};


		/// @brief 配列 setter
		union
		{
			const SetterArrayMemberFunc setter_array_member_func_;
			const SetterArrayGlobalFunc setter_array_global_func_;
		};

		/// @brief 配列 getter
		union
		{
			const GetterArrayMemberFunc getter_array_member_func_;
			const GetterArrayGlobalFunc getter_array_global_func_;
		};
		
		/// @brief 配列 getter address_
		union
		{
			const GetterArrayMemberFunc getter_array_address_member_func_;
			const GetterArrayGlobalFunc getter_array_address_global_func_;
		};

		/// @brief 名前
		const ReflectionStringView name_;

		/// @brief 完全な名前
		const ReflectionStringView fullname_;

		/// @brief 名前空間
		const ReflectionStringView namespace_;

	};

	namespace detail
	{
		template<class T> requires(std::is_pointer_v<T> || std::is_member_object_pointer_v<T>)
		class VariableInfoImpl final: public VariableInfo
		{
		public:
			inline constexpr VariableInfoImpl(
				const T& object_pointer,
				const ReflectionStringView name,
				const ReflectionStringView fullname,
				const ReflectionStringView _namespace,
				nox::reflection::AccessLevel access_level,
				const std::uint32_t bit_width,
				const std::uint32_t field_offset,
				const std::reference_wrapper<const class nox::reflection::ReflectionObject>* attribute_list,
				const std::uint8_t	attribute_list_length,
				const nox::reflection::FieldAttributeFlag field_attribute_flgas,
				const nox::reflection::Type& type,
				const nox::reflection::Type& owner_class_type,
				const SetterMemberFunc setter_member_func = nullptr,
				const GetterMemberFunc getter_member_func = nullptr,
				const GetterMemberFunc getter_address_member_func = nullptr,
				const SetterArrayMemberFunc setter_array_member_func = nullptr,
				const GetterArrayMemberFunc getter_array_member_func = nullptr,
				const GetterArrayMemberFunc getter_array_address_member_func = nullptr
			)noexcept:
				VariableInfo(
					name,
					fullname,
					_namespace,
					access_level,
					bit_width,
					field_offset,
					attribute_list,
					attribute_list_length,
					field_attribute_flgas,
					type,
					owner_class_type,
					setter_member_func,
					getter_member_func,
					getter_address_member_func,
					setter_array_member_func,
					getter_array_member_func,
					getter_array_address_member_func
				),
				object_pointer_(object_pointer)
			{}

			inline constexpr VariableInfoImpl(
				const T& object_pointer,
				ReflectionStringView name,
				ReflectionStringView fullname,
				ReflectionStringView _namespace,
				nox::reflection::AccessLevel access_level,
				const std::uint32_t bit_width,
				const std::uint32_t field_offset,
				const std::reference_wrapper<const class nox::reflection::ReflectionObject>* attribute_list,
				std::uint8_t	attribute_list_length,
				nox::reflection::FieldAttributeFlag field_attribute_flgas,
				const nox::reflection::Type& type,
				const nox::reflection::Type& owner_class_type,
				const SetterGlobalFunc setter_global_func = nullptr,
				const GetterGlobalFunc getter_global_func = nullptr,
				const GetterGlobalFunc getter_address_global_func = nullptr,
				const SetterArrayGlobalFunc setter_array_global_func = nullptr,
				const GetterArrayGlobalFunc getter_array_global_func = nullptr,
				const GetterArrayGlobalFunc getter_array_address_global_func = nullptr
			)noexcept:
				VariableInfo(
					name,
					fullname,
					_namespace,
					access_level,
					bit_width,
					field_offset,
					attribute_list,
					attribute_list_length,
					field_attribute_flgas,
					type,
					owner_class_type,
					setter_global_func,
					getter_global_func,
					getter_address_global_func,
					setter_array_global_func,
					getter_array_global_func,
					getter_array_address_global_func
				),
				object_pointer_(object_pointer)
			{}

			inline std::uint64_t GetID()const noexcept override
			{
				if constexpr (std::is_member_object_pointer_v<T> == true)
				{
					const void* const& p = reinterpret_cast<const void* const&>(object_pointer_);
					return reinterpret_cast<std::uint64_t>(p);
				}
				else
				{
					return reinterpret_cast<std::uint64_t>(object_pointer_);
				}
			}

		private:
			const T& object_pointer_;
		};

		template<class T>
		inline constexpr nox::reflection::detail::VariableInfoImpl<T> CreateVariableInfo(
			const T& object_pointer,
			ReflectionStringView name,
			ReflectionStringView fullname,
			ReflectionStringView _namespace,
			nox::reflection::AccessLevel access_level,
			const std::uint32_t bit_width,
			const std::uint32_t field_offset,
			const std::reference_wrapper<const class nox::reflection::ReflectionObject>* attribute_list,
			std::uint8_t	attribute_list_length,
			bool is_constexpr,
			bool is_constinit,
			const VariableInfo::SetterMemberFunc setter_member_func = nullptr,
			const VariableInfo::GetterMemberFunc getter_member_func = nullptr,
			const VariableInfo::GetterMemberFunc getter_address_member_func = nullptr,
			const VariableInfo::SetterArrayMemberFunc setter_array_member_func = nullptr,
			const VariableInfo::GetterArrayMemberFunc getter_array_member_func = nullptr,
			const VariableInfo::GetterArrayMemberFunc getter_array_address_member_func = nullptr)noexcept
		{
			nox::reflection::FieldAttributeFlag field_attribute_flgas = nox::reflection::GetFieldAttributeFlags<T>();
			if (is_constexpr == true)
			{
				field_attribute_flgas |= nox::reflection::FieldAttributeFlag::Constexpr;
			}
			if (is_constinit == true)
			{
				field_attribute_flgas |= nox::reflection::FieldAttributeFlag::Constinit;
			}

			return nox::reflection::detail::VariableInfoImpl<T>(
				object_pointer,
				name,
				fullname,
				_namespace,
				access_level,
				bit_width,
				field_offset,
				attribute_list,
				attribute_list_length,
				field_attribute_flgas,
				nox::reflection::Typeof<nox::FieldReturnType<T>>(),
				nox::reflection::Typeof<nox::FieldClassType<T>>(),
				setter_member_func,
				getter_member_func,
				getter_address_member_func,
				setter_array_member_func,
				getter_array_member_func,
				getter_array_address_member_func
			);
		}

		template<class T>
		inline constexpr nox::reflection::detail::VariableInfoImpl<T> CreateVariableInfo(
			const T& object_pointer,
			ReflectionStringView name,
			ReflectionStringView fullname,
			ReflectionStringView _namespace,
			nox::reflection::AccessLevel access_level,
			const std::uint32_t bit_width,
			const std::uint32_t field_offset,
			const std::reference_wrapper<const class nox::reflection::ReflectionObject>* attribute_list,
			std::uint8_t	attribute_list_length,
			bool is_constexpr,
			bool is_constinit,
			const VariableInfo::SetterGlobalFunc setter_global_func = nullptr,
			const VariableInfo::GetterGlobalFunc getter_global_func = nullptr,
			const VariableInfo::GetterGlobalFunc getter_address_global_func = nullptr,
			const VariableInfo::SetterArrayGlobalFunc setter_array_global_func = nullptr,
			const VariableInfo::GetterArrayGlobalFunc getter_array_global_func = nullptr,
			const VariableInfo::GetterArrayGlobalFunc getter_array_address_global_func = nullptr)noexcept
		{
			nox::reflection::FieldAttributeFlag field_attribute_flgas = nox::reflection::GetFieldAttributeFlags<T>();
			if (is_constexpr == true)
			{
				field_attribute_flgas |= nox::reflection::FieldAttributeFlag::Constexpr;
			}
			if (is_constinit == true)
			{
				field_attribute_flgas |= nox::reflection::FieldAttributeFlag::Constinit;
			}

			return nox::reflection::detail::VariableInfoImpl<T>(
				object_pointer,
				name,
				fullname,
				_namespace,
				access_level,
				bit_width,
				field_offset,
				attribute_list,
				attribute_list_length,
				field_attribute_flgas,
				nox::reflection::Typeof<nox::FieldReturnType<T>>(),
				nox::reflection::InvalidType,
				setter_global_func,
				getter_global_func,
				getter_address_global_func,
				setter_array_global_func,
				getter_array_global_func,
				getter_array_address_global_func
			);
		}

		///// @brief メンバフィールドのフィールド情報を作成
		///// @tparam _OwnerType 
		///// @param name 
		///// @param fullname 
		///// @param _namespace 
		///// @param access_level 
		///// @param attribute_ptr_table 
		///// @param attribute_length 
		///// @param field_attribute_flgas 
		///// @param type 
		///// @param setter_member_func 
		///// @param getter_member_func 
		///// @param setter_array_member_func 
		///// @param getter_array_member_func 
		///// @return 
		//template<concepts::UserDefinedCompoundType _OwnerType>
		//inline constexpr	nox::reflection::VariableInfo	CreateFieldInfo(
		//	ReflectionStringView name,
		//	ReflectionStringView fullname,
		//	ReflectionStringView _namespace,
		//	nox::reflection::AccessLevel access_level,
		//	const std::reference_wrapper<const class nox::reflection::ReflectionObject>* attribute_list,
		//	std::uint8_t	attribute_list_length,
		//	const nox::reflection::Type& type,
		//	const nox::reflection::VariableInfo::SetterMemberFunc setter_member_func = nullptr,
		//	const nox::reflection::VariableInfo::GetterMemberFunc getter_member_func = nullptr,
		//	const nox::reflection::VariableInfo::GetterMemberFunc getter_address_member_func = nullptr,
		//	const nox::reflection::VariableInfo::SetterArrayMemberFunc setter_array_member_func = nullptr,
		//	const nox::reflection::VariableInfo::GetterArrayMemberFunc getter_array_member_func = nullptr,
		//	const nox::reflection::VariableInfo::GetterArrayMemberFunc getter_array_address_member_func = nullptr
		//)noexcept
		//{
		//	return nox::reflection::VariableInfo(
		//		name,
		//		fullname,
		//		_namespace,
		//		access_level,
		//		attribute_list,
		//		attribute_list_length,
		//		FieldAttributeFlag::Member,
		//		type,
		//		Typeof< _OwnerType>(),
		//		setter_member_func,
		//		getter_member_func,
		//		getter_address_member_func,
		//		setter_array_member_func,
		//		getter_array_member_func,
		//		getter_array_address_member_func
		//	);
		//}

		/*inline constexpr	VariableInfo	CreateFieldInfo(
			ReflectionStringView name,
			ReflectionStringView fullname,
			ReflectionStringView _namespace,
			AccessLevel access_level,
			const class ReflectionObject* const* attribute_ptr_table,
			std::uint8_t	attribute_length,
			const Type& type,
			const VariableInfo::SetterGlobalFunc setter_member_func = nullptr,
			const VariableInfo::GetterGlobalFunc getter_member_func = nullptr,
			const VariableInfo::SetterArrayGlobalFunc setter_array_member_func = nullptr,
			const VariableInfo::GetterArrayGlobalFunc getter_array_member_func = nullptr
		)noexcept
		{
			return VariableInfo(
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
