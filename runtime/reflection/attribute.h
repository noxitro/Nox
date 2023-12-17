//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	attribute.h
///	@brief	attribute
#pragma once

#include	"reflection_object.h"

namespace nox::reflection
{
	//	前方宣言
	class ReflectionObject;
	struct IAttribute;

	namespace detail
	{
		template<std::derived_from<class IAttribute> FirstType, std::derived_from<class IAttribute>... Types>
		inline consteval bool CheckAttributes()noexcept
		{
			return true;
		}
	}


#if NOX_REFLECTION_GENERATOR
#define	NOX_ATTR(...)\
NOX_ATTR_RAW(#__VA_ARGS__)

#else
#define	NOX_ATTR(...) \
NOX_ATTR_RAW(#__VA_ARGS__)

#endif // NOX_REFLECTION_GENERATOR

#if NOX_DEVELOP
	/// @brief 属性
	namespace attr
	{
		//	リフレクションの種類
		//	1.	全てのクラスを対象にリフレクション
		//	2.	ExpressedReflectionを付けたもの、ReflectionObjectを継承したもの、NOX_REFLECTION_DECLAREを付けたものを対象にリフレクション
		//	3.	リフレクションをしない

		/// @brief リフレクション対象外属性
		template<std::derived_from<class ReflectionObject> Base>// requires(std::is_base_of_v<IAttribute, Base>)
			class IgnoreReflectionBase : public Base
		{
			NOX_DECLARE_REFLECTION_OBJECT(IgnoreReflectionBase);
		protected:
			inline constexpr IgnoreReflectionBase()noexcept = default;
		};

		/// @brief リフレクション対象として表明
		template<std::derived_from<class ReflectionObject> Base>// requires(std::is_base_of_v<IAttribute, Base>)
			class ExpressedReflection : public Base
		{
			NOX_DECLARE_REFLECTION_OBJECT(ExpressedReflection);
		protected:
			inline constexpr ExpressedReflection()noexcept = default;
		};

		using IgnoreReflection = ::nox::reflection::attr::IgnoreReflectionBase<class ReflectionObject>;
	}


#endif // NOX_DEVELOP

}