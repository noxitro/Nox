///	@file	object_pointer_signature.h
///	@brief	変数ポインタのシグネチャ解析
#pragma once
#include	"concepts.h"

namespace nox
{
	namespace detail
	{
		/// @brief 変数ポインタ解析
		/// @tparam T 
		template<class T>
		struct ObjectPointerSignature;

		template<class _ResultType>
		struct ObjectPointerSignature<_ResultType(*)>
		{
			using ResultType = _ResultType;
		};

		template<class _ResultType, class _ClassType>
		struct ObjectPointerSignature<_ResultType(_ClassType::*)>
		{
			using ResultType = _ResultType;
			using ClassType = _ClassType;
		};
	}
	

	/// @brief 変数の型
	/// @tparam T 
	template<class T>
	using ObjectPointerResultType = typename nox::detail::ObjectPointerSignature<T>::ResultType;

	/// @brief メンバオブジェクトポインタのクラス型
	/// @tparam T 
	template<nox::concepts::MemberObjectPointer T>
	using MemberObjectPointerClassType = typename nox::detail::ObjectPointerSignature<T>::ClassType;

}