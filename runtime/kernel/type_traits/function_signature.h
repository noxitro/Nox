// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

///	@file	function_signature.h
///	@brief	関数の特殊化
#pragma once

#include	"type_traits.h"
#pragma warning(push)
#pragma warning(disable:4624)
namespace nox
{
	namespace detail
	{
		template<class _Result, class... Args>
		struct FunctionSignatureBase
		{
			using ResultType = _Result;

			using ArgsTupleType = std::tuple<Args...>;

			using RawFunctionType = ResultType(Args...);

			using RawFunctionPointerType = ResultType(*)(Args...);

			template<class T>
			using RawMemberFunctionPointerType = ResultType(*)(T, Args...);

			inline constexpr FunctionSignatureBase()noexcept = delete;
			inline constexpr ~FunctionSignatureBase()noexcept = delete;
		};

		template<class Result, class... Args>
		struct FunctionSignatureFunction : FunctionSignatureBase<Result, Args...>
		{
			static constexpr bool is_function = true;
			static constexpr bool is_function_pointer = false;
			static constexpr bool is_member_function_pointer = false;
		};

		template<class Result, class... Args>
		struct FunctionSignatureFunctionPointer : FunctionSignatureBase<Result, Args...>
		{
			static constexpr bool is_function = false;
			static constexpr bool is_function_pointer = true;
			static constexpr bool is_member_function_pointer = false;

			static constexpr bool is_const = false;
			static constexpr bool is_volatile = false;
			static constexpr bool is_lvalue_reference = false;
			static constexpr bool is_rvalue_reference = false;
		};

		template<class _ClassType, class Result, class... Args>
		struct FunctionSignatureMemberFunctionPointer : FunctionSignatureBase<Result, Args...>
		{
			using ClassType = _ClassType;
			static constexpr bool is_function = false;
			static constexpr bool is_function_pointer = false;
			static constexpr bool is_member_function_pointer = true;
		};

		template<class T>
		struct FunctionSignature;

		template<class T>
		using FunctionSignatureAdapter = FunctionSignature<std::remove_cvref_t<T>>;
#pragma region 関数型
		template<class Result, class... Args>
		struct FunctionSignature<Result(Args...)> : nox::detail::FunctionSignatureFunction<Result, Args...>
		{
			static constexpr bool is_const = false;
			static constexpr bool is_volatile = false;
			static constexpr bool is_noexcept = false;
			static constexpr bool is_lvalue_reference = false;
			static constexpr bool is_rvalue_reference = false;

			using ToFunctionType = Result(Args...);
			using ToFunctionPointerType = Result(*)(Args...);
			template<class U>
			using ToMemberFunctionPointerType = Result(U::*)(Args...);
		};

		template<class Result, class... Args>
		struct FunctionSignature<Result(Args...) const> : nox::detail::FunctionSignatureFunction<Result, Args...>
		{
			static constexpr bool is_const = true;
			static constexpr bool is_volatile = false;
			static constexpr bool is_noexcept = false;
			static constexpr bool is_lvalue_reference = false;
			static constexpr bool is_rvalue_reference = false;

			using ToFunctionType = Result(Args...) const;
			using ToFunctionPointerType = Result(*)(Args...);
			template<class U>
			using ToMemberFunctionPointerType = Result(U::*)(Args...) const;
		};

		template<class Result, class... Args>
		struct FunctionSignature<Result(Args...) volatile> : nox::detail::FunctionSignatureFunction<Result, Args...>
		{
			static constexpr bool is_const = false;
			static constexpr bool is_volatile = true;
			static constexpr bool is_noexcept = false;
			static constexpr bool is_lvalue_reference = false;
			static constexpr bool is_rvalue_reference = false;

			using ToFunctionType = Result(Args...) volatile;
			using ToFunctionPointerType = Result(*)(Args...);
			template<class U>
			using ToMemberFunctionPointerType = Result(U::*)(Args...) volatile;
		};

		template<class Result, class... Args>
		struct FunctionSignature<Result(Args...) noexcept> : nox::detail::FunctionSignatureFunction<Result, Args...>
		{
			static constexpr bool is_const = false;
			static constexpr bool is_volatile = false;
			static constexpr bool is_noexcept = true;
			static constexpr bool is_lvalue_reference = false;
			static constexpr bool is_rvalue_reference = false;

			using ToFunctionType = Result(Args...) noexcept;
			using ToFunctionPointerType = Result(*)(Args...) noexcept;
			template<class U>
			using ToMemberFunctionPointerType = Result(U::*)(Args...) noexcept;
		};
		
		template<class Result, class... Args>
		struct FunctionSignature<Result(Args...)const noexcept> : nox::detail::FunctionSignatureFunction<Result, Args...>
		{
			static constexpr bool is_const = true;
			static constexpr bool is_volatile = false;
			static constexpr bool is_noexcept = true;
			static constexpr bool is_lvalue_reference = false;
			static constexpr bool is_rvalue_reference = false;

			using ToFunctionType = Result(Args...) const noexcept;
			using ToFunctionPointerType = Result(*)(Args...) noexcept;
			template<class U>
			using ToMemberFunctionPointerType = Result(U::*)(Args...) const noexcept;
		};

		template<class Result, class... Args>
		struct FunctionSignature<Result(Args...)volatile noexcept> : nox::detail::FunctionSignatureFunction<Result, Args...>
		{
			static constexpr bool is_const = false;
			static constexpr bool is_volatile = true;
			static constexpr bool is_noexcept = true;
			static constexpr bool is_lvalue_reference = false;
			static constexpr bool is_rvalue_reference = false;

			using ToFunctionType = Result(Args...) volatile noexcept;
			using ToFunctionPointerType = Result(*)(Args...) noexcept;
			template<class U>
			using ToMemberFunctionPointerType = Result(U::*)(Args...) volatile noexcept;
		};
		
		template<class Result, class... Args>
		struct FunctionSignature<Result(Args...)const volatile> : nox::detail::FunctionSignatureFunction<Result, Args...>
		{
			static constexpr bool is_const = true;
			static constexpr bool is_volatile = true;
			static constexpr bool is_noexcept = false;
			static constexpr bool is_lvalue_reference = false;
			static constexpr bool is_rvalue_reference = false;

			using ToFunctionType = Result(Args...)const volatile;
			using ToFunctionPointerType = Result(*)(Args...) ;
			template<class U>
			using ToMemberFunctionPointerType = Result(U::*)(Args...) const volatile;
		};
		
		template<class Result, class... Args>
		struct FunctionSignature<Result(Args...)const volatile noexcept> : nox::detail::FunctionSignatureFunction<Result, Args...>
		{
			static constexpr bool is_const = true;
			static constexpr bool is_volatile = true;
			static constexpr bool is_noexcept = true;
			static constexpr bool is_lvalue_reference = false;
			static constexpr bool is_rvalue_reference = false;

			using ToFunctionType = Result(Args...) const volatile noexcept;
			using ToFunctionPointerType = Result(*)(Args...) noexcept;
			template<class U>
			using ToMemberFunctionPointerType = Result(U::*)(Args...) const volatile noexcept;
		};
		
		template<class Result, class... Args>
		struct FunctionSignature<Result(Args...)const& noexcept> : nox::detail::FunctionSignatureFunction<Result, Args...>
		{
			static constexpr bool is_const = true;
			static constexpr bool is_volatile = false;
			static constexpr bool is_noexcept = true;
			static constexpr bool is_lvalue_reference = true;
			static constexpr bool is_rvalue_reference = false;

			using ToFunctionType = Result(Args...) const& noexcept;
			using ToFunctionPointerType = Result(*)(Args...) noexcept;
			template<class U>
			using ToMemberFunctionPointerType = Result(U::*)(Args...) const& noexcept;
		};
		
		template<class Result, class... Args>
		struct FunctionSignature<Result(Args...)const volatile&> : nox::detail::FunctionSignatureFunction<Result, Args...>
		{
			static constexpr bool is_const = true;
			static constexpr bool is_volatile = true;
			static constexpr bool is_noexcept = false;
			static constexpr bool is_lvalue_reference = true;
			static constexpr bool is_rvalue_reference = false;

			using ToFunctionType = Result(Args...)const volatile&;
			using ToFunctionPointerType = Result(*)(Args...) ;
			template<class U>
			using ToMemberFunctionPointerType = Result(U::*)(Args...) const volatile&;
		};

		template<class Result, class... Args>
		struct FunctionSignature<Result(Args...)&> : nox::detail::FunctionSignatureFunction<Result, Args...>
		{
			static constexpr bool is_const = false;
			static constexpr bool is_volatile = false;
			static constexpr bool is_noexcept = false;
			static constexpr bool is_lvalue_reference = true;
			static constexpr bool is_rvalue_reference = false;

			using ToFunctionType = Result(Args...)&;
			using ToFunctionPointerType = Result(*)(Args...);
			template<class U>
			using ToMemberFunctionPointerType = Result(U::*)(Args...)&;
		};

		template<class Result, class... Args>
		struct FunctionSignature<Result(Args...)&&> : nox::detail::FunctionSignatureFunction<Result, Args...>
		{
			static constexpr bool is_const = false;
			static constexpr bool is_volatile = false;
			static constexpr bool is_noexcept = false;
			static constexpr bool is_lvalue_reference = false;
			static constexpr bool is_rvalue_reference = true;

			using ToFunctionType = Result(Args...)&&;
			using ToFunctionPointerType = Result(*)(Args...);
			template<class U>
			using ToMemberFunctionPointerType = Result(U::*)(Args...)&&;
		};

		template<class Result, class... Args>
		struct FunctionSignature<Result(Args...) const&> : nox::detail::FunctionSignatureFunction<Result, Args...>
		{
			static constexpr bool is_const = true;
			static constexpr bool is_volatile = false;
			static constexpr bool is_noexcept = false;
			static constexpr bool is_lvalue_reference = true;
			static constexpr bool is_rvalue_reference = false;

			using ToFunctionType = Result(Args...) const&;
			using ToFunctionPointerType = Result(*)(Args...);
			template<class U>
			using ToMemberFunctionPointerType = Result(U::*)(Args...) const&;
		};

		template<class Result, class... Args>
		struct FunctionSignature<Result(Args...) volatile&> : nox::detail::FunctionSignatureFunction<Result, Args...>
		{
			static constexpr bool is_const = false;
			static constexpr bool is_volatile = true;
			static constexpr bool is_noexcept = false;
			static constexpr bool is_lvalue_reference = true;
			static constexpr bool is_rvalue_reference = false;

			using ToFunctionType = Result(Args...) volatile&;
			using ToFunctionPointerType = Result(*)(Args...);
			template<class U>
			using ToMemberFunctionPointerType = Result(U::*)(Args...) volatile&;
		};

		template<class Result, class... Args>
		struct FunctionSignature<Result(Args...) & noexcept> : nox::detail::FunctionSignatureFunction<Result, Args...>
		{
			static constexpr bool is_const = false;
			static constexpr bool is_volatile = false;
			static constexpr bool is_noexcept = true;
			static constexpr bool is_lvalue_reference = true;
			static constexpr bool is_rvalue_reference = false;

			using ToFunctionType = Result(Args...) & noexcept;
			using ToFunctionPointerType = Result(*)(Args...) noexcept;
			template<class U>
			using ToMemberFunctionPointerType = Result(U::*)(Args...) & noexcept;
		};
		
		template<class Result, class... Args>
		struct FunctionSignature<Result(Args...) volatile& noexcept> : nox::detail::FunctionSignatureFunction<Result, Args...>
		{
			static constexpr bool is_const = false;
			static constexpr bool is_volatile = true;
			static constexpr bool is_noexcept = true;
			static constexpr bool is_lvalue_reference = true;
			static constexpr bool is_rvalue_reference = false;

			using ToFunctionType = Result(Args...) volatile& noexcept;
			using ToFunctionPointerType = Result(*)(Args...) noexcept;
			template<class U>
			using ToMemberFunctionPointerType = Result(U::*)(Args...) volatile& noexcept;
		};

		template<class Result, class... Args>
		struct FunctionSignature<Result(Args...) const&&> : nox::detail::FunctionSignatureFunction<Result, Args...>
		{
			static constexpr bool is_const = true;
			static constexpr bool is_volatile = false;
			static constexpr bool is_noexcept = false;
			static constexpr bool is_lvalue_reference = false;
			static constexpr bool is_rvalue_reference = true;

			using ToFunctionType = Result(Args...) const&&;
			using ToFunctionPointerType = Result(*)(Args...);
			template<class U>
			using ToMemberFunctionPointerType = Result(U::*)(Args...) const&&;
		};

		template<class Result, class... Args>
		struct FunctionSignature<Result(Args...) volatile&&> : nox::detail::FunctionSignatureFunction<Result, Args...>
		{
			static constexpr bool is_const = false;
			static constexpr bool is_volatile = true;
			static constexpr bool is_noexcept = false;
			static constexpr bool is_lvalue_reference = false;
			static constexpr bool is_rvalue_reference = true;

			using ToFunctionType = Result(Args...) volatile&&;
			using ToFunctionPointerType = Result(*)(Args...);
			template<class U>
			using ToMemberFunctionPointerType = Result(U::*)(Args...) volatile&&;
		};

		template<class Result, class... Args>
		struct FunctionSignature<Result(Args...) const&& noexcept> : nox::detail::FunctionSignatureFunction<Result, Args...>
		{
			static constexpr bool is_const = true;
			static constexpr bool is_volatile = false;
			static constexpr bool is_noexcept = true;
			static constexpr bool is_lvalue_reference = false;
			static constexpr bool is_rvalue_reference = true;

			using ToFunctionType = Result(Args...) const&& noexcept;
			using ToFunctionPointerType = Result(*)(Args...) noexcept;
			template<class U>
			using ToMemberFunctionPointerType = Result(U::*)(Args...) const&& noexcept;
		};

		template<class Result, class... Args>
		struct FunctionSignature<Result(Args...) volatile&& noexcept> : nox::detail::FunctionSignatureFunction<Result, Args...>
		{
			static constexpr bool is_const = false;
			static constexpr bool is_volatile = true;
			static constexpr bool is_noexcept = true;
			static constexpr bool is_lvalue_reference = false;
			static constexpr bool is_rvalue_reference = true;

			using ToFunctionType = Result(Args...) volatile&& noexcept;
			using ToFunctionPointerType = Result(*)(Args...) noexcept;
			template<class U>
			using ToMemberFunctionPointerType = Result(U::*)(Args...) volatile&& noexcept;
		};
		
		template<class Result, class... Args>
		struct FunctionSignature<Result(Args...) const volatile&&> : nox::detail::FunctionSignatureFunction<Result, Args...>
		{
			static constexpr bool is_const = true;
			static constexpr bool is_volatile = true;
			static constexpr bool is_noexcept = false;
			static constexpr bool is_lvalue_reference = false;
			static constexpr bool is_rvalue_reference = true;

			using ToFunctionType = Result(Args...) volatile&& noexcept;
			using ToFunctionPointerType = Result(*)(Args...) noexcept;
			template<class U>
			using ToMemberFunctionPointerType = Result(U::*)(Args...) volatile&& noexcept;
		};
#pragma endregion

#pragma region 関数ポインタ型

		template<class Result, class... Args>
		struct FunctionSignature<Result(*)(Args...)> : nox::detail::FunctionSignatureFunctionPointer<Result, Args...>
		{
			static constexpr bool is_noexcept = false;

			using ToFunctionType = Result(Args...);
			using ToFunctionPointerType = Result(*)(Args...);
			template<class U>
			using ToMemberFunctionPointerType = Result(U::*)(Args...);
		};

		template<class Result, class... Args>
		struct FunctionSignature<Result(*)(Args...)noexcept> : nox::detail::FunctionSignatureFunctionPointer<Result, Args...>
		{
			static constexpr bool is_noexcept = true;

			using ToFunctionType = Result(Args...)noexcept;
			using ToFunctionPointerType = Result(*)(Args...)noexcept;
			template<class U>
			using ToMemberFunctionPointerType = Result(U::*)(Args...)noexcept;
		};
#pragma endregion

#pragma region メンバ関数ポインタ型

		template<class ClassType, class Result, class... Args>
		struct FunctionSignature<Result(ClassType::*)(Args...)> : nox::detail::FunctionSignatureMemberFunctionPointer<ClassType, Result, Args...>
		{
			static constexpr bool is_const = false;
			static constexpr bool is_volatile = false;
			static constexpr bool is_noexcept = false;
			static constexpr bool is_lvalue_reference = false;
			static constexpr bool is_rvalue_reference = false;
		};

		template<class ClassType, class Result, class... Args>
		struct FunctionSignature<Result(ClassType::*)(Args...) const> : nox::detail::FunctionSignatureMemberFunctionPointer<ClassType, Result, Args...>
		{
			static constexpr bool is_const = true;
			static constexpr bool is_volatile = false;
			static constexpr bool is_noexcept = false;
			static constexpr bool is_lvalue_reference = false;
			static constexpr bool is_rvalue_reference = false;
		};

		template<class ClassType, class Result, class... Args>
		struct FunctionSignature<Result(ClassType::*)(Args...) volatile> : nox::detail::FunctionSignatureMemberFunctionPointer<ClassType, Result, Args...>
		{
			static constexpr bool is_const = false;
			static constexpr bool is_volatile = true;
			static constexpr bool is_noexcept = false;
			static constexpr bool is_lvalue_reference = false;
			static constexpr bool is_rvalue_reference = false;
		};

		template<class ClassType, class Result, class... Args>
		struct FunctionSignature<Result(ClassType::*)(Args...) noexcept> : nox::detail::FunctionSignatureMemberFunctionPointer<ClassType, Result, Args...>
		{
			static constexpr bool is_const = false;
			static constexpr bool is_volatile = false;
			static constexpr bool is_noexcept = true;
			static constexpr bool is_lvalue_reference = false;
			static constexpr bool is_rvalue_reference = false;
		};

		template<class ClassType, class Result, class... Args>
		struct FunctionSignature<Result(ClassType::*)(Args...) const noexcept> : nox::detail::FunctionSignatureMemberFunctionPointer<ClassType, Result, Args...>
		{
			static constexpr bool is_const = true;
			static constexpr bool is_volatile = false;
			static constexpr bool is_noexcept = true;
			static constexpr bool is_lvalue_reference = false;
			static constexpr bool is_rvalue_reference = false;
		};
		
		template<class ClassType, class Result, class... Args>
		struct FunctionSignature<Result(ClassType::*)(Args...) const volatile> : nox::detail::FunctionSignatureMemberFunctionPointer<ClassType, Result, Args...>
		{
			static constexpr bool is_const = true;
			static constexpr bool is_volatile = true;
			static constexpr bool is_noexcept = false;
			static constexpr bool is_lvalue_reference = false;
			static constexpr bool is_rvalue_reference = false;
		};
		
		template<class ClassType, class Result, class... Args>
		struct FunctionSignature<Result(ClassType::*)(Args...) volatile noexcept> : nox::detail::FunctionSignatureMemberFunctionPointer<ClassType, Result, Args...>
		{
			static constexpr bool is_const = false;
			static constexpr bool is_volatile = true;
			static constexpr bool is_noexcept = true;
			static constexpr bool is_lvalue_reference = false;
			static constexpr bool is_rvalue_reference = false;
		};

		template<class ClassType, class Result, class... Args>
		struct FunctionSignature<Result(ClassType::*)(Args...) const volatile noexcept> : nox::detail::FunctionSignatureMemberFunctionPointer<ClassType, Result, Args...>
		{
			static constexpr bool is_const = true;
			static constexpr bool is_volatile = true;
			static constexpr bool is_noexcept = true;
			static constexpr bool is_lvalue_reference = false;
			static constexpr bool is_rvalue_reference = false;
		};

		template<class ClassType, class Result, class... Args>
		struct FunctionSignature<Result(ClassType::*)(Args...)&> : nox::detail::FunctionSignatureMemberFunctionPointer<ClassType, Result, Args...>
		{
			static constexpr bool is_const = false;
			static constexpr bool is_volatile = false;
			static constexpr bool is_noexcept = false;
			static constexpr bool is_lvalue_reference = true;
			static constexpr bool is_rvalue_reference = false;
		};

		template<class ClassType, class Result, class... Args>
		struct FunctionSignature<Result(ClassType::*)(Args...)&&> : nox::detail::FunctionSignatureMemberFunctionPointer<ClassType, Result, Args...>
		{
			static constexpr bool is_const = false;
			static constexpr bool is_volatile = false;
			static constexpr bool is_noexcept = false;
			static constexpr bool is_lvalue_reference = false;
			static constexpr bool is_rvalue_reference = true;
		};

		template<class ClassType, class Result, class... Args>
		struct FunctionSignature<Result(ClassType::*)(Args...) const&> : nox::detail::FunctionSignatureMemberFunctionPointer<ClassType, Result, Args...>
		{
			static constexpr bool is_const = true;
			static constexpr bool is_volatile = false;
			static constexpr bool is_noexcept = false;
			static constexpr bool is_lvalue_reference = true;
			static constexpr bool is_rvalue_reference = false;
		};

		template<class ClassType, class Result, class... Args>
		struct FunctionSignature<Result(ClassType::*)(Args...) volatile&> : nox::detail::FunctionSignatureMemberFunctionPointer<ClassType, Result, Args...>
		{
			static constexpr bool is_const = false;
			static constexpr bool is_volatile = true;
			static constexpr bool is_noexcept = false;
			static constexpr bool is_lvalue_reference = true;
			static constexpr bool is_rvalue_reference = false;
		};

		template<class ClassType, class Result, class... Args>
		struct FunctionSignature<Result(ClassType::*)(Args...) & noexcept> : nox::detail::FunctionSignatureMemberFunctionPointer<ClassType, Result, Args...>
		{
			static constexpr bool is_const = false;
			static constexpr bool is_volatile = false;
			static constexpr bool is_noexcept = true;
			static constexpr bool is_lvalue_reference = true;
			static constexpr bool is_rvalue_reference = false;
		};

		template<class ClassType, class Result, class... Args>
		struct FunctionSignature<Result(ClassType::*)(Args...)const & noexcept> : nox::detail::FunctionSignatureMemberFunctionPointer<ClassType, Result, Args...>
		{
			static constexpr bool is_const = true;
			static constexpr bool is_volatile = false;
			static constexpr bool is_noexcept = true;
			static constexpr bool is_lvalue_reference = true;
			static constexpr bool is_rvalue_reference = false;
		};
		
		template<class ClassType, class Result, class... Args>
		struct FunctionSignature<Result(ClassType::*)(Args...)const volatile&> : nox::detail::FunctionSignatureMemberFunctionPointer<ClassType, Result, Args...>
		{
			static constexpr bool is_const = true;
			static constexpr bool is_volatile = true;
			static constexpr bool is_noexcept = false;
			static constexpr bool is_lvalue_reference = true;
			static constexpr bool is_rvalue_reference = false;
		};	
		
		template<class ClassType, class Result, class... Args>
		struct FunctionSignature<Result(ClassType::*)(Args...)volatile& noexcept> : nox::detail::FunctionSignatureMemberFunctionPointer<ClassType, Result, Args...>
		{
			static constexpr bool is_const = false;
			static constexpr bool is_volatile = true;
			static constexpr bool is_noexcept = true;
			static constexpr bool is_lvalue_reference = true;
			static constexpr bool is_rvalue_reference = false;
		};	
		
		template<class ClassType, class Result, class... Args>
		struct FunctionSignature<Result(ClassType::*)(Args...)const volatile& noexcept> : nox::detail::FunctionSignatureMemberFunctionPointer<ClassType, Result, Args...>
		{
			static constexpr bool is_const = true;
			static constexpr bool is_volatile = true;
			static constexpr bool is_noexcept = true;
			static constexpr bool is_lvalue_reference = true;
			static constexpr bool is_rvalue_reference = false;
		};

		template<class ClassType, class Result, class... Args>
		struct FunctionSignature<Result(ClassType::*)(Args...) const&&> : nox::detail::FunctionSignatureMemberFunctionPointer<ClassType, Result, Args...>
		{
			static constexpr bool is_const = true;
			static constexpr bool is_volatile = false;
			static constexpr bool is_noexcept = false;
			static constexpr bool is_lvalue_reference = false;
			static constexpr bool is_rvalue_reference = true;
		};

		template<class ClassType, class Result, class... Args>
		struct FunctionSignature<Result(ClassType::*)(Args...) volatile&&> : nox::detail::FunctionSignatureMemberFunctionPointer<ClassType, Result, Args...>
		{
			static constexpr bool is_const = false;
			static constexpr bool is_volatile = true;
			static constexpr bool is_noexcept = false;
			static constexpr bool is_lvalue_reference = false;
			static constexpr bool is_rvalue_reference = true;
		};
		
		template<class ClassType, class Result, class... Args>
		struct FunctionSignature<Result(ClassType::*)(Args...) const volatile&&> : nox::detail::FunctionSignatureMemberFunctionPointer<ClassType, Result, Args...>
		{
			static constexpr bool is_const = true;
			static constexpr bool is_volatile = true;
			static constexpr bool is_noexcept = false;
			static constexpr bool is_lvalue_reference = false;
			static constexpr bool is_rvalue_reference = true;
		};

		template<class ClassType, class Result, class... Args>
		struct FunctionSignature<Result(ClassType::*)(Args...) const&& noexcept> : nox::detail::FunctionSignatureMemberFunctionPointer<ClassType, Result, Args...>
		{
			static constexpr bool is_const = true;
			static constexpr bool is_volatile = false;
			static constexpr bool is_noexcept = true;
			static constexpr bool is_lvalue_reference = false;
			static constexpr bool is_rvalue_reference = true;
		};

		template<class ClassType, class Result, class... Args>
		struct FunctionSignature<Result(ClassType::*)(Args...) volatile&& noexcept> : nox::detail::FunctionSignatureMemberFunctionPointer<ClassType, Result, Args...>
		{
			static constexpr bool is_const = false;
			static constexpr bool is_volatile = true;
			static constexpr bool is_noexcept = true;
			static constexpr bool is_lvalue_reference = false;
			static constexpr bool is_rvalue_reference = true;
		};
		
		template<class ClassType, class Result, class... Args>
		struct FunctionSignature<Result(ClassType::*)(Args...)const volatile&& noexcept> : nox::detail::FunctionSignatureMemberFunctionPointer<ClassType, Result, Args...>
		{
			static constexpr bool is_const = true;
			static constexpr bool is_volatile = true;
			static constexpr bool is_noexcept = true;
			static constexpr bool is_lvalue_reference = false;
			static constexpr bool is_rvalue_reference = true;
		};


#pragma endregion

		
	}

	namespace concepts
	{
		/// @brief 解釈可能な関数型か
		template<class T>
		concept FunctionSignatureType = sizeof(nox::detail::FunctionSignatureAdapter<T>) >= 0;
	}

	/// @brief 関数の戻り値の型
	template<concepts::FunctionSignatureType T>
	using function_result_t = typename nox::detail::FunctionSignatureAdapter<T>::ResultType;

	/// @brief 修飾子情報を全て外した関数型
	template<concepts::FunctionSignatureType T>
	using function_raw_t = typename nox::detail::FunctionSignatureAdapter<T>::RawFunctionType;

	/// @brief メンバ関数を持つクラスの型
	template<class T> requires(std::is_member_function_pointer_v<T>)
	using function_class_t = typename nox::detail::FunctionSignatureAdapter<T>::ClassType;

	template<class T>
	using to_function_pointer_t = typename nox::detail::FunctionSignatureAdapter<T>::ToFunctionPointerType;

	template<class T, class ClassType> requires(!std::is_member_object_pointer_v<T> && (std::is_class_v<ClassType> || std::is_union_v<ClassType>))
	using to_member_function_pointer_t = typename nox::detail::FunctionSignatureAdapter<T>::template ToMemberFunctionPointerType<ClassType>;

	/// @brief 引数の型tuple
	template<concepts::FunctionSignatureType T>
	using function_args_tuple_t = typename nox::detail::FunctionSignatureAdapter<T>::ArgsTupleType;

	template<concepts::FunctionSignatureType T>
	constexpr size_t function_args_length_v = std::tuple_size_v<function_args_tuple_t<T>>;

	template<concepts::FunctionSignatureType T>
	constexpr bool is_function_noexcept_v = nox::detail::FunctionSignatureAdapter<T>::is_noexcept;

	template<concepts::FunctionSignatureType T>
	constexpr bool is_function_const_v = nox::detail::FunctionSignatureAdapter<T>::is_const;

	template<concepts::FunctionSignatureType T>
	constexpr bool is_function_lvalue_reference_v = nox::detail::FunctionSignatureAdapter<T>::is_lvalue_reference;
	
	template<concepts::FunctionSignatureType T>
	constexpr bool is_function_rvalue_reference_v = nox::detail::FunctionSignatureAdapter<T>::is_rvalue_reference;

	template<concepts::FunctionSignatureType T>
	constexpr bool is_function_volatile_v = nox::detail::FunctionSignatureAdapter<T>::is_volatile;
}
#pragma warning(pop)