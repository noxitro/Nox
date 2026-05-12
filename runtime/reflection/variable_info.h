///	@file	field_info.h
///	@brief	field_info
#pragma once

namespace nox::reflection
{
	//	前方宣言
	class ClassInfo;
	class ReflectionObject;

	namespace detail
	{
		template<class>
		class VariableInfoImpl;
		/*struct TypeErasedPtr
		{
			void* ptr;
			const nox::reflection::Type& type;
		};*/
	}

	/// @brief フィールド情報
	class VariableInfo
	{
	public:
#pragma region 変数アクセスの型定義

		using SetterMemberFunc = void(*)(void* instance, void* value);
		template<class R>
		using GetterMemberFunc = nox::reflection::ReflectionOptional<R>(*)(void* instance);
		using GetterAddressMemberFunc = void*(*)(void* instance);

		using SetterSubscriptOperatorMemberFunc = bool(*)(void* instance, void* valuePtr, const std::uint32_t index);
		template<class R>
		using GetterSubscriptOperatorMemberFunc = nox::reflection::ReflectionOptional<R>(*)(void* instance, const std::uint32_t index);
		using GetterAddressSubscriptOperatorMemberFunc = void*(*)(void* instance, const std::uint32_t index);

		using SetterGlobalFunc = void(*)(void* value);
		template<class R>
		using GetterGlobalFunc = nox::reflection::ReflectionOptional<R>(*)();
		using GetterAddressGlobalFunc = void*(*)();

		using SetterSubscriptOperatorGlobalFunc = bool(*)(void* value, const std::uint32_t index);
		template<class R>
		using GetterSubscriptOperatorGlobalFunc = nox::reflection::ReflectionOptional<R>(*)(const std::uint32_t index);
		using GetterAddressSubscriptOperatorGlobalFunc = void*(*)(const std::uint32_t index);

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
			const std::u8string_view name,
			const std::u8string_view fullname,
			const std::u8string_view _namespace,
			nox::reflection::AccessLevel access_level,
			const std::int32_t bit_width,
			const std::int32_t field_offset,
			const std::reference_wrapper<const class nox::reflection::ReflectionObject>* attribute_list,
			const std::uint8_t	attribute_list_length,
			const nox::ObjectPointerId& object_id,
			const nox::reflection::VariableAttributeFlag field_attribute_flgas,
			const nox::reflection::Type& type,
			const nox::reflection::Type& owner_class_type,
			const SetterMemberFunc setter_member_func = nullptr,
			const GetterAddressMemberFunc getter_address_member_func = nullptr,
			const SetterSubscriptOperatorMemberFunc setter_array_member_func = nullptr,
			const GetterAddressSubscriptOperatorMemberFunc getter_array_address_member_func = nullptr
		)noexcept :
			name_(name),
			fullname_(fullname),
			namespace_(_namespace),
			access_level_(access_level),
			bit_width_(bit_width),
			field_offset_(field_offset),
			attribute_list_(attribute_list),
			attribute_list_length_(attribute_list_length),
			object_id_(object_id),
			field_attribute_flgas_(field_attribute_flgas),
			type_(type),
			containing_type_(owner_class_type),
			setter_member_func_(setter_member_func),
			getter_address_member_func_(getter_address_member_func),
			setter_array_member_func_(setter_array_member_func),
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
			std::u8string_view name,
			std::u8string_view fullname,
			std::u8string_view _namespace,
			nox::reflection::AccessLevel access_level,
			const std::int32_t bit_width,
			const std::int32_t field_offset,
			const std::reference_wrapper<const class nox::reflection::ReflectionObject>* attribute_list,
			std::uint8_t	attribute_list_length,
			const nox::ObjectPointerId& object_id,
			nox::reflection::VariableAttributeFlag field_attribute_flgas,
			const nox::reflection::Type& type,
			const nox::reflection::Type& owner_class_type,
			const SetterGlobalFunc setter_global_func = nullptr,
			const GetterAddressGlobalFunc getter_address_global_func = nullptr,
			const SetterSubscriptOperatorGlobalFunc setter_array_global_func = nullptr,
			const GetterAddressSubscriptOperatorGlobalFunc getter_array_address_global_func = nullptr
		)noexcept :
			name_(name),
			fullname_(fullname),
			namespace_(_namespace),
			access_level_(access_level),
			bit_width_(bit_width),
			field_offset_(field_offset),
			attribute_list_(attribute_list),
			attribute_list_length_(attribute_list_length),
			object_id_(object_id),
			field_attribute_flgas_(field_attribute_flgas),
			type_(type),
			containing_type_(owner_class_type),
			setter_global_func_(setter_global_func),
			getter_address_global_func_(getter_address_global_func),
			setter_array_global_func_(setter_array_global_func),
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
		const class nox::reflection::ClassInfo* GetContainingUserDefinedCompoundTypeInfo()const noexcept;

		inline	constexpr	const nox::reflection::Type& GetType()const noexcept { return type_; }
		inline	constexpr	nox::reflection::AccessLevel	GetAccessLevel()const noexcept { return access_level_; }
		inline	constexpr	std::int32_t GetFieldOffsetBits()const noexcept { return field_offset_; }
		inline	constexpr	std::int32_t GetBitWidth()const noexcept { return bit_width_; }

		inline	constexpr	const nox::ObjectPointerId& GetObjectPointerId()const noexcept { return object_id_; }

		/// @brief 属性リストを取得
		inline	constexpr	std::span<const std::reference_wrapper< const class nox::reflection::ReflectionObject>>	GetAttributeList()const noexcept { return std::span(attribute_list_, attribute_list_length_); }
		inline	constexpr	std::uint8_t	GetAttributeListLength()const noexcept { return attribute_list_length_; }
		inline	constexpr	const class nox::reflection::ReflectionObject& GetAttribute(const std::uint8_t index)const noexcept { return util::At(attribute_list_, attribute_list_length_, index); }
		const class nox::reflection::ReflectionObject* GetAttribute(const nox::reflection::Type& type)const noexcept;
		template<class T>
		inline	constexpr	const class nox::reflection::ReflectionObject* GetAttribute()const noexcept
		{
			return this->GetAttribute(nox::reflection::Typeof<T>());
		}

		inline constexpr	bool	IsFieldAttributeFlag(const VariableAttributeFlag flag)const noexcept { return util::IsBitAnd(field_attribute_flgas_, flag); }

		/// @brief メンバ変数か
		inline	constexpr	bool	IsStatic()const noexcept { return IsFieldAttributeFlag(VariableAttributeFlag::Static); }

		/// @brief 読み取り専用
		inline	constexpr	bool	IsReadOnly()const noexcept { return type_.IsConstQualified(); }
#pragma endregion

#pragma region 変数の設定
		template<class TInstanceType, class TValueType>
		inline constexpr bool TrySetValue(TInstanceType&& instance, TValueType&& value)const
		{
			return TrySetValueMemberImpl(
				const_cast<void*>(static_cast<const void*>(&instance)),
				nox::reflection::Typeof<TInstanceType>(),
				const_cast<void*>(static_cast<const void*>(&value)),
				nox::reflection::Typeof<TValueType>()
			);
		}

		template<class TValueType>
		inline constexpr bool TrySetValue(TValueType&& value)const
		{
			return TrySetValueGlobalImpl(
				const_cast<void*>(static_cast<const void*>(&value)),
				nox::reflection::Typeof<TValueType>()
			);
		}

		template<class TInstanceType, class TValueType>
		inline constexpr void SetValue(TInstanceType&& instance, TValueType&& value)const noexcept(false)
		{
			bool success = TrySetValue<TInstanceType, TValueType>(std::forward<TInstanceType>(instance), std::forward<TValueType>(value));
			NOX_ASSERT(success, u"変数の設定に失敗しました");
		}

#pragma endregion


#pragma region 変数の取得
		template<class R>
		inline constexpr R GetValue()const
		{
			std::optional<R> result = this->TryGetValue<R>();
			NOX_ASSERT(result.has_value(), u"変数の取得に失敗しました");
			return result.value();
		}

		template<class R, class _InstanceType>
		inline constexpr R GetValue(_InstanceType&& owner_instance)const
		{
			std::optional<R> result = this->TryGetValue<R>(std::forward<_InstanceType>(owner_instance));
			NOX_ASSERT(result.has_value(), u"変数の取得に失敗しました");
			return result.value();
		}

		/// @brief メンバ変数を取得
		/// @tparam _ResultType 
		/// @tparam _InstanceType 
		/// @param out_value 
		/// @param owner_instance 
		/// @return 
		template<class R, class _InstanceType>
		inline	constexpr	nox::reflection::ReflectionOptional<R>	TryGetValue(_InstanceType&& owner_instance)const
		{
			if (IsStatic() == true)
			{
				return std::nullopt;
			}

			if (type_.IsConvertible(nox::reflection::Typeof<R>()) == false)
			{
				return std::nullopt;
			}

			if (this->containing_type_.IsConvertible(nox::reflection::Typeof<_InstanceType>()) == false)
			{
				return std::nullopt;
			}

			return InvokeImpl<R>([]<class ImplR>(const nox::reflection::VariableInfo& self, nox::not_null<void*> instance_ptr)
				-> nox::reflection::ReflectionOptional<ImplR>
			{
				return static_cast<const nox::reflection::detail::VariableInfoImpl<ImplR>&>(self).GetValueImpl(instance_ptr);
			},
				const_cast<void*>(static_cast<const void*>(&owner_instance))
			);
		}

		template<class R>
		inline	constexpr	nox::reflection::ReflectionOptional<R>	TryGetValue()const
		{
			if (IsStatic() == false)
			{
				return std::nullopt;
			}

			if (type_.IsConvertible(nox::reflection::Typeof<R>()) == false)
			{
				return std::nullopt;
			}

			return InvokeImpl<R>([]<class ImplR>(const nox::reflection::VariableInfo& self)
				-> nox::reflection::ReflectionOptional<ImplR>
			{
				return static_cast<const nox::reflection::detail::VariableInfoImpl<ImplR>&>(self).GetValueImpl();
			});
		}

		template<class _InstanceType>
		inline void* TryGetValueAddress(_InstanceType&& owner_instance)const
		{
			if (IsStatic() == false)
			{
				return nullptr;
			}

			if (type_.IsConvertible(nox::reflection::Typeof<std::remove_const_t<void*>>()) == false)
			{
				return nullptr;
			}

			if (this->containing_type_.IsConvertible(nox::reflection::Typeof<_InstanceType>()) == false)
			{
				return nullptr;
			}

			if (getter_address_member_func_ == nullptr)
			{
				return nullptr;
			}

			return getter_address_member_func_(const_cast<void*>(static_cast<const void*>(&owner_instance)));
		}
#pragma endregion
	private:
		template<class R, class F, class... Args>
		inline	constexpr	nox::reflection::ReflectionOptional<R> InvokeImpl(F&& f, Args&&... args)const
		{
			if (type_ == nox::reflection::Typeof<R>())
			{
				return f.template operator() < R > (*this, std::forward<Args>(args)...);
			}
			else
			{
				return std::nullopt;
			}
		}
#pragma region 呼び出しチェック関数
		/// @brief メンバ関数アクセス時のチェック
		/// @return 
		inline	constexpr	bool	CheckMemberParams(const Type& out_type, const Type& owner_class_type)const noexcept
		{
			if (IsStatic() == true)
			{
				return false;
			}

			if (type_.IsConvertible(out_type) == false)
			{
				return false;
			}

			if (owner_class_type.GetRemoveAllModifiersType() != containing_type_)
			{
				return false;
			}

			return true;
		}

		inline constexpr	bool	CheckParams(const Type& return_type)const noexcept
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

		inline constexpr bool CheckSubscriptParams()
		{
			return false;
		}
#pragma endregion

#pragma region 変数設定の内部実装
		inline constexpr bool TrySetValueMemberImpl(void* instance, const Type& owner_class_type, void* value, const Type& value_type)const
		{
			if (setter_member_func_ == nullptr)
			{
				return false;
			}

			if (IsStatic() == true)
			{
				return false;
			}

			if (owner_class_type.GetRemoveAllModifiersType() != containing_type_)
			{
				return false;
			}

			// const メンバは書き込み禁止
			if (type_.IsConstQualified())
			{
				return false;
			}

			if (type_.IsConvertible(value_type) == false)
			{
				return false;
			}

			std::invoke(setter_member_func_, instance, value);

			return true;
		}

		inline constexpr bool TrySetValueGlobalImpl(void* value, const Type& value_type)const
		{
			if (setter_global_func_ == nullptr)
			{
				return false;
			}

			if (IsStatic() == false)
			{
				return false;
			}

			// const グローバルは書き込み禁止
			if (type_.IsConstQualified())
			{
				return false;
			}

			if (value_type.IsConvertible(type_) == false)
			{
				return false;
			}

			std::invoke(setter_global_func_, const_cast<void*>(value));
			return true;
		}
#pragma endregion

#pragma region 変数の取得の内部実装
#pragma endregion
	private:
		/// @brief 属性テーブルの長さ
		const std::uint8_t attribute_list_length_;

		/// @brief 値の属性
		const nox::reflection::VariableAttributeFlag field_attribute_flgas_;

		/// @brief アクセスレベル
		const nox::reflection::AccessLevel access_level_;

		const std::int32_t field_offset_;
		const std::int32_t bit_width_;

		const nox::ObjectPointerId& object_id_;

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

		/// @brief getter address_
		union
		{
			const GetterAddressMemberFunc getter_address_member_func_;
			const GetterAddressGlobalFunc getter_address_global_func_;
		};

		/// @brief 配列 setter
		union
		{
			const SetterSubscriptOperatorMemberFunc setter_array_member_func_;
			const SetterSubscriptOperatorGlobalFunc setter_array_global_func_;
		};

		
		
		/// @brief 配列 getter address_
		union
		{
			const GetterAddressSubscriptOperatorMemberFunc getter_array_address_member_func_;
			const GetterAddressSubscriptOperatorGlobalFunc getter_array_address_global_func_;
		};

		/// @brief 名前
		const std::u8string_view name_;

		/// @brief 完全な名前
		const std::u8string_view fullname_;

		/// @brief 名前空間
		const std::u8string_view namespace_;

	};

	namespace detail
	{
		template<class R>
		struct VariableInfoImplMemberDesc
		{
			const std::u8string_view name;
			const std::u8string_view fullname;
			const std::u8string_view namespace_str;
			nox::reflection::AccessLevel access_level;
			const nox::ObjectPointerId& object_id;
			const std::int32_t bit_width;
			const std::int32_t field_offset;
			const std::reference_wrapper<const class nox::reflection::ReflectionObject>* attribute_list;
			const std::uint8_t	attribute_list_length;
			nox::reflection::VariableAttributeFlag field_attribute_flgas;
			const nox::reflection::Type& type;
			const nox::reflection::Type& owner_class_type;

			const VariableInfo::SetterMemberFunc setter_member_func;
			const VariableInfo::GetterAddressMemberFunc getter_address_member_func;
			const VariableInfo::SetterSubscriptOperatorMemberFunc setter_array_member_func;
			const VariableInfo::GetterAddressSubscriptOperatorMemberFunc getter_array_address_member_func;
			const VariableInfo::GetterMemberFunc<R> getter_member_func;
			const VariableInfo::GetterSubscriptOperatorMemberFunc<R> getter_array_member_func;
		};

		template<class R>
		struct VariableInfoImplGlobalDesc
		{
			const std::u8string_view name;
			const std::u8string_view fullname;
			const std::u8string_view namespace_str;
			nox::reflection::AccessLevel access_level;
			const nox::ObjectPointerId& object_id;
			const std::int32_t bit_width;
			const std::int32_t field_offset;
			const std::reference_wrapper<const class nox::reflection::ReflectionObject>* attribute_list;
			const std::uint8_t	attribute_list_length;
			nox::reflection::VariableAttributeFlag field_attribute_flgas;
			const nox::reflection::Type& type;
			const nox::reflection::Type& owner_class_type;

			const VariableInfo::SetterGlobalFunc setter_global_func;
			const VariableInfo::GetterAddressGlobalFunc getter_address_global_func;
			const VariableInfo::SetterSubscriptOperatorGlobalFunc setter_array_global_func;
			const VariableInfo::GetterAddressSubscriptOperatorGlobalFunc getter_array_address_global_func;
			const VariableInfo::GetterGlobalFunc<R> getter_global_func;
			const VariableInfo::GetterSubscriptOperatorGlobalFunc<R> getter_array_global_func;
		};

		/// @brief 変数情報
		/// @tparam T オブジェクトポインタ型
		template<class ResultType>
		class VariableInfoImpl final: public VariableInfo
		{
		public:
			inline constexpr explicit VariableInfoImpl(
				const nox::reflection::detail::VariableInfoImplMemberDesc<ResultType>& desc
			)noexcept:
				VariableInfo(
					desc.name,
					desc.fullname,
					desc.namespace_str,
					desc.access_level,
					desc.bit_width,
					desc.field_offset,
					desc.attribute_list,
					desc.attribute_list_length,
					desc.object_id,
					desc.field_attribute_flgas,
					desc.type,
					desc.owner_class_type,
					desc.setter_member_func,
					desc.getter_address_member_func,
					desc.setter_array_member_func,
					desc.getter_array_address_member_func
				),
				getter_member_func_(desc.getter_member_func),
				getter_array_member_func_(desc.getter_array_member_func)
			{}

			inline constexpr explicit VariableInfoImpl(
				const nox::reflection::detail::VariableInfoImplGlobalDesc<ResultType>& desc
			)noexcept:
				VariableInfo(
					desc.name,
					desc.fullname,
					desc.namespace_str,
					desc.access_level,
					desc.bit_width,
					desc.field_offset,
					desc.attribute_list,
					desc.attribute_list_length,
					desc.object_id,
					desc.field_attribute_flgas,
					desc.type,
					desc.owner_class_type,
					desc.setter_global_func,
					desc.getter_address_global_func,
					desc.setter_array_global_func,
					desc.getter_array_address_global_func
				),
				getter_global_func_(desc.getter_global_func),
				getter_array_global_func_(desc.getter_array_global_func)
			{}

			inline constexpr nox::reflection::ReflectionOptional<ResultType> GetValueImpl(nox::not_null<void*> args)const
			{
				if (getter_member_func_ == nullptr)
				{
					return std::nullopt;
				}
				return std::invoke(getter_member_func_, args.get());
			}

			inline constexpr nox::reflection::ReflectionOptional<ResultType> GetValueImpl()const
			{
				if (getter_global_func_ == nullptr)
				{
					return std::nullopt;
				}
				return std::invoke(getter_global_func_);
			}

			inline constexpr nox::reflection::ReflectionOptional<ResultType> GetArrayValueImpl(void* const args, const std::uint32_t index)const
			{
				if (getter_array_member_func_ == nullptr)
				{
					return std::nullopt;
				}
				return std::invoke(getter_array_member_func_, args, index);
			}

			inline constexpr nox::reflection::ReflectionOptional<ResultType> GetArrayValueImpl(const std::uint32_t index)const
			{
				if (getter_array_global_func_ == nullptr)
				{
					return std::nullopt;
				}
				return std::invoke(getter_array_global_func_, index);
			}
		private:
			union
			{
				const GetterMemberFunc<ResultType> getter_member_func_;
				const GetterGlobalFunc<ResultType> getter_global_func_;
			};

			/// @brief 配列 getter
			union
			{
				const GetterSubscriptOperatorMemberFunc<ResultType> getter_array_member_func_;
				const GetterSubscriptOperatorGlobalFunc<ResultType> getter_array_global_func_;
			};
		};

		/// @brief		参照メンバ変数情報
		template<class R>
		inline constexpr nox::reflection::detail::VariableInfoImpl<R> CreateVariableInfoMemberRef(
			const nox::reflection::Type& type,
			const nox::reflection::Type& owner_type,
			std::u8string_view name,
			std::u8string_view fullname,
			std::u8string_view _namespace,
			nox::reflection::AccessLevel access_level,
			const std::int32_t bit_width,
			const std::int32_t field_offset,
			const std::reference_wrapper<const class nox::reflection::ReflectionObject>* attribute_list,
			const std::uint8_t	attribute_list_length,
			const nox::reflection::VariableAttributeFlag additinal_flags,
			const VariableInfo::SetterMemberFunc setter_member_func = nullptr,
			const VariableInfo::GetterMemberFunc<R> getter_member_func = nullptr,
			const VariableInfo::GetterAddressMemberFunc getter_address_member_func = nullptr,
			const VariableInfo::SetterSubscriptOperatorMemberFunc setter_array_member_func = nullptr,
			const VariableInfo::GetterSubscriptOperatorMemberFunc<R> getter_array_member_func = nullptr,
			const VariableInfo::GetterAddressSubscriptOperatorMemberFunc getter_array_address_member_func = nullptr)noexcept
		{
			const auto desc = nox::reflection::detail::VariableInfoImplMemberDesc<R>{
				.name = name,
				.fullname = fullname,
				.namespace_str = _namespace,
				.access_level = access_level,
				.object_id = nox::GetInvalidObjectPointerId(),
				.bit_width = bit_width,
				.field_offset = field_offset,
				.attribute_list = attribute_list,
				.attribute_list_length = attribute_list_length,
				.field_attribute_flgas = additinal_flags,
				.type = type,
				.owner_class_type = owner_type,
				.setter_member_func = setter_member_func,
				.getter_address_member_func = getter_address_member_func,
				.setter_array_member_func = setter_array_member_func,
				.getter_array_address_member_func = getter_array_address_member_func,
				.getter_member_func = getter_member_func,
				.getter_array_member_func = getter_array_member_func
			};

			return nox::reflection::detail::VariableInfoImpl<R>(desc);
		}
		
		template<class R>
		inline constexpr nox::reflection::detail::VariableInfoImpl<R> CreateVariableInfoGlobalRef(
			const nox::reflection::Type& type,
			const std::u8string_view name,
			const std::u8string_view fullname,
			const std::u8string_view _namespace,
			const nox::reflection::AccessLevel access_level,
			const std::int32_t bit_width,
			const std::int32_t field_offset,
			const std::reference_wrapper<const class nox::reflection::ReflectionObject>* attribute_list,
			const std::uint8_t	attribute_list_length,
			const VariableAttributeFlag additinal_flags,
			const VariableInfo::SetterGlobalFunc setter_global_func = nullptr,
			const VariableInfo::GetterGlobalFunc<R> getter_global_func = nullptr,
			const VariableInfo::GetterAddressGlobalFunc getter_address_global_func = nullptr,
			const VariableInfo::SetterSubscriptOperatorGlobalFunc setter_array_global_func = nullptr,
			const VariableInfo::GetterSubscriptOperatorGlobalFunc<R> getter_array_global_func = nullptr,
			const VariableInfo::GetterAddressSubscriptOperatorGlobalFunc getter_array_address_global_func = nullptr)noexcept
		{
			const auto desc = nox::reflection::detail::VariableInfoImplGlobalDesc<R>{
				.name = name,
				.fullname = fullname,
				.namespace_str = _namespace,
				.access_level = access_level,
				.object_id = nox::GetInvalidObjectPointerId(),
				.bit_width = bit_width,
				.field_offset = field_offset,
				.attribute_list = attribute_list,
				.attribute_list_length = attribute_list_length,
				.field_attribute_flgas = additinal_flags,
				.type = type,
				.owner_class_type = nox::reflection::GetInvalidType(),
				.setter_global_func = setter_global_func,
				.getter_address_global_func = getter_address_global_func,
				.setter_array_global_func = setter_array_global_func,
				.getter_array_address_global_func = getter_array_address_global_func,
				.getter_global_func = getter_global_func,
				.getter_array_global_func = getter_array_global_func
			};

			return nox::reflection::detail::VariableInfoImpl(desc);
		}

		template<class R, auto object_pointer>
		inline constexpr nox::reflection::detail::VariableInfoImpl<R> CreateVariableInfoMember(
			const nox::ObjectPointerId& object_pointer_id,
			std::u8string_view name,
			std::u8string_view fullname,
			std::u8string_view _namespace,
			nox::reflection::AccessLevel access_level,
			const std::int32_t bit_width,
			const std::int32_t field_offset,
			const std::reference_wrapper<const class nox::reflection::ReflectionObject>* attribute_list,
			std::uint8_t	attribute_list_length,
			const VariableAttributeFlag additinal_flags,
			const VariableInfo::SetterMemberFunc setter_member_func = nullptr,
			const VariableInfo::GetterMemberFunc<R> getter_member_func = nullptr,
			const VariableInfo::GetterAddressMemberFunc getter_address_member_func = nullptr,
			const VariableInfo::SetterSubscriptOperatorMemberFunc setter_array_member_func = nullptr,
			const VariableInfo::GetterSubscriptOperatorMemberFunc<R> getter_array_member_func = nullptr,
			const VariableInfo::GetterAddressSubscriptOperatorMemberFunc getter_array_address_member_func = nullptr)noexcept
		{
			const nox::reflection::VariableAttributeFlag field_attribute_flgas =
				nox::util::BitOr(nox::reflection::GetFieldAttributeFlags<decltype(object_pointer)>(), additinal_flags);
		
			const auto desc = nox::reflection::detail::VariableInfoImplMemberDesc<R>{
				.name = name,
				.fullname = fullname,
				.namespace_str = _namespace,
				.access_level = access_level,
				.object_id = object_pointer_id,
				.bit_width = bit_width,
				.field_offset = field_offset,
				.attribute_list = attribute_list,
				.attribute_list_length = attribute_list_length,
				.field_attribute_flgas = field_attribute_flgas,
				.type = nox::reflection::Typeof<R>(),
				.owner_class_type = nox::reflection::Typeof<nox::MemberObjectPointerClassType<decltype(object_pointer)>>(),
				.setter_member_func = setter_member_func,
				.getter_address_member_func = getter_address_member_func,
				.setter_array_member_func = setter_array_member_func,
				.getter_array_address_member_func = getter_array_address_member_func,
				.getter_member_func = getter_member_func,
				.getter_array_member_func = getter_array_member_func
			};

			return nox::reflection::detail::VariableInfoImpl<R>(desc);
		}

		template<class R, auto object_pointer>
		inline constexpr nox::reflection::detail::VariableInfoImpl<R> CreateVariableInfoGlobal(
			const nox::ObjectPointerId& object_pointer_id,
			const std::u8string_view name,
			const std::u8string_view fullname,
			const std::u8string_view _namespace,
			const nox::reflection::AccessLevel access_level,
			const std::int32_t bit_width = -1,
			const std::int32_t field_offset = -1,
			const std::reference_wrapper<const class nox::reflection::ReflectionObject>* attribute_list = nullptr,
			const std::uint8_t	attribute_list_length = 0,
			const VariableAttributeFlag additional_attribute_flags = VariableAttributeFlag::None,
			const VariableInfo::SetterGlobalFunc setter_global_func = nullptr,
			const VariableInfo::GetterGlobalFunc<R> getter_global_func = nullptr,
			const VariableInfo::GetterAddressGlobalFunc getter_address_global_func = nullptr,
			const VariableInfo::SetterSubscriptOperatorGlobalFunc setter_array_global_func = nullptr,
			const VariableInfo::GetterSubscriptOperatorGlobalFunc<R> getter_array_global_func = nullptr,
			const VariableInfo::GetterAddressSubscriptOperatorGlobalFunc getter_array_address_global_func = nullptr)noexcept
		{
			const nox::reflection::VariableAttributeFlag field_attribute_flgas =
				nox::util::BitOr(nox::reflection::GetFieldAttributeFlags<decltype(object_pointer)>(), additional_attribute_flags);

			const auto desc = nox::reflection::detail::VariableInfoImplGlobalDesc<R>{
				.name = name,
				.fullname = fullname,
				.namespace_str = _namespace,
				.access_level = access_level,
				.object_id = object_pointer_id,
				.bit_width = bit_width,
				.field_offset = field_offset,
				.attribute_list = attribute_list,
				.attribute_list_length = attribute_list_length,
				.field_attribute_flgas = field_attribute_flgas,
				.type = nox::reflection::Typeof<R>(),
				.owner_class_type = nox::reflection::GetInvalidType(),
				.setter_global_func = setter_global_func,
				.getter_address_global_func = getter_address_global_func,
				.setter_array_global_func = setter_array_global_func,
				.getter_array_address_global_func = getter_array_address_global_func,
				.getter_global_func = getter_global_func,
				.getter_array_global_func = getter_array_global_func
			};

			return nox::reflection::detail::VariableInfoImpl<R>(desc);
		}

	}
}

