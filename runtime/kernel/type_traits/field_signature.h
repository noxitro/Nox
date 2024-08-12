///	@file	field_signature.h
///	@brief	field_signature
#pragma once
#include	"concepts.h"

namespace nox
{
	/**
	 * @brief 変数情報
	 * @tparam T
	*/
	template<class T>
	struct VariableSignature;

	template<class _ReturnType>
	struct VariableSignature<_ReturnType(*)>
	{
		using ReturnType = _ReturnType;

		/**
		 * @brief メンバ変数か
		*/
		static inline constexpr bool IsMemberVariable()noexcept { return false; }
	};

	template<class _ReturnType, class _ClassType>
	struct VariableSignature<_ReturnType(_ClassType::*)>
	{
		using ReturnType = _ReturnType;
		using ClassType = _ClassType;

		/**
		 * @brief メンバ変数か
		*/
		static inline constexpr bool IsMemberVariable()noexcept { return true; }
	};

	namespace concepts
	{
		/// @brief フィールド型
		template<class T>
		concept FieldSignatureType = std::is_class_v<VariableSignature<T>>;
	}

	template<concepts::FieldSignatureType T>
	using FieldReturnType = typename VariableSignature<T>::ReturnType;

	/**
	 * @brief メンバオブジェクトポインタのクラス型
	*/
	template<concepts::MemberObjectPointer T>
	using FieldClassType = typename VariableSignature<T>::ClassType;

	/// @brief メンバ変数か
	template<concepts::FieldSignatureType T>
	constexpr bool IsFieldMemberVariableValue = VariableSignature<T>::IsMemberVariable();
}