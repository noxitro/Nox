///	@file	function_signature.cpp
///	@brief		関数の特殊化のサンプルコード
/// @details	static_assertで関数の特殊化をチェックする
#include	"stdafx.h"
#include	"function_signature.h"

namespace
{
	template<class T, bool Test>
	constexpr bool CheckFunction = nox::detail::FunctionSignatureAdapter<T>::is_function == Test;

	template<class T, bool Test>
	constexpr bool CheckFunctionPointer = nox::detail::FunctionSignatureAdapter<T>::is_function_pointer == Test;

	template<class T, bool Test>
	constexpr bool CheckMemberFunctionPointer = nox::detail::FunctionSignatureAdapter<T>::is_member_function_pointer == Test;

	template<class T, bool Test>
	constexpr bool CheckConst = nox::detail::FunctionSignatureAdapter<T>::is_const == Test;

	template<class T, bool Test>
	constexpr bool CheckNoexcept = nox::detail::FunctionSignatureAdapter<T>::is_noexcept == Test;

	template<class T, bool Test>
	constexpr bool CheckVolatile = nox::detail::FunctionSignatureAdapter<T>::is_volatile == Test;

	template<class T, bool Test>
	constexpr bool CheckLValueReference = nox::detail::FunctionSignatureAdapter<T>::is_lvalue_reference == Test;

	template<class T, bool Test>
	constexpr bool CheckRValueReference = nox::detail::FunctionSignatureAdapter<T>::is_rvalue_reference == Test;

#pragma region 関数型
	// 0
	using Function00 = void();
	static_assert(CheckFunction<Function00, true>);
	static_assert(CheckFunctionPointer<Function00, false>);
	static_assert(CheckMemberFunctionPointer<Function00, false>);
	static_assert(CheckConst<Function00, false>);
	static_assert(CheckNoexcept<Function00, false>);
	static_assert(CheckVolatile<Function00, false>);
	static_assert(CheckLValueReference<Function00, false>);
	static_assert(CheckRValueReference<Function00, false>);

	// 1
	using Function01 = void() const;
	static_assert(CheckFunction<Function01, true>);
	static_assert(CheckFunctionPointer<Function01, false>);
	static_assert(CheckMemberFunctionPointer<Function01, false>);
	static_assert(CheckConst<Function01, true>);
	static_assert(CheckNoexcept<Function01, false>);
	static_assert(CheckVolatile<Function01, false>);
	static_assert(CheckLValueReference<Function01, false>);
	static_assert(CheckRValueReference<Function01, false>);

	// 2
	using Function02 = void() noexcept;
	static_assert(CheckFunction<Function02, true>);
	static_assert(CheckFunctionPointer<Function02, false>);
	static_assert(CheckMemberFunctionPointer<Function02, false>);
	static_assert(CheckConst<Function02, false>);
	static_assert(CheckNoexcept<Function02, true>);
	static_assert(CheckVolatile<Function02, false>);
	static_assert(CheckLValueReference<Function02, false>);
	static_assert(CheckRValueReference<Function02, false>);

	// 3
	using Function03 = void() volatile;
	static_assert(CheckFunction<Function03, true>);
	static_assert(CheckFunctionPointer<Function03, false>);
	static_assert(CheckMemberFunctionPointer<Function03, false>);
	static_assert(CheckConst<Function03, false>);
	static_assert(CheckNoexcept<Function03, false>);
	static_assert(CheckVolatile<Function03, true>);
	static_assert(CheckLValueReference<Function03, false>);
	static_assert(CheckRValueReference<Function03, false>);

	// 4
	using Function04 = void()&;
	static_assert(CheckFunction<Function04, true>);
	static_assert(CheckFunctionPointer<Function04, false>);
	static_assert(CheckMemberFunctionPointer<Function04, false>);
	static_assert(CheckConst<Function04, false>);
	static_assert(CheckNoexcept<Function04, false>);
	static_assert(CheckVolatile<Function04, false>);
	static_assert(CheckLValueReference<Function04, true>);
	static_assert(CheckRValueReference<Function04, false>);

	// 5
	using Function05 = void()&&;
	static_assert(CheckFunction<Function05, true>);
	static_assert(CheckFunctionPointer<Function05, false>);
	static_assert(CheckMemberFunctionPointer<Function05, false>);
	static_assert(CheckConst<Function05, false>);
	static_assert(CheckNoexcept<Function05, false>);
	static_assert(CheckVolatile<Function05, false>);
	static_assert(CheckLValueReference<Function05, false>);
	static_assert(CheckRValueReference<Function05, true>);

	// 6
	using Function06 = void() const noexcept;
	static_assert(CheckFunction<Function06, true>);
	static_assert(CheckFunctionPointer<Function06, false>);
	static_assert(CheckMemberFunctionPointer<Function06, false>);
	static_assert(CheckConst<Function06, true>);
	static_assert(CheckNoexcept<Function06, true>);
	static_assert(CheckVolatile<Function06, false>);
	static_assert(CheckLValueReference<Function06, false>);
	static_assert(CheckRValueReference<Function06, false>);

	// 7
	using Function07 = void() volatile noexcept;
	static_assert(CheckFunction<Function07, true>);
	static_assert(CheckFunctionPointer<Function07, false>);
	static_assert(CheckMemberFunctionPointer<Function07, false>);
	static_assert(CheckConst<Function07, false>);
	static_assert(CheckNoexcept<Function07, true>);
	static_assert(CheckVolatile<Function07, true>);
	static_assert(CheckLValueReference<Function07, false>);
	static_assert(CheckRValueReference<Function07, false>);

	// 8
	using Function08 = void() const volatile;
	static_assert(CheckFunction<Function08, true>);
	static_assert(CheckFunctionPointer<Function08, false>);
	static_assert(CheckMemberFunctionPointer<Function08, false>);
	static_assert(CheckConst<Function08, true>);
	static_assert(CheckNoexcept<Function08, false>);
	static_assert(CheckVolatile<Function08, true>);
	static_assert(CheckLValueReference<Function08, false>);
	static_assert(CheckRValueReference<Function08, false>);

	// 9
	using Function09 = void() const volatile noexcept;
	static_assert(CheckFunction<Function09, true>);
	static_assert(CheckFunctionPointer<Function09, false>);
	static_assert(CheckMemberFunctionPointer<Function09, false>);
	static_assert(CheckConst<Function09, true>);
	static_assert(CheckNoexcept<Function09, true>);
	static_assert(CheckVolatile<Function09, true>);
	static_assert(CheckLValueReference<Function09, false>);
	static_assert(CheckRValueReference<Function09, false>);

	// 10
	using Function10 = void() const&;
	static_assert(CheckFunction<Function10, true>);
	static_assert(CheckFunctionPointer<Function10, false>);
	static_assert(CheckMemberFunctionPointer<Function10, false>);
	static_assert(CheckConst<Function10, true>);
	static_assert(CheckNoexcept<Function10, false>);
	static_assert(CheckVolatile<Function10, false>);
	static_assert(CheckLValueReference<Function10, true>);
	static_assert(CheckRValueReference<Function10, false>);

	// 11
	using Function11 = void() const&&;
	static_assert(CheckFunction<Function11, true>);
	static_assert(CheckFunctionPointer<Function11, false>);
	static_assert(CheckMemberFunctionPointer<Function11, false>);
	static_assert(CheckConst<Function11, true>);
	static_assert(CheckNoexcept<Function11, false>);
	static_assert(CheckVolatile<Function11, false>);
	static_assert(CheckLValueReference<Function11, false>);
	static_assert(CheckRValueReference<Function11, true>);

	// 12
	using Function12 = void() volatile&;
	static_assert(CheckFunction<Function12, true>);
	static_assert(CheckFunctionPointer<Function12, false>);
	static_assert(CheckMemberFunctionPointer<Function12, false>);
	static_assert(CheckConst<Function12, false>);
	static_assert(CheckNoexcept<Function12, false>);
	static_assert(CheckVolatile<Function12, true>);
	static_assert(CheckLValueReference<Function12, true>);
	static_assert(CheckRValueReference<Function12, false>);

	// 13
	using Function13 = void() volatile&&;
	static_assert(CheckFunction<Function13, true>);
	static_assert(CheckFunctionPointer<Function13, false>);
	static_assert(CheckMemberFunctionPointer<Function13, false>);
	static_assert(CheckConst<Function13, false>);
	static_assert(CheckNoexcept<Function13, false>);
	static_assert(CheckVolatile<Function13, true>);
	static_assert(CheckLValueReference<Function13, false>);
	static_assert(CheckRValueReference<Function13, true>);

	// 14
	using Function14 = void() const volatile&;
	static_assert(CheckFunction<Function14, true>);
	static_assert(CheckFunctionPointer<Function14, false>);
	static_assert(CheckMemberFunctionPointer<Function14, false>);
	static_assert(CheckConst<Function14, true>);
	static_assert(CheckNoexcept<Function14, false>);
	static_assert(CheckVolatile<Function14, true>);
	static_assert(CheckLValueReference<Function14, true>);
	static_assert(CheckRValueReference<Function14, false>);

	// 15
	using Function15 = void() const volatile&&;
	static_assert(CheckFunction<Function15, true>);
	static_assert(CheckFunctionPointer<Function15, false>);
	static_assert(CheckMemberFunctionPointer<Function15, false>);
	static_assert(CheckConst<Function15, true>);
	static_assert(CheckNoexcept<Function15, false>);
	static_assert(CheckVolatile<Function15, true>);
	static_assert(CheckLValueReference<Function15, false>);
	static_assert(CheckRValueReference<Function15, true>);

	// 16
	using Function16 = void() const& noexcept;
	static_assert(CheckFunction<Function16, true>);
	static_assert(CheckFunctionPointer<Function16, false>);
	static_assert(CheckMemberFunctionPointer<Function16, false>);
	static_assert(CheckConst<Function16, true>);
	static_assert(CheckNoexcept<Function16, true>);
	static_assert(CheckVolatile<Function16, false>);
	static_assert(CheckLValueReference<Function16, true>);
	static_assert(CheckRValueReference<Function16, false>);

	// 17
	using Function17 = void() const&& noexcept;
	static_assert(CheckFunction<Function17, true>);
	static_assert(CheckFunctionPointer<Function17, false>);
	static_assert(CheckMemberFunctionPointer<Function17, false>);
	static_assert(CheckConst<Function17, true>);
	static_assert(CheckNoexcept<Function17, true>);
	static_assert(CheckVolatile<Function17, false>);
	static_assert(CheckLValueReference<Function17, false>);
	static_assert(CheckRValueReference<Function17, true>);

	// 18
	using Function18 = void() volatile& noexcept;
	static_assert(CheckFunction<Function18, true>);
	static_assert(CheckFunctionPointer<Function18, false>);
	static_assert(CheckMemberFunctionPointer<Function18, false>);
	static_assert(CheckConst<Function18, false>);
	static_assert(CheckNoexcept<Function18, true>);
	static_assert(CheckVolatile<Function18, true>);
	static_assert(CheckLValueReference<Function18, true>);
	static_assert(CheckRValueReference<Function18, false>);


#pragma endregion

#pragma region 関数ポインタ型
	// 0
	using FunctionPointer00 = void(*)();
	static_assert(CheckFunction<FunctionPointer00, false>);
	static_assert(CheckFunctionPointer<FunctionPointer00, true>);
	static_assert(CheckMemberFunctionPointer<FunctionPointer00, false>);
	static_assert(CheckConst<FunctionPointer00, false>);
	static_assert(CheckNoexcept<FunctionPointer00, false>);
	static_assert(CheckVolatile<FunctionPointer00, false>);
	static_assert(CheckLValueReference<FunctionPointer00, false>);
	static_assert(CheckRValueReference<FunctionPointer00, false>);

	// 2
	using FunctionPointer02 = void(*)() noexcept;
	static_assert(CheckFunction<FunctionPointer02, false>);
	static_assert(CheckFunctionPointer<FunctionPointer02, true>);
	static_assert(CheckMemberFunctionPointer<FunctionPointer02, false>);
	static_assert(CheckConst<FunctionPointer02, false>);
	static_assert(CheckNoexcept<FunctionPointer02, true>);
	static_assert(CheckVolatile<FunctionPointer02, false>);
	static_assert(CheckLValueReference<FunctionPointer02, false>);
	static_assert(CheckRValueReference<FunctionPointer02, false>);

#pragma endregion

#pragma region メンバ関数ポインタ
	//	無駄にクラス定義を増やさないように型エイリアスを使う
	using Class = std::bool_constant<true>;

	// 0
	using MemberFunctionPointer00 = void(Class::*)();
	static_assert(CheckFunction<MemberFunctionPointer00, false>);
	static_assert(CheckFunctionPointer<MemberFunctionPointer00, false>);
	static_assert(CheckMemberFunctionPointer<MemberFunctionPointer00, true>);
	static_assert(CheckConst<MemberFunctionPointer00, false>);
	static_assert(CheckNoexcept<MemberFunctionPointer00, false>);
	static_assert(CheckVolatile<MemberFunctionPointer00, false>);
	static_assert(CheckLValueReference<MemberFunctionPointer00, false>);
	static_assert(CheckRValueReference<MemberFunctionPointer00, false>);

	// 1
	using MemberFunctionPointer01 = void(Class::*)() const;
	static_assert(CheckFunction<MemberFunctionPointer01, false>);
	static_assert(CheckFunctionPointer<MemberFunctionPointer01, false>);
	static_assert(CheckMemberFunctionPointer<MemberFunctionPointer01, true>);
	static_assert(CheckConst<MemberFunctionPointer01, true>);
	static_assert(CheckNoexcept<MemberFunctionPointer01, false>);
	static_assert(CheckVolatile<MemberFunctionPointer01, false>);
	static_assert(CheckLValueReference<MemberFunctionPointer01, false>);
	static_assert(CheckRValueReference<MemberFunctionPointer01, false>);

	// 2
	using MemberFunctionPointer02 = void(Class::*)() noexcept;
	static_assert(CheckFunction<MemberFunctionPointer02, false>);
	static_assert(CheckFunctionPointer<MemberFunctionPointer02, false>);
	static_assert(CheckMemberFunctionPointer<MemberFunctionPointer02, true>);
	static_assert(CheckConst<MemberFunctionPointer02, false>);
	static_assert(CheckNoexcept<MemberFunctionPointer02, true>);
	static_assert(CheckVolatile<MemberFunctionPointer02, false>);
	static_assert(CheckLValueReference<MemberFunctionPointer02, false>);
	static_assert(CheckRValueReference<MemberFunctionPointer02, false>);

	// 3
	using MemberFunctionPointer03 = void(Class::*)() volatile;
	static_assert(CheckFunction<MemberFunctionPointer03, false>);
	static_assert(CheckFunctionPointer<MemberFunctionPointer03, false>);
	static_assert(CheckMemberFunctionPointer<MemberFunctionPointer03, true>);
	static_assert(CheckConst<MemberFunctionPointer03, false>);
	static_assert(CheckNoexcept<MemberFunctionPointer03, false>);
	static_assert(CheckVolatile<MemberFunctionPointer03, true>);
	static_assert(CheckLValueReference<MemberFunctionPointer03, false>);
	static_assert(CheckRValueReference<MemberFunctionPointer03, false>);

	// 4
	using MemberFunctionPointer04 = void(Class::*)()&;
	static_assert(CheckFunction<MemberFunctionPointer04, false>);
	static_assert(CheckFunctionPointer<MemberFunctionPointer04, false>);
	static_assert(CheckMemberFunctionPointer<MemberFunctionPointer04, true>);
	static_assert(CheckConst<MemberFunctionPointer04, false>);
	static_assert(CheckNoexcept<MemberFunctionPointer04, false>);
	static_assert(CheckVolatile<MemberFunctionPointer04, false>);
	static_assert(CheckLValueReference<MemberFunctionPointer04, true>);
	static_assert(CheckRValueReference<MemberFunctionPointer04, false>);

	// 5
	using MemberFunctionPointer05 = void(Class::*)()&&;
	static_assert(CheckFunction<MemberFunctionPointer05, false>);
	static_assert(CheckFunctionPointer<MemberFunctionPointer05, false>);
	static_assert(CheckMemberFunctionPointer<MemberFunctionPointer05, true>);
	static_assert(CheckConst<MemberFunctionPointer05, false>);
	static_assert(CheckNoexcept<MemberFunctionPointer05, false>);
	static_assert(CheckVolatile<MemberFunctionPointer05, false>);
	static_assert(CheckLValueReference<MemberFunctionPointer05, false>);
	static_assert(CheckRValueReference<MemberFunctionPointer05, true>);

	// 6
	using MemberFunctionPointer06 = void(Class::*)() const noexcept;
	static_assert(CheckFunction<MemberFunctionPointer06, false>);
	static_assert(CheckFunctionPointer<MemberFunctionPointer06, false>);
	static_assert(CheckMemberFunctionPointer<MemberFunctionPointer06, true>);
	static_assert(CheckConst<MemberFunctionPointer06, true>);
	static_assert(CheckNoexcept<MemberFunctionPointer06, true>);
	static_assert(CheckVolatile<MemberFunctionPointer06, false>);
	static_assert(CheckLValueReference<MemberFunctionPointer06, false>);
	static_assert(CheckRValueReference<MemberFunctionPointer06, false>);

	// 7
	using MemberFunctionPointer07 = void(Class::*)() volatile noexcept;
	static_assert(CheckFunction<MemberFunctionPointer07, false>);
	static_assert(CheckFunctionPointer<MemberFunctionPointer07, false>);
	static_assert(CheckMemberFunctionPointer<MemberFunctionPointer07, true>);
	static_assert(CheckConst<MemberFunctionPointer07, false>);
	static_assert(CheckNoexcept<MemberFunctionPointer07, true>);
	static_assert(CheckVolatile<MemberFunctionPointer07, true>);
	static_assert(CheckLValueReference<MemberFunctionPointer07, false>);
	static_assert(CheckRValueReference<MemberFunctionPointer07, false>);

	// 8
	using MemberFunctionPointer08 = void(Class::*)() const volatile;
	static_assert(CheckFunction<MemberFunctionPointer08, false>);
	static_assert(CheckFunctionPointer<MemberFunctionPointer08, false>);
	static_assert(CheckMemberFunctionPointer<MemberFunctionPointer08, true>);
	static_assert(CheckConst<MemberFunctionPointer08, true>);
	static_assert(CheckNoexcept<MemberFunctionPointer08, false>);
	static_assert(CheckVolatile<MemberFunctionPointer08, true>);
	static_assert(CheckLValueReference<MemberFunctionPointer08, false>);
	static_assert(CheckRValueReference<MemberFunctionPointer08, false>);

	// 9
	using MemberFunctionPointer09 = void(Class::*)() const volatile noexcept;
	static_assert(CheckFunction<MemberFunctionPointer09, false>);
	static_assert(CheckFunctionPointer<MemberFunctionPointer09, false>);
	static_assert(CheckMemberFunctionPointer<MemberFunctionPointer09, true>);
	static_assert(CheckConst<MemberFunctionPointer09, true>);
	static_assert(CheckNoexcept<MemberFunctionPointer09, true>);
	static_assert(CheckVolatile<MemberFunctionPointer09, true>);
	static_assert(CheckLValueReference<MemberFunctionPointer09, false>);
	static_assert(CheckRValueReference<MemberFunctionPointer09, false>);

	// 10
	using MemberFunctionPointer10 = void(Class::*)() const&;
	static_assert(CheckFunction<MemberFunctionPointer10, false>);
	static_assert(CheckFunctionPointer<MemberFunctionPointer10, false>);
	static_assert(CheckMemberFunctionPointer<MemberFunctionPointer10, true>);
	static_assert(CheckConst<MemberFunctionPointer10, true>);
	static_assert(CheckNoexcept<MemberFunctionPointer10, false>);
	static_assert(CheckVolatile<MemberFunctionPointer10, false>);
	static_assert(CheckLValueReference<MemberFunctionPointer10, true>);
	static_assert(CheckRValueReference<MemberFunctionPointer10, false>);

	// 11
	using MemberFunctionPointer11 = void(Class::*)() const&&;
	static_assert(CheckFunction<MemberFunctionPointer11, false>);
	static_assert(CheckFunctionPointer<MemberFunctionPointer11, false>);
	static_assert(CheckMemberFunctionPointer<MemberFunctionPointer11, true>);
	static_assert(CheckConst<MemberFunctionPointer11, true>);
	static_assert(CheckNoexcept<MemberFunctionPointer11, false>);
	static_assert(CheckVolatile<MemberFunctionPointer11, false>);
	static_assert(CheckLValueReference<MemberFunctionPointer11, false>);
	static_assert(CheckRValueReference<MemberFunctionPointer11, true>);

	// 12
	using MemberFunctionPointer12 = void(Class::*)() volatile&;
	static_assert(CheckFunction<MemberFunctionPointer12, false>);
	static_assert(CheckFunctionPointer<MemberFunctionPointer12, false>);
	static_assert(CheckMemberFunctionPointer<MemberFunctionPointer12, true>);
	static_assert(CheckConst<MemberFunctionPointer12, false>);
	static_assert(CheckNoexcept<MemberFunctionPointer12, false>);
	static_assert(CheckVolatile<MemberFunctionPointer12, true>);
	static_assert(CheckLValueReference<MemberFunctionPointer12, true>);
	static_assert(CheckRValueReference<MemberFunctionPointer12, false>);

	// 13
	using MemberFunctionPointer13 = void(Class::*)() volatile&&;
	static_assert(CheckFunction<MemberFunctionPointer13, false>);
	static_assert(CheckFunctionPointer<MemberFunctionPointer13, false>);
	static_assert(CheckMemberFunctionPointer<MemberFunctionPointer13, true>);
	static_assert(CheckConst<MemberFunctionPointer13, false>);
	static_assert(CheckNoexcept<MemberFunctionPointer13, false>);
	static_assert(CheckVolatile<MemberFunctionPointer13, true>);
	static_assert(CheckLValueReference<MemberFunctionPointer13, false>);
	static_assert(CheckRValueReference<MemberFunctionPointer13, true>);

	// 14
	using MemberFunctionPointer14 = void(Class::*)() const volatile&;
	static_assert(CheckFunction<MemberFunctionPointer14, false>);
	static_assert(CheckFunctionPointer<MemberFunctionPointer14, false>);
	static_assert(CheckMemberFunctionPointer<MemberFunctionPointer14, true>);
	static_assert(CheckConst<MemberFunctionPointer14, true>);
	static_assert(CheckNoexcept<MemberFunctionPointer14, false>);
	static_assert(CheckVolatile<MemberFunctionPointer14, true>);
	static_assert(CheckLValueReference<MemberFunctionPointer14, true>);
	static_assert(CheckRValueReference<MemberFunctionPointer14, false>);

	// 15
	using MemberFunctionPointer15 = void(Class::*)() const volatile&&;
	static_assert(CheckFunction<MemberFunctionPointer15, false>);
	static_assert(CheckFunctionPointer<MemberFunctionPointer15, false>);
	static_assert(CheckMemberFunctionPointer<MemberFunctionPointer15, true>);
	static_assert(CheckConst<MemberFunctionPointer15, true>);
	static_assert(CheckNoexcept<MemberFunctionPointer15, false>);
	static_assert(CheckVolatile<MemberFunctionPointer15, true>);
	static_assert(CheckLValueReference<MemberFunctionPointer15, false>);
	static_assert(CheckRValueReference<MemberFunctionPointer15, true>);

	// 16
	using MemberFunctionPointer16 = void(Class::*)() const& noexcept;
	static_assert(CheckFunction<MemberFunctionPointer16, false>);
	static_assert(CheckFunctionPointer<MemberFunctionPointer16, false>);
	static_assert(CheckMemberFunctionPointer<MemberFunctionPointer16, true>);
	static_assert(CheckConst<MemberFunctionPointer16, true>);
	static_assert(CheckNoexcept<MemberFunctionPointer16, true>);
	static_assert(CheckVolatile<MemberFunctionPointer16, false>);
	static_assert(CheckLValueReference<MemberFunctionPointer16, true>);
	static_assert(CheckRValueReference<MemberFunctionPointer16, false>);

	// 17
	using MemberFunctionPointer17 = void(Class::*)() const&& noexcept;
	static_assert(CheckFunction<MemberFunctionPointer17, false>);
	static_assert(CheckFunctionPointer<MemberFunctionPointer17, false>);
	static_assert(CheckMemberFunctionPointer<MemberFunctionPointer17, true>);
	static_assert(CheckConst<MemberFunctionPointer17, true>);
	static_assert(CheckNoexcept<MemberFunctionPointer17, true>);
	static_assert(CheckVolatile<MemberFunctionPointer17, false>);
	static_assert(CheckLValueReference<MemberFunctionPointer17, false>);
	static_assert(CheckRValueReference<MemberFunctionPointer17, true>);

	// 18
	using MemberFunctionPointer18 = void(Class::*)() volatile& noexcept;
	static_assert(CheckFunction<MemberFunctionPointer18, false>);
	static_assert(CheckFunctionPointer<MemberFunctionPointer18, false>);
	static_assert(CheckMemberFunctionPointer<MemberFunctionPointer18, true>);
	static_assert(CheckConst<MemberFunctionPointer18, false>);
	static_assert(CheckNoexcept<MemberFunctionPointer18, true>);
	static_assert(CheckVolatile<MemberFunctionPointer18, true>);
	static_assert(CheckLValueReference<MemberFunctionPointer18, true>);
	static_assert(CheckRValueReference<MemberFunctionPointer18, false>);


#pragma endregion
}
