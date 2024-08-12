//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	attribute.h
///	@brief	attribute
#pragma once
namespace nox::reflection
{
	//	前方宣言
	class ReflectionObject;
	
	/// @brief 属性クラスインターフェース
	struct IAttribute
	{
	protected:
		[[nodiscard]] inline constexpr IAttribute()noexcept = default;
	};

	/// @brief 同時付与が不可能な属性の定義
	/// @tparam T 
	/// @tparam U 
	template<class T, class U> requires(!std::is_same_v<IAttribute, T> && std::is_base_of_v<IAttribute, T> && !std::is_same_v<IAttribute, U>&& std::is_base_of_v<IAttribute, U>)
	struct IsIgnoreAttribute : std::false_type {};

	/// @brief この属性を付けている場合、他の属性を付与できない
	/// @tparam T 
	template<class T> requires(!std::is_same_v<IAttribute, T> && std::is_base_of_v<IAttribute, T>)
	struct IsOnlyAttribute : std::false_type {};

#pragma region 属性型チェック
	namespace detail
	{
		template<class _FirstType, class... _Types>
		inline constexpr bool CheckAttributeType()noexcept
		{
			if constexpr (std::is_same_v<struct nox::reflection::IAttribute, _FirstType> == true)
			{
				static_assert(!std::is_same_v<struct nox::reflection::IAttribute, _FirstType>, "IAttirubte cannot be directly assigned");
				return false;
			}
			else if constexpr (nox::reflection::IsOnlyAttribute< _FirstType>::value == true)
			{
				static_assert(!nox::reflection::IsOnlyAttribute< _FirstType>::value, "only attribute");
				return false;
			}
			else if constexpr (std::is_base_of_v<struct nox::reflection::IAttribute, _FirstType> == false)
			{
				static_assert(std::is_base_of_v<struct nox::reflection::IAttribute, _FirstType>, "IAttirubte not inherited");
				return false;
			}
			else if constexpr (sizeof...(_Types) <= 0)
			{
				return true;
			}
			//	属性が複数ある
			else 
			{
				if constexpr (std::disjunction_v<std::is_same<_FirstType, _Types>...> == true)
				{
					static_assert(std::disjunction_v<std::is_same<_FirstType, _Types>...>, "duplication attribute");
					return false;
				}
				else if constexpr (std::disjunction_v<nox::reflection::IsIgnoreAttribute<_FirstType, _Types>...>)
				{
					static_assert(!std::disjunction_v<nox::reflection::IsIgnoreAttribute<_FirstType, _Types>...>, "ignore attribute");
					return false;
				}
				else if constexpr (std::disjunction_v<nox::reflection::IsIgnoreAttribute<_Types, _FirstType>...>)
				{
					static_assert(!std::disjunction_v<nox::reflection::IsIgnoreAttribute<_Types, _FirstType>...>, "ignore attribute");
					return false;
				}
				else
				{
					return true;
				}
			}
		}

		template<class _FirstType, class... _Types>
		inline constexpr bool CheckAttributesImpl()noexcept
		{
			if constexpr (CheckAttributeType<_FirstType, _Types...>() == false)
			{
				return false;
			}
			else if constexpr (sizeof...(_Types) <= 0)
			{
				return true;
			}
			else
			{
				return CheckAttributesImpl<_Types...>() == true;
			}
		}

		///@brief	Tuple型で受け取り可変長テンプレート引数として処理するための構造体
		template<class... _Types>
		struct CheckAttributesTuple;

		///@brief	Tuple型で受け取り可変長テンプレート引数として処理するための構造体
		///@details	型が1つの場合
		template<class _FirstType>
		struct CheckAttributesTuple<std::tuple<_FirstType>>
		{
		private:
			static inline constexpr bool CheckAttributeTypeSingle()noexcept
			{
				if constexpr (std::is_same_v<struct nox::reflection::IAttribute, _FirstType> == true)
				{
					static_assert(!std::is_same_v<struct nox::reflection::IAttribute, _FirstType>, "IAttirubte cannot be directly assigned");
					return false;
				}
				else if constexpr (std::is_base_of_v<struct nox::reflection::IAttribute, _FirstType> == false)
				{
					static_assert(std::is_base_of_v<struct nox::reflection::IAttribute, _FirstType>, "IAttirubte not inherited");
					return false;
				}
				else
				{
					return true;
				}
			}
		public:
			static constexpr bool value = CheckAttributeTypeSingle();
		};

		///@brief	Tuple型で受け取り可変長テンプレート引数として処理するための構造体
		///@details	型が複数の場合
		template<class _FirstType, class... _Types>
		struct CheckAttributesTuple<std::tuple<_FirstType, _Types...>>
		{
			static constexpr bool value = CheckAttributesImpl<_FirstType, _Types...>();

			static bool test()noexcept
			{
				return CheckAttributesImpl<_FirstType, _Types...>();
			}
		};

		///@brief	属性群が定義可能かチェックする
		///@details	Tuple型を受け取ります
		template<class _TupleType>
		inline constexpr bool CheckAttributes()noexcept
		{
			return CheckAttributesTuple<_TupleType>::value;
		}
	}
#pragma endregion

///@brief	属性の特殊化用マクロ
#define	NOX_ATTIRBUTE_SPECIALIZATION(TypeTraitsClass, ...) \
	namespace nox::reflection { \
		template<>	\
		struct TypeTraitsClass<__VA_ARGS__> : std::true_type {};	\
	}
//	end define

//	属性付与マクロ
#if NOX_REFLECTION_GENERATOR
//#if defined(__clang__)
///@brief	属性付与のためのマクロ	エンジン外でannotate属性が使われることを想定して、NOX_REFLECTION_GENERATORを先頭に付けておく
//#define	NOX_DETAIL_ATTR_IMPL3(x) __attribute__((annotate(#x)))
//#define	NOX_DETAIL_ATTR_IMPL2(x) NOX_DETAIL_ATTR_IMPL3(x)
//#define	NOX_DETAIL_ATTR_IMPL(x) NOX_DETAIL_ATTR_IMPL2(NOX_PP_CAT_I(NOX_REFLECTION_GENERATOR,x))
	#define	NOX_DETAIL_ATTR_IMPL(x) __attribute__((annotate(#x)))
#else
///@brief	属性付与のためのマクロ
#define	NOX_DETAIL_ATTR_IMPL(x) //	[[annotate(NOX_PP_TO_STRING(NOX_REFLECTION_GENERATOR##x))]]
#endif // NOX_REFLECTION_GENERATOR

///@brief	属性付与
///@details	エンジン外でannotate属性が使われることを想定して、annotate("NOX_REFLECTION_GENERATOR")を付与する
#define	NOX_ATTR(...) NOX_DETAIL_ATTR_IMPL(NOX_REFLECTION_GENERATOR) NOX_PP_REPEAT_AUTO(NOX_DETAIL_ATTR_IMPL, __VA_ARGS__)

///@brief	型に対しての属性付与
#define	NOX_ATTR_TYPE(...) \
	alignas([]()constexpr noexcept{return 0; static_assert(::nox::reflection::detail::CheckAttributes<decltype(std::make_tuple(__VA_ARGS__))>(), "failed attributes"); }())	\
	NOX_ATTR(__VA_ARGS__)

///@brief	Enumメンバに対しての属性付与
#define	NOX_ATTR_ENUMERATOR(...) NOX_ATTR(__VA_ARGS__)

///@brief	変数や関数などの定義に対しての属性付与
#define NOX_ATTR_DECLARATION(...)	\
	static_assert(::nox::reflection::detail::CheckAttributes<decltype(std::make_tuple(__VA_ARGS__))>(), "failed attributes");	\
	NOX_ATTR(__VA_ARGS__)

	/// @brief 属性
	namespace attr
	{
		//	リフレクションの種類
		//	1.	全てのクラスを対象にリフレクション
		//	2.	ExpressedReflectionを付けたもの、ReflectionObjectを継承したもの、NOX_REFLECTION_DECLAREを付けたものを対象にリフレクション
		//	3.	リフレクションをしない

		/// @brief		リフレクション対象外属性
		/// @details	システム属性
		class IgnoreReflection : public IAttribute
		{
		};

		/// @brief		リフレクション対象として表明
		/// @details	システム属性
		class ExpressedReflection : public IAttribute
		{
		};
	}

	template<>
	struct IsOnlyAttribute<attr::IgnoreReflection> : std::true_type {};

	template<>
	struct IsOnlyAttribute<attr::ExpressedReflection> : std::true_type {};
}