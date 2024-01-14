///	@file		type_traits.h
///	@brief		type_traits
#pragma once
#include    <type_traits>

#include	"../if/basic_type.h"
#include	"../if/advanced_type.h"

namespace nox
{
#pragma region ContainerElementType
	namespace detail
	{
		template<class T>
		struct ContainerElement
		{
			using type = T;
		};

		template<class T> requires(std::is_array_v<T>)
			struct ContainerElement<T>
		{
			using type = std::remove_extent_t<T>;
		};

		template<class T> requires(std::is_same_v<typename T::value_type, typename T::value_type> && !std::is_array_v<T>)
		struct ContainerElement<T>
		{
			using type = typename T::value_type;
		};
	}
	template<class T>
	using ContainerElementType = typename detail::ContainerElement<T>::type;

	
#pragma endregion


	namespace detail
	{
		/// @brief 関数の型にnoexceptが修飾されているかどうか
		/// @tparam F 関数の型
		/// @tparam ...Args 関数の引数の型
		template <typename F, typename... Args>
		struct IsNoexcept : std::false_type {};

		/// @brief 関数の型にnoexceptが修飾されているかどうか
		/// @tparam F 関数の型
		/// @tparam ...Args 関数の引数の型
		template <typename F, typename... Args>
		struct IsNoexcept<F(Args...) noexcept> : std::true_type {};

		//	templateの引数の数を取得
		//	他に方法が思いつかない...
		/// @brief templateの引数の数を取得
		template<template<class> class>
		consteval uint8 GetTemplateParamLength()noexcept { return 1U; }
		template<template<class, class> class>
		consteval uint8 GetTemplateParamLength()noexcept { return 2U; }
		template<template<class, class, class> class>
		consteval uint8 GetTemplateParamLength()noexcept { return 3U; }

		template<class T>
		struct IsVector : std::false_type {};

		template<class... Args>
		struct IsVector<std::vector<Args...>> : std::true_type {};

		template<class T>
		constexpr bool IsVectorV = IsVector<T>::value;

		template <typename T>
		struct IsStdArray : std::false_type {};

		template <typename T, std::size_t N>
		struct IsStdArray<std::array<T, N>> : std::true_type {};

		template<class T>
		constexpr bool IsStdArrayV = IsStdArray<T>::value;

		template<typename T, bool B = std::is_enum_v<T>>
		struct IsScopedEnum : std::false_type {};

		template<typename T>
		struct IsScopedEnum<T, true> : std::integral_constant<bool, !std::is_convertible_v<T, std::underlying_type_t<T>>> {};

	
	}

	/// @brief グローバル関数ポインタ型か
	/// @tparam T 型
	template<class T>
	constexpr bool IsGlobalFunctionPointerValue = std::is_function_v<std::remove_pointer_t<std::decay_t<T>>>;

	namespace concepts
	{
		template<class T>
		concept GlobalFunctionPointer = IsGlobalFunctionPointerValue<T>;
	}

	namespace detail
	{
		/// @brief 関数ポインタ型か
		/// @tparam T 型
		template<class T>
		struct IsFunctionPointer : std::false_type {};

		template<class T> requires(IsGlobalFunctionPointerValue<T>)
		struct IsFunctionPointer<T> : std::true_type {};

		template<class T> requires(std::is_member_function_pointer_v<T>)
		struct IsFunctionPointer<T> : std::true_type {};
	}

	/// @brief  関数ポインタ型か
	/// @tparam T 型
	template<class T>
	constexpr bool IsFunctionPointerV = detail::IsFunctionPointer<T>::value;

	/**
	 * @brief sizeof可能な型かどうか
	 * @tparam T
	 * @tparam U
	*/
	template<class T, typename U = size_t>
	struct IsSizeofType : std::false_type {};

	/**
	 * @brief sizeof可能な型かどうか
	*/
	template<class T>
	struct IsSizeofType<T, decltype(sizeof(T))> : std::true_type {};

	/**
	 * @brief sizeof可能な型かどうか
	*/
	template<typename T>
	constexpr bool IsSizeofTypeValue = IsSizeofType<T>::value;

	namespace detail
	{
		
	}

	/**
	 * @brief あらゆる関数型
	*/
	template<class T>
	constexpr bool IsEveryFunctionV =
		nox::IsFunctionPointerV<T> ||
		std::is_function_v<T>;

	namespace detail
	{
		template<class T>
		struct LambdaToFunctionImpl;

		template<class Ret, class Cls, class... Args>
		struct LambdaToFunctionImpl<Ret(Cls::*)(Args...)>
		{
			using type = Ret(*)(Args...);
		};

		template<class Ret, class Cls, class... Args>
		struct LambdaToFunctionImpl<Ret(Cls::*)(Args...)const>
		{
			using type = Ret(*)(Args...);
		};

		template<class Ret, class Cls, class... Args>
		struct LambdaToFunctionImpl<Ret(Cls::*)(Args...)noexcept>
		{
			using type = Ret(*)(Args...);
		};

		template<class Ret, class Cls, class... Args>
		struct LambdaToFunctionImpl<Ret(Cls::*)(Args...)const noexcept>
		{
			using type = Ret(*)(Args...);
		};

		/**
		 * @brief ラムダ式型かどうか
		*/
		template<class T>
		struct is_lambda : std::false_type {};

		template<class T> requires(IsEveryFunctionV<T>)
			struct is_lambda<T> : std::false_type {};

		template<class T>
			requires(!IsEveryFunctionV<T> && sizeof(detail::LambdaToFunctionImpl<decltype(&T::operator())>) >= 0)
		struct is_lambda<T> : std::true_type {};
	}

	namespace concepts
	{
		/**
		 * @brief あらゆる関数型
		*/
		template<class T>
		concept EveryFunctionType = IsEveryFunctionV<T>;
	}

	/// @brief 関数の型にnoexceptが修飾されているかどうか
	/// @tparam T 関数の型
	template <class T> requires(std::is_function_v<T>)
	constexpr bool IsNoexceptValue = detail::IsNoexcept<T>::value;

	/// @brief const pointer型を表現する
	/// @tparam T 
	template<class T>
	using AddConstPointerType = std::conditional_t<
		std::is_pointer_v<T>,
		std::add_pointer_t<std::add_const_t<std::remove_pointer_t<T>>>,
		T>;

	/**
	 * @brief const pointerからconstを除去する
	*/
	template<class T>
	using RemoveConstPointerType = std::conditional_t<
		std::is_pointer_v<T>,
		std::add_pointer_t<std::remove_const_t<std::remove_pointer_t<T>>>,
		T>;

	/**
	 * @brief const lvalue reference型を表現する
	*/
	template<class T>
	using RemoveConstLvalueReferenceType = std::conditional_t<
		std::is_lvalue_reference_v<T>,
		std::add_lvalue_reference_t<std::remove_const_t<std::remove_reference_t<T>>>,
		T>;

	/**
	 * @brief lvalue reference型を表現する
	*/
	template<class T>
	using AddConstLvalueReferenceType =
		std::conditional_t<
		std::is_lvalue_reference_v<T>,
		std::add_lvalue_reference_t<std::add_const_t<std::remove_reference_t<T>>>,
		T>;

	/**
	 * @brief const rvalue reference型を表現する
	*/
	template<class T>
	using RemoveConstRvalueReferenceType = std::conditional_t<
		std::is_rvalue_reference_v<T>,
		std::add_rvalue_reference_t<std::remove_const_t<std::remove_reference_t<T>>>,
		T>;

	/**
	 * @brief rvalue reference型を表現する
	*/
	template<class T>
	using AddConstRvalueReferenceType =
		std::conditional_t<
		std::is_rvalue_reference_v<T>,
		std::add_rvalue_reference_t<std::add_const_t<std::remove_reference_t<T>>>,
		T>;

	/**
	 * @brief constを除去
	*/
	template<class T>
	using RemoveConstPointerReferenceType = RemoveConstRvalueReferenceType<RemoveConstLvalueReferenceType<RemoveConstPointerType<T>>>;

	/**
	 * @brief const pointer型かどうか
	*/
	template<class T>
	constexpr bool IsConstPointerValue = std::is_pointer_v<T> && std::is_same_v <T, AddConstPointerType<T>>;

	/**
	 * @brief const lvalue reference型かどうか
	*/
	template<class T>
	constexpr bool IsConstLvalueReferenceValue = std::is_lvalue_reference_v<T> && std::is_same_v<T, AddConstLvalueReferenceType<T>>;

	/**
	 * @brief const rvalue reference型かどうか
	*/
	template<class T>
	constexpr bool IsConstRvalueReferenceValue = std::is_rvalue_reference_v<T> && std::is_same_v<T, AddConstRvalueReferenceType<T>>;

	/**
	 * @brief あらゆるconst型か
	*/
	template<class T>
	constexpr bool IsConstAllValue = std::is_const_v<T> || IsConstPointerValue<T> || IsConstLvalueReferenceValue<T> || IsConstRvalueReferenceValue<T>;

	/**
	 * @brief シーケンスコンテナ
	*/
	template<class T>
	constexpr bool IsSequenceContainerClassValue = detail::IsStdArrayV<T> || detail::IsVectorV<T>;

	/// @brief スコープ付きの列挙型か
	template<class T>
	constexpr bool IsScopedEnumValue =
#if defined(__clang__)
		detail::IsScopedEnum<T>::value;
#else
		std::is_scoped_enum_v<T>;
#endif // __clang__

	template<class T>
	struct LambdaToFunction;

	template<class T> requires(sizeof(detail::LambdaToFunctionImpl<decltype(&T::operator())>) >= 0)
		struct LambdaToFunction<T> : detail::LambdaToFunctionImpl<decltype(&T::operator())>
	{};

	template<concepts::EveryFunctionType T>
	struct LambdaToFunction<T>
	{
		using type = T;
	};

	template<class T>
	using LambdaToFunctionType = typename LambdaToFunction<T>::type;

	/**
	 * @brief ラムダかどうか
	 * @tparam T
	*/
	template<class T>
	constexpr bool IsLambdaValue = detail::is_lambda<T>::value;

	/**
	 * @brief キャプチャラムダ
	*/
	template<class T>
	struct IsCaptureLambda : std::true_type {};

	template<class T>requires(!IsEveryFunctionV<T>&& std::is_invocable_v<T> && sizeof(decltype(+T())) >= 0)
	struct IsCaptureLambda<T> : std::false_type {};

	template<class T>requires(IsEveryFunctionV<T>)
	struct IsCaptureLambda<T> : std::false_type {};

	template<class T>
	constexpr bool IsCaptureLambdaV = IsCaptureLambda<T>::value;

	/**
	 * @brief キャプチャのないラムダ
	*/
	template<class T>
	constexpr bool IsNotCaptureLambdaV = IsLambdaValue<T> && !IsCaptureLambdaV<T>;

	namespace detail
	{
		/**
	 * @brief 文字列型か
	*/
		template<class T>
		struct is_char_type : std::false_type {};

		/**
		 * @brief 文字列型か
		*/
		template<>
		struct is_char_type<char> : ::std::true_type {};
		template<>
		struct is_char_type<signed char> : ::std::true_type {};
		template<>
		struct is_char_type<unsigned char> : ::std::true_type {};
		template<>
		struct is_char_type<wchar_t> : ::std::true_type {};
		template<>
		struct is_char_type<char8_t> : ::std::true_type {};
		template<>
		struct is_char_type<char16_t> : ::std::true_type {};
		template<>
		struct is_char_type<char32_t> : ::std::true_type {};

		/**
		 * @brief 文字列型かどうか
		 * @tparam T
		*/
		template<class T>
		struct is_string_class : ::std::false_type {};

		/**
		 * @brief std::basic_string型
		 * @tparam ValueType
		 * @tparam TraitsType
		 * @tparam AllocatorType
		*/
		template<class ValueType, class TraitsType, class AllocatorType>
		struct is_string_class<::std::basic_string<ValueType, TraitsType, AllocatorType>>
			: ::std::true_type {};

		template<class T>
		struct IsStringViewClass : ::std::false_type {};

		/**
		 * @brief string_view型
		 * @tparam ValueType
		 * @tparam TraitsType
		*/
		template<class ValueType, class TraitsType>
		struct IsStringViewClass<::std::basic_string_view<ValueType, TraitsType>> : ::std::true_type {};
	}

	/**
	 * @brief 文字列型か
	*/
	template<class T>
	constexpr bool IsCharTypeValue = detail::is_char_type<T>::value;

	/**
	 * @brief string型かどうか
	 * @tparam T
	*/
	template<class T>
	constexpr bool IsStringClassValue = detail::is_string_class<T>::value;

	/**
	 * @brief string_view型かどうか
	 * @tparam T
	*/
	template<class T>
	constexpr bool IsStringViewClassValue = detail::IsStringViewClass<T>::value;

	template<class T>
	constexpr bool IsStringClassAllValue = IsStringClassValue<T> || IsStringViewClassValue<T>;

	namespace concepts::detail
	{
		template<class T>
		concept HasDefaultOperator = requires(T & x) {
			x.operator=(x);
		};

		template<class T>
		concept HasIndexOperator = requires(T & x) {
			x.operator[](0);
		};
	}

	template<class T>
	constexpr bool IsInvokableDefaultOperatorValue = nox::concepts::detail::HasDefaultOperator<T>;

	template<class T>
	constexpr bool HasIndexOperatorValue = nox::concepts::detail::HasIndexOperator<T>;

}