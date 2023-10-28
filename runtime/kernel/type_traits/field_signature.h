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
	struct FieldSignature;

	template<class _ReturnType>
	struct FieldSignature<_ReturnType(*)>
	{
		using ReturnType = _ReturnType;

		/**
		 * @brief メンバ変数か
		*/
		static inline constexpr bool IsMemberVariable()noexcept { return false; }
	};

	template<class _ReturnType, class _ClassType>
	struct FieldSignature<_ReturnType(_ClassType::*)>
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
		concept FieldSignatureType = std::is_class_v<FieldSignature<T>>;
	}

	template<concepts::FieldSignatureType T>
	using FieldReturnType = typename FieldSignature<T>::ReturnType;

	/**
	 * @brief メンバオブジェクトポインタのクラス型
	*/
	template<concepts::MemberObjectPointer T>
	using FieldClassType = typename FieldSignature<T>::ClassType;

	/// @brief メンバ変数か
	template<concepts::FieldSignatureType T>
	constexpr bool IsFieldMemberVariableValue = FieldSignature<T>::IsMemberVariable();
}