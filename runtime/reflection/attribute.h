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

///	@brief		属性付与(テキスト形式)
///	@details	enum値など型定義が不可能なものに対して使う
///				行番号で判定されるので、必ず付与対象の上行に書いてください
#define	NOX_ATTR_TEXT(...)

#if NOX_REFLECTION_GENERATOR
#define	NOX_ATTR_RAW(...)

#define	NOX_ATTR(...)\
NOX_PP_CAT_I(class NoxAttributeReflectionGenerator, __COUNTER__) \
{inline constexpr void AttrContainer(const char* text = #__VA_ARGS__)const noexcept = delete;\
};
#else
#define	NOX_ATTR(...) 
#define	NOX_ATTR_RAW(...)
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
		template<std::derived_from<class ReflectionObject> Base> requires(std::is_base_of_v<IAttribute, Base>)
			class IgnoreReflection : public Base
		{
			NOX_DECLARE_REFLECTION_OBJECT(IgnoreReflection);
		protected:
			inline constexpr IgnoreReflection()noexcept = default;
		};

		/// @brief リフレクション対象として表明
		template<std::derived_from<class ReflectionObject> Base> requires(std::is_base_of_v<IAttribute, Base>)
			class ExpressedReflection : public Base
		{
			NOX_DECLARE_REFLECTION_OBJECT(ExpressedReflection);
		protected:
			inline constexpr ExpressedReflection()noexcept = default;
		};
	}


#endif // NOX_DEVELOP

}