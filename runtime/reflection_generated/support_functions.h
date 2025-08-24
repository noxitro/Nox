//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file		support_functions.h
///	@brief		リフレクションジェネレータ用サポート関数群
/// @details	外部には公開せず、generator内でのみ使用する
#pragma once

#pragma region マクロ関連
#pragma region メンバ関数
///	@brief	メンバ変数へのセット
#define	NOX_VARIABLE_INFO_LAMBDA_SETTER(ClassName, FieldName) +[](nox::not_null<void*> instance_ptr, const void* const value){\
	static_cast<ClassName*>(instance_ptr.get())->FieldName = *static_cast<nox::AddConstPointerType<std::add_pointer_t<decltype(ClassName::FieldName)>>>(value);\
	}

///	@brief	メンバ変数の取得
#define	NOX_VARIABLE_INFO_LAMBDA_GETTER(ClassName, FieldName) +[](nox::not_null<void*> outValue, nox::not_null<const void*> instance_ptr) {\
	*static_cast<nox::RemoveConstPointerReferenceType<std::add_pointer_t<decltype(ClassName::FieldName)>>>(outValue.get()) = static_cast<const ClassName*>(instance_ptr.get())->FieldName;\
}

///	@brief	メンバ変数アドレスの取得
#define	NOX_VARIABLE_INFO_LAMBDA_GETTER_ADDRESS(ClassName, FieldName) +[](nox::not_null<void*> outValue, nox::not_null<const void*> instance_ptr) {\
	*static_cast<nox::RemoveConstPointerReferenceType<std::add_pointer_t<decltype(ClassName::FieldName)>>>(outValue.get()) = static_cast<const ClassName*>(instance_ptr.get())->FieldName;\
}

/// @brief	メンバ変数へのセット（配列用）
#define	NOX_VARIABLE_INFO_LAMBDA_SETTER_ARRAY(ClassName, FieldName) +[](nox::not_null<void*> instance_ptr, const void* const value, const u32 index) {\
	if (nox::util::IsValidIndex(static_cast<const ClassName*>(instance_ptr.get())->FieldName, index) == false) {\
		return false;\
	}\
	static_cast<ClassName*>(instance_ptr.get())->FieldName[index] = *static_cast<nox::AddConstPointerType<std::add_pointer_t<nox::ContainerElementType<decltype(ClassName::FieldName)>>>>(value);\
	return true;\
	}

/// @brief	メンバ変数の取得（配列用）
#define	NOX_VARIABLE_INFO_LAMBDA_GETTER_ARRAY(ClassName, FieldName) +[](nox::not_null<void*> outValue, nox::not_null<const void*> instance_ptr, const u32 index) {\
	if (nox::util::IsValidIndex(static_cast<const ClassName*>(instance_ptr.get())->FieldName, index) == false) {\
		return false;\
	}\
	*static_cast<RemoveConstPointerReferenceType<std::add_pointer_t<nox::ContainerElementType<decltype(ClassName::FieldName)>>>>(outValue.get()) = static_cast<const ClassName*>(instance_ptr.get())->FieldName[index];\
	return true;\
	}

#define NOX_VARIABLE_INFO_LAMBDA_ADDRESS_GETTER(ClassName, FieldName) +[](nox::not_null<void*> outValue, nox::not_null<const void*> instance_ptr) {\
	*static_cast<std::add_pointer_t<nox::AddConstPointerType<std::add_pointer_t<decltype(ClassName::FieldName)>>>>(outValue.get()) = &static_cast<const ClassName*>(instance_ptr.get())->FieldName;\
}


	//	グローバル版
#define	NOX_VARIABLE_INFO_LAMBDA_SETTER_GLOBAL(FieldName) +[]( const void* const value){\
	FieldName = *static_cast<nox::AddConstPointerType<std::add_pointer_t<decltype(FieldName)>>>(value);\
	}

#define	NOX_VARIABLE_INFO_LAMBDA_GETTER_GLOBAL(FieldName) +[](not_null<void*> outValue) {\
	*static_cast<RemoveConstPointerReferenceType<std::add_pointer_t<decltype(FieldName)>>>(outValue.get()) = FieldName;\
}

#define	NOX_VARIABLE_INFO_LAMBDA_SETTER_ARRAY_GLOBAL(FieldName) +[](const void* const value, const u32 index) {\
	if (nox::util::IsValidIndex(FieldName, index) == false) {\
		return false;\
	}\
	FieldName[index] = *static_cast<nox::AddConstPointerType<std::add_po1inter_t<nox::ContainerElementType<decltype(FieldName)>>>>(value);\
	return true;\
	}

#define	NOX_VARIABLE_INFO_LAMBDA_GETTER_ARRAY_GLOBAL(FieldName) +[](not_null<void*> outValue, const u32 index) {\
	if (nox::util::IsValidIndex(FieldName, index) == false) {\
		return false;\
	}\
	*static_cast<RemoveConstPointerReferenceType<std::add_pointer_t<nox::ContainerElementType<decltype(FieldName)>>>>(outValue.get()) = FieldName[index];\
	return true;\
	}
#pragma endregion

#pragma region 改良版
/// @brief	メンバ変数へのセット（メンバ関数用）
#define NOX_VARIABLE_INFO_SETTER_MEMBER(_ClassType, VariableFullName) +[](nox::not_null<void*> instance, const void* const value) {\
		using VariableType = decltype(VariableFullName);\
		using ClassType = _ClassType;\
		/*代入可能*/ \
		if constexpr (nox::concepts::Assignable<VariableType, VariableType>) {\
			static_cast<ClassType*>(instance.get())->VariableFullName = *static_cast<const std::remove_reference_t<VariableType>*>(value);\
		}\
	}
// end

/// @brief	メンバ変数の取得（メンバ関数用）
#define NOX_VARIABLE_INFO_GETTER_MEMBER(_ClassType, VariableFullName) +[](nox::not_null<void*> out, nox::not_null<const void*> instance) {\
		using VariableType = decltype(VariableFullName);\
		using ClassType = _ClassType;\
		/*変換可能か？=代入可能*/ \
		if constexpr (std::convertible_to<VariableType, std::remove_const_t<VariableType>>) {\
			*static_cast<std::remove_const_t<std::decay_t<VariableType>>*>(out.get()) = static_cast<const ClassType*>(instance.get())->VariableFullName;\
		}\
	}
// end

/// @brief	メンバ変数のアドレスを取得（メンバ関数用）
#define NOX_VARIABLE_INFO_GETTER_ADDRESS_MEMBER(_ClassType, VariableFullName) +[](nox::not_null<void*> out, nox::not_null<void*> instance) {\
		using VariableType = decltype(VariableFullName);\
		using ClassType = _ClassType;\
		/*変換可能か？=代入可能*/ \
		if constexpr (std::convertible_to<VariableType, std::remove_const_t<VariableType>>) {\
			*static_cast<std::remove_reference_t<VariableType>**>(out.get()) = &static_cast<ClassType*>(instance.get())->VariableFullName;\
		}\
	}
// end

/// @brief	メンバ変数へのセット（配列用、メンバ関数用）
#define NOX_VARIABLE_INFO_SETTER_SUBSCRIPT_MEMBER(_ClassType, VariableFullName) +[](nox::not_null<void*> instance, const void* const value, const std::uint32_t index) {\
		using VariableType = decltype(VariableFullName);\
		using ClassType = _ClassType;\
		if constexpr (nox::HasIndexOperatorValue<VariableType> == true)\
		{\
			using ElementType = nox::ContainerElementType<VariableType>;\
			if (nox::util::IsValidIndex(static_cast<ClassType*>(instance.get())->VariableFullName, index) == false) {\
				return;\
			}\
			if constexpr (nox::concepts::Assignable<ElementType, ElementType>)\
			{\
				static_cast<ClassType*>(instance.get())->VariableFullName[index] = *static_cast<ElementType*>(value);\
				return true;\
			}\
		}\
		return false;\
	}
// end

/// @brief	メンバ変数の取得（配列用、メンバ関数用）
#define NOX_VARIABLE_INFO_GETTER_SUBSCRIPT_MEMBER(_ClassType, VariableFullName) +[](nox::not_null<void*> out, nox::not_null<const void*> instance, const std::uint32_t index) {\
		using VariableType = decltype(VariableFullName);\
		using ClassType = _ClassType;\
		if constexpr (nox::HasIndexOperatorValue<VariableType> == true)\
		{\
			using ElementType = nox::ContainerElementType<VariableType>;\
			if (nox::util::IsValidIndex(static_cast<const ClassType*>(instance.get())->VariableFullName, index) == false) \
			{\
				return;\
			}\
			if constexpr (nox::concepts::Assignable<ElementType, ElementType>)\
			{\
				*static_cast<ElementType*>(out.get()) = static_cast<const ClassType*>(instance.get())->VariableFullName[index];\
				return true;\
			}\
		}\
		return false;\
	}
// end

/// @brief	メンバ変数のアドレスを取得（配列用、メンバ関数用）
#define NOX_VARIABLE_INFO_GETTER_ADDRESS_SUBSCRIPT_MEMBER(_ClassType, VariableFullName) +[](nox::not_null<void*> out, nox::not_null<void*> instance, const std::uint32_t index) {\
		using VariableType = decltype(VariableFullName);\
		using ClassType = _ClassType;\
		if constexpr (nox::HasIndexOperatorValue<VariableType> == true)\
		{\
			using ElementType = nox::ContainerElementType<VariableType>;\
			if constexpr (nox::concepts::Assignable<ElementType, ElementType>)\
			{\
				if (nox::util::IsValidIndex(static_cast<const ClassType*>(instance.get())->VariableFullName, index) == true) \
				{\
					*static_cast<std::add_pointer_t<nox::AddConstPointerType<ElementType*>>>(out.get()) = &static_cast<ClassType*>(instance.get())->VariableFullName[index];\
					return true;\
				}\
			}\
		}\
		return false;\
	}
// end

/// @brief	変数へのセット(グローバル変数用)
#define NOX_VARIABLE_INFO_SETTER_GLOBAL(VariableFullName) +[](const void* const value) {\
		using VariableType = decltype(VariableFullName);\
		if constexpr (nox::concepts::Assignable<VariableType, VariableType>) {\
			VariableFullName = *static_cast<const std::remove_reference_t<VariableType>*>(value);\
		}\
	}
// end

/// @brief	変数の取得(グローバル変数用)
#define NOX_VARIABLE_INFO_GETTER_GLOBAL(VariableFullName) +[](nox::not_null<void*> out) {\
		using VariableType = decltype(VariableFullName);\
		if constexpr (std::convertible_to<VariableType, std::remove_const_t<VariableType>>) {\
			*static_cast<std::remove_const_t<std::decay_t<VariableType>>*>(out.get()) = VariableFullName;\
		}\
	}
// end

/// @brief	変数のアドレスを取得(グローバル変数用)
#define NOX_VARIABLE_INFO_GETTER_ADDRESS_GLOBAL(VariableFullName) +[](nox::not_null<void*> out) {\
		using VariableType = decltype(VariableFullName);\
		if constexpr (std::convertible_to<VariableType, std::remove_const_t<VariableType>>) {\
			*static_cast<std::remove_reference_t<VariableType>**>(out.get()) = &VariableFullName;\
		}\
	}
// end

/// @brief	変数へのセット（配列用、グローバル変数用）
#define NOX_VARIABLE_INFO_SETTER_SUBSCRIPT_GLOBAL(VariableFullName) +[](const void* const value, const std::uint32_t index) {\
		using VariableType = decltype(VariableFullName);\
		if constexpr (nox::HasIndexOperatorValue<VariableType> == true)\
		{\
			using ElementType = const nox::ContainerElementType<const VariableType>;\
			if (nox::util::IsValidIndex(VariableFullName, index) == false) {\
				return false;\
			}\
			if constexpr (nox::concepts::Assignable<VariableType, VariableType>)\
			{\
				VariableFullName[index] = *static_cast<std::add_pointer_t<ElementType>>(value);\
			}\
			return true;\
		}\
		return false;\
	}
// end

/// @brief	変数の取得（配列用、グローバル変数用）
#define NOX_VARIABLE_INFO_GETTER_SUBSCRIPT_GLOBAL(VariableFullName) +[](nox::not_null<void*> out, const std::uint32_t index) {\
		using VariableType = decltype(VariableFullName);\
		if constexpr (nox::HasIndexOperatorValue<VariableType> == true)\
		{\
			using ElementType = nox::ContainerElementType<VariableType>;\
			if (nox::util::IsValidIndex(VariableFullName, index) == false)\
			{\
				return false;\
			}\
			if constexpr (std::convertible_to<ElementType, std::decay_t<ElementType>>)\
			{\
				*static_cast<std::decay_t<ElementType>*>(out.get()) = VariableFullName[index];\
				return true;\
			}\
		}\
		return false;\
	}
// end

/// @brief	変数のアドレスを取得（配列用、グローバル変数用）
#define NOX_VARIABLE_INFO_GETTER_ADDRESS_SUBSCRIPT_GLOBAL(VariableFullName) +[](nox::not_null<void*> out, const std::uint32_t index) {\
		using VariableType = decltype(VariableFullName);\
		if constexpr (nox::HasIndexOperatorValue<VariableType> == true)\
		{\
			using ElementType = nox::ContainerElementType<VariableType>;\
			if (nox::util::IsValidIndex(VariableFullName, index) == false) {\
				return false;\
			}\
			*static_cast<std::add_pointer_t<nox::AddConstPointerType<ElementType*>>>(out.get()) = &VariableFullName[index];\
			return true;\
		}\
		return false;\
	}
// end

#pragma endregion

#pragma region ラムダ式作成用ラムダ


#define NOX_VARIABLE_INFO_SETTER_SUBSCRIPT

//	メンバ
#define	NOX_VARIABLE_INFO_CREATE_LAMBDA_SETTER(ClassName, FieldName) +[]()constexpr{\
		using _T = decltype(ClassName::FieldName);\
		if constexpr (std::is_array_v<_T> || std::is_const_v<_T> == true)\
		{\
			return nullptr;\
		}\
		else\
		{\
			if constexpr (std::is_class_v<_T> == false)\
			{\
				return NOX_VARIABLE_INFO_LAMBDA_SETTER(ClassName, FieldName); \
			}\
			else if constexpr (nox::IsInvokableDefaultOperatorValue<_T> == true)\
			{\
				return NOX_VARIABLE_INFO_LAMBDA_SETTER(ClassName, FieldName); \
			}\
			else\
			{\
				return nullptr; \
			}\
		}\
		}()


#define	NOX_VARIABLE_INFO_CREATE_LAMBDA_GETTER(ClassName, FieldName) +[]()constexpr{\
		using _T = decltype(ClassName::FieldName);\
		if constexpr (std::is_array_v<_T>)\
		{\
			return nullptr;\
		}\
		else\
		{\
			if constexpr (std::is_class_v<_T> == false)\
			{\
				return NOX_VARIABLE_INFO_LAMBDA_GETTER(ClassName, FieldName);\
			}\
			else if constexpr (nox::IsInvokableDefaultOperatorValue<_T> == true)\
			{\
				return NOX_VARIABLE_INFO_LAMBDA_GETTER(ClassName, FieldName);\
			}\
			else\
			{\
				return nullptr;\
			}\
		}\
		}()

#define	NOX_VARIABLE_INFO_CREATE_LAMBDA_SETTER_ARRAY(ClassName, FieldName) +[]()constexpr{\
		using _T = decltype(ClassName::FieldName);\
		if constexpr ((std::is_array_v<_T> == false && nox::IsSequenceContainerClassValue<_T> == false) || std::is_const_v<RemoveExtentArraySequenceContainerT<_T>> == true)\
		{\
			return nullptr;\
		}\
		else\
		{\
			if constexpr (std::is_class_v<_T> == false)\
			{\
				return NOX_VARIABLE_INFO_LAMBDA_SETTER_ARRAY(ClassName, FieldName); \
			}\
			else if constexpr (nox::HasIndexOperatorValue<_T> == true)\
			{\
				return NOX_VARIABLE_INFO_LAMBDA_SETTER_ARRAY(ClassName, FieldName); \
			}\
			else\
			{\
				return nullptr; \
			}\
		}\
		}()

#define	NOX_VARIABLE_INFO_CREATE_LAMBDA_GETTER_ARRAY(ClassName, FieldName) +[]()constexpr{\
		using _T = decltype(ClassName::FieldName);\
		if constexpr (std::is_array_v<_T> == false && nox::IsSequenceContainerClassValue<_T> == false)\
		{\
			return nullptr;\
		}\
		else\
		{\
			if constexpr (std::is_class_v<_T> == false)\
			{\
				return NOX_VARIABLE_INFO_LAMBDA_GETTER_ARRAY(ClassName, FieldName); \
			}\
			else if constexpr (nox::HasIndexOperatorValue<_T> == true)\
			{\
				return NOX_VARIABLE_INFO_LAMBDA_GETTER_ARRAY(ClassName, FieldName); \
			}\
			else\
			{\
				return nullptr; \
			}\
		}\
		}()

//	global
#define	NOX_VARIABLE_INFO_CREATE_LAMBDA_SETTER_GLOBAL(FieldName) +[]()constexpr{\
		using _T = decltype(FieldName);\
		if constexpr (std::is_array_v<_T> || std::is_const_v<_T> == true)\
		{\
			return nullptr;\
		}\
		else\
		{\
			if constexpr (std::is_class_v<_T> == false)\
			{\
				return NOX_VARIABLE_INFO_LAMBDA_SETTER_GLOBAL(FieldName); \
			}\
			else if constexpr (nox::IsInvokableDefaultOperatorValue<_T> == true)\
			{\
				return NOX_VARIABLE_INFO_LAMBDA_SETTER_GLOBAL(FieldName); \
			}\
			else\
			{\
				return nullptr; \
			}\
		}\
		}()


#define	NOX_VARIABLE_INFO_CREATE_LAMBDA_GETTER_GLOBAL(FieldName) +[]()constexpr{\
		using _T = decltype(FieldName);\
		if constexpr (std::is_array_v<_T>)\
		{\
			return nullptr;\
		}\
		else\
		{\
			if constexpr (std::is_class_v<_T> == false)\
			{\
				return NOX_VARIABLE_INFO_LAMBDA_GETTER_GLOBAL(FieldName);\
			}\
			else if constexpr (nox::IsInvokableDefaultOperatorValue<_T> == true)\
			{\
				return NOX_VARIABLE_INFO_LAMBDA_GETTER_GLOBAL(FieldName);\
			}\
			else\
			{\
				return nullptr;\
			}\
		}\
		}()

#define	NOX_VARIABLE_INFO_CREATE_LAMBDA_SETTER_ARRAY_GLOBAL(FieldName) +[]()constexpr{\
		using _T = decltype(FieldName);\
		if constexpr ((std::is_array_v<_T> == false && nox::IsSequenceContainerClassValue<_T> == false) || std::is_const_v<RemoveExtentArraySequenceContainerT<_T>> == true)\
		{\
			return nullptr;\
		}\
		else\
		{\
			if constexpr (std::is_class_v<_T> == false)\
			{\
				return NOX_VARIABLE_INFO_LAMBDA_SETTER_ARRAY_GLOBAL(FieldName); \
			}\
			else if constexpr (nox::HasIndexOperatorValue<_T> == true)\
			{\
				return NOX_VARIABLE_INFO_LAMBDA_SETTER_ARRAY_GLOBAL(FieldName); \
			}\
			else\
			{\
				return nullptr; \
			}\
		}\
		}()

#define	NOX_VARIABLE_INFO_CREATE_LAMBDA_GETTER_ARRAY_GLOBAL(FieldName) +[]()constexpr{\
		using _T = decltype(FieldName);\
		if constexpr (std::is_array_v<_T> == false && nox::IsSequenceContainerClassValue<_T> == false)\
		{\
			return nullptr;\
		}\
		else\
		{\
			if constexpr (std::is_class_v<_T> == false)\
			{\
				return NOX_VARIABLE_INFO_LAMBDA_GETTER_ARRAY_GLOBAL(FieldName); \
			}\
			else if constexpr (nox::HasIndexOperatorValue<_T> == true)\
			{\
				return NOX_VARIABLE_INFO_LAMBDA_GETTER_ARRAY_GLOBAL(FieldName); \
			}\
			else\
			{\
				return nullptr; \
			}\
		}\
		}()

#pragma endregion


#pragma endregion
