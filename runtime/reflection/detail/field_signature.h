///	@file	field_signature.h
///	@brief	field_signature
#pragma once

namespace nox::reflection::detail
{
	/**
	 * @brief 変数情報
	 * @tparam T
	*/
	template<class T>
	struct VariableSignature;

	template<class _ResultType>
	struct VariableSignature<_ResultType(*)>
	{
		using ReturnType = _ResultType;

		/**
		 * @brief メンバ変数か
		*/
		static constexpr bool IsMemberVariable()noexcept { return false; }
	};

	template<class _ResultType, class _ClassType>
	struct VariableSignature<_ResultType(_ClassType::*)>
	{
		using ReturnType = _ResultType;
		using ClassType = _ClassType;

		/**
		 * @brief メンバ変数か
		*/
		static constexpr bool IsMemberVariable()noexcept { return true; }
	};

	namespace concepts
	{
		/// @brief フィールド型
		template<class T>
		concept FieldSignatureType = std::is_class_v<VariableSignature<T>>;
	}

	template<concepts::FieldSignatureType T>
	using VariablePointeeType = typename VariableSignature<T>::ReturnType;

	/**
	 * @brief メンバオブジェクトポインタのクラス型
	*/
	template<class T> requires(std::is_member_object_pointer_v<T>)
	using VariableOwnerType = typename VariableSignature<T>::ClassType;

	/// @brief メンバ変数か
	template<concepts::FieldSignatureType T>
	constexpr bool IsFieldMemberVariableValue = VariableSignature<T>::IsMemberVariable();
}