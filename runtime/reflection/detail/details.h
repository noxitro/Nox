///	@file	details.h
///	@brief	details
#pragma once
#include	<cstdint>
#include	<cmath>
#include	<string_view>
#include	<vector>

namespace nox::reflection::detail
{

	//inline	constexpr std::uint32_t	Crc32(const std::string_view str)noexcept
	//{
	//	//			constexpr u32 CRC32POLY1 = 0x04C11DB7UL;
	//	constexpr std::uint32_t CRC32POLY2 = 0xEDB88320UL;/* 左右逆転 */

	//	std::uint32_t r = 0xFFFFFFFFUL;
	//	for (std::int32_t i = 0; i < static_cast<std::int32_t>(str.length()); i++)
	//	{
	//		r ^= str.at(i);
	//		for (std::int32_t j = 0; j < std::numeric_limits<std::int8_t>::digits; j++)
	//		{
	//			if (r & 1)
	//			{
	//				r = (r >> 1) ^ CRC32POLY2;
	//			}
	//			else
	//			{
	//				r >>= 1;
	//			}
	//		}
	//	}
	//	return r ^ 0xFFFFFFFFUL;
	//}

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
	 * @brief あらゆる関数型
	*/
	template<class T>
	constexpr bool IsEveryFunctionV =
		IsFunctionPointerV<T> ||
		std::is_function_v<T>;

	namespace concepts
	{
		template<class T>
		concept Enum = std::is_enum<T>::value;

		template<class T>
		concept Arithmetic = std::is_arithmetic_v<T>;

		template<class T>
		concept ClassUnion = std::is_class_v<T> || std::is_union_v<T>;
	}

	namespace util
	{
		template<typename T>
		[[nodiscard]] inline	constexpr	std::underlying_type_t<T> ToUnderlying(const T value)noexcept
		{
#if defined(__clang__)
			return static_cast<std::underlying_type_t<T>>(value);
#else
			return std::to_underlying(value);
#endif
		}

		template<concepts::ClassUnion T>
		[[nodiscard]] inline constexpr T BitOr(const T& a, const T& b)noexcept { return std::bit_or<T>()(a, b); }

		template<concepts::Enum T>
		[[nodiscard]] inline constexpr T BitOr(const T a, const T b)noexcept { return static_cast<T>(std::bit_or<std::underlying_type_t<T>>()(ToUnderlying(a), ToUnderlying(b))); }

		template<concepts::Arithmetic T>
		[[nodiscard]] inline constexpr T BitOr(const T a, const T b)noexcept { return std::bit_or<T>()(a, b); }

		template<concepts::ClassUnion T>
		[[nodiscard]] inline	constexpr	bool IsBitAnd(const T& a, const T& b)noexcept { return std::bit_and<T>()(a, b); }

		template<concepts::Enum T>
		[[nodiscard]] inline	constexpr	bool IsBitAnd(const T a, const T b)noexcept { return std::bit_and<std::underlying_type_t<T>>()(ToUnderlying(a), ToUnderlying(b)); }

		template<class T>
		[[nodiscard]] inline	constexpr	bool IsBitAnd(const T a, const T b)noexcept { return std::bit_and<T>()(a, b); }
	}

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

		/// @brief グローバル関数ポインタ型か
	/// @tparam T 型
		template<class T>
		struct IsGlobalFunctionPointer : std::false_type {};

		/// @brief グローバル関数ポインタ型か
		/// @tparam _ReturnType 戻り値の型
		/// @tparam ...Args 引数の型
		template<class _ReturnType, class... Args>
		struct IsGlobalFunctionPointer<_ReturnType(*)(Args...)> : std::true_type {};

		/// @brief グローバル関数ポインタ型か(noexcept版)
		/// @tparam _ReturnType  戻り値の型
		/// @tparam ...Args 引数の型
		template<class _ReturnType, class... Args>
		struct IsGlobalFunctionPointer<_ReturnType(*)(Args...)noexcept> : std::true_type {};

		template<typename T, bool B = std::is_enum_v<T>>
		struct IsScopedEnum : std::false_type {};

		template<typename T>
		struct IsScopedEnum<T, true> : std::integral_constant<bool, !std::is_convertible_v<T, std::underlying_type_t<T>>> {};

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

	/// @brief グローバル関数ポインタ型か
	/// @tparam T 型
	template<class T>
	constexpr bool IsGlobalFunctionPointerValue = detail::IsGlobalFunctionPointer<T>::value;

	/// @brief スコープ付きの列挙型か
	template<class T>
	constexpr bool IsScopedEnumValue =
#if defined(__clang__)
		detail::IsScopedEnum<T>::value;
#else
		std::is_scoped_enum_v<T>;
#endif // __clang__

	/**
	 * @brief ラムダかどうか
	 * @tparam T
	*/
	template<class T>
	constexpr bool IsLambdaValue = detail::is_lambda<T>::value;

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


	

	namespace concepts
	{
		/**
		 * @brief あらゆる関数型
		*/
		template<class T>
		concept EveryFunctionType = IsEveryFunctionV<T>;
	}

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
}