///	@file	function_signature.h
///	@brief	関数の特殊化
#pragma once

#include	"type_traits.h"
#include	<tuple>

namespace nox
{
	/// @brief 非公開
	namespace detail
	{
		/**
		 * @brief	シグネチャ取得ベース
		 * @details	戻り値型、引数型の情報から情報を設定する
		 * @tparam _FuncType 関数の型
		 * @tparam _ReturnType 戻り値の型
		 * @tparam ..._Args 引数群の型
		*/
		template<class _FuncType, class _ReturnType, class... _Args>
		struct FunctionSignatureBase
		{
			/**
			 * @brief 関数の型
			*/
			using FuncType = _FuncType;

			/**
			 * @brief 戻り値の型
			*/
			using RetType = _ReturnType;

			/**
			 * @brief 引数群の型(tuple型)
			*/
			using ArgsType = std::tuple<_Args...>;

			/**
			 * @brief
			*/
			template<template<class> class Factor>
			using ArgsTypeEx = std::tuple<Factor<_Args>...>;

			/**
			 * @brief 生の関数型(修飾子、クラス情報を削除した状態)
			*/
			using RawFuncType = std::decay_t<_ReturnType(_Args...)>;

			/**
			 * @brief インデックス指定の型
			*/
			template <size_t index>
			using ArgType = std::tuple_element_t<index, std::tuple<_Args...>>;

			/**
			 * @brief 引数の数を取得
			*/
			static	constexpr size_t ArgsLength()noexcept { return sizeof...(_Args); }

			/// @brief 呼び出し可能か
			template<class _F>
			static inline constexpr bool IsInvocable()noexcept { return std::is_invocable_v<_F, _Args...>; }

			/// @brief 戻り値の有無
			static inline consteval bool IsNoReturn()noexcept { return std::is_same_v<void, _ReturnType>; }
		private:

		};

		/**
		 * @brief グローバル関数用シグネチャ取得ベース
		 * @tparam _FuncType 関数の型
		 * @tparam _ReturnType 戻り値の型
		 * @tparam ..._Args 引数の型
		*/
		template<class _FuncType, class _ReturnType, class... _Args>
		struct GetSignatureGlobalFunc : public FunctionSignatureBase<_FuncType, _ReturnType, _Args...>
		{
			[[nodiscard]]	static	consteval	bool	IsMemberFunc()noexcept { return false; }
		};

		/**
		 * @brief メンバ関数用シグネチャ取得ベース
		 * @tparam _FuncType 関数の型
		 * @tparam _ClassType クラスの型
		 * @tparam _ReturnType 戻り値の型
		 * @tparam ..._Args 引数の型
		*/
		template<class _FuncType, class _ClassType, class _ReturnType, class... _Args>
		struct GetSignatureMemberFunc : public FunctionSignatureBase<_FuncType, _ReturnType, _Args...>
		{
			/**
			 * @brief クラス型
			*/
			using ClassType = _ClassType;

			/**
			 * @brief メンバ関数かどうか
			*/
			[[nodiscard]]	static	consteval	bool	IsMemberFunc()noexcept { return true; }
		};
	}

	/*!********************************************************************
	*
	*	@brief	シグネチャ取得
	*
	**********************************************************************/
	template<typename T>
	struct FunctionSignature;

	/*!********************************************************************
	*
	*	@brief	シグネチャ取得(グローバル関数)
	*
	**********************************************************************/
	template<class _ReturnType, class... _ArgsType>
	struct FunctionSignature<_ReturnType(_ArgsType...)> :
		public detail::GetSignatureGlobalFunc<_ReturnType(_ArgsType...), _ReturnType, _ArgsType...>
	{
		[[nodiscard]]	static	consteval	bool	IsConst()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsVolatile()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsLvalueRef()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsRvalueRef()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsNoexcept()noexcept { return false; }
	};

	/*!********************************************************************
	*
	*	@brief	シグネチャ取得(グローバル関数)
	*
	**********************************************************************/
	template<class _ReturnType, class... _ArgsType>
	struct FunctionSignature<_ReturnType(*)(_ArgsType...)> :
		public detail::GetSignatureGlobalFunc<_ReturnType(*)(_ArgsType...), _ReturnType, _ArgsType...>
	{
		[[nodiscard]]	static	consteval	bool	IsConst()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsVolatile()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsLvalueRef()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsRvalueRef()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsNoexcept()noexcept { return false; }
	};

	/*!********************************************************************
	*
	*	@brief	シグネチャ取得(noexceptグローバル関数)
	*
	**********************************************************************/
	template<class _ReturnType, class... _ArgsType>
	struct FunctionSignature<_ReturnType(_ArgsType...)noexcept> :
		public detail::GetSignatureGlobalFunc<_ReturnType(_ArgsType...)noexcept, _ReturnType, _ArgsType...>
	{
		[[nodiscard]]	static	consteval	bool	IsConst()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsVolatile()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsLvalueRef()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsRvalueRef()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsNoexcept()noexcept { return true; }
	};

	/*!********************************************************************
	*
	*	@brief	シグネチャ取得(noexceptグローバル関数)
	*
	**********************************************************************/
	template<class _ReturnType, class... _ArgsType>
	struct FunctionSignature<_ReturnType(*)(_ArgsType...)noexcept> :
		public detail::GetSignatureGlobalFunc<_ReturnType(*)(_ArgsType...)noexcept, _ReturnType, _ArgsType...>
	{
		[[nodiscard]]	static	consteval	bool	IsConst()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsVolatile()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsLvalueRef()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsRvalueRef()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsNoexcept()noexcept { return true; }
	};

	/*!********************************************************************
	*
	*	@brief	シグネチャ取得(メンバ関数)
	*
	**********************************************************************/
	template<class ClassType, class _ReturnType, class... _ArgsType>
	struct FunctionSignature<_ReturnType(ClassType::*)(_ArgsType...)> :
		public detail::GetSignatureMemberFunc<_ReturnType(ClassType::*)(_ArgsType...), ClassType, _ReturnType, _ArgsType...>
	{
		[[nodiscard]]	static	consteval	bool	IsConst()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsVolatile()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsLvalueRef()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsRvalueRef()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsNoexcept()noexcept { return false; }
	};

	/*!********************************************************************
	*
	*	@brief	シグネチャ取得(constメンバ関数)
	*
	**********************************************************************/
	template<class ClassType, class _ReturnType, class... _ArgsType>
	struct FunctionSignature<_ReturnType(ClassType::*)(_ArgsType...)const> :
		public detail::GetSignatureMemberFunc<_ReturnType(ClassType::*)(_ArgsType...)const, ClassType, _ReturnType, _ArgsType...>
	{
		[[nodiscard]]	static	consteval	bool	IsConst()noexcept { return true; }
		[[nodiscard]]	static	consteval	bool	IsVolatile()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsLvalueRef()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsRvalueRef()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsNoexcept()noexcept { return false; }
	};

	/*!********************************************************************
	*
	*	@brief	シグネチャ取得(volatile メンバ関数)
	*
	**********************************************************************/
	template<class ClassType, class _ReturnType, class... _ArgsType>
	struct FunctionSignature<_ReturnType(ClassType::*)(_ArgsType...)volatile> :
		public detail::GetSignatureMemberFunc<_ReturnType(ClassType::*)(_ArgsType...)volatile, ClassType, _ReturnType, _ArgsType...>
	{
		[[nodiscard]]	static	consteval	bool	IsConst()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsVolatile()noexcept { return true; }
		[[nodiscard]]	static	consteval	bool	IsLvalueRef()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsRvalueRef()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsNoexcept()noexcept { return false; }
	};

	/*!********************************************************************
	*
	*	@brief	シグネチャ取得(const volatile メンバ関数)
	*
	**********************************************************************/
	template<class ClassType, class _ReturnType, class... _ArgsType>
	struct FunctionSignature<_ReturnType(ClassType::*)(_ArgsType...)const volatile> :
		public detail::GetSignatureMemberFunc<_ReturnType(ClassType::*)(_ArgsType...)const volatile, ClassType, _ReturnType, _ArgsType...>
	{
		[[nodiscard]]	static	consteval	bool	IsConst()noexcept { return true; }
		[[nodiscard]]	static	consteval	bool	IsVolatile()noexcept { return true; }
		[[nodiscard]]	static	consteval	bool	IsLvalueRef()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsRvalueRef()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsNoexcept()noexcept { return false; }
	};

	/*!********************************************************************
	*
	*	@brief	シグネチャ取得(左辺参照 メンバ関数)
	*
	**********************************************************************/
	template<class ClassType, class _ReturnType, class... _ArgsType>
	struct FunctionSignature<_ReturnType(ClassType::*)(_ArgsType...)&> :
		public detail::GetSignatureMemberFunc<_ReturnType(ClassType::*)(_ArgsType...)&, ClassType, _ReturnType, _ArgsType...>
	{
		[[nodiscard]]	static	consteval	bool	IsConst()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsVolatile()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsLvalueRef()noexcept { return true; }
		[[nodiscard]]	static	consteval	bool	IsRvalueRef()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsNoexcept()noexcept { return false; }
	};

	/*!********************************************************************
	*
	*	@brief	シグネチャ取得(左辺参照 const メンバ関数)
	*
	**********************************************************************/
	template<class ClassType, class _ReturnType, class... _ArgsType>
	struct FunctionSignature<_ReturnType(ClassType::*)(_ArgsType...)const&> :
		public detail::GetSignatureMemberFunc<_ReturnType(ClassType::*)(_ArgsType...)const&, ClassType, _ReturnType, _ArgsType...>
	{
		[[nodiscard]]	static	consteval	bool	IsConst()noexcept { return true; }
		[[nodiscard]]	static	consteval	bool	IsVolatile()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsLvalueRef()noexcept { return true; }
		[[nodiscard]]	static	consteval	bool	IsRvalueRef()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsNoexcept()noexcept { return false; }
	};

	/*!********************************************************************
	*
	*	@brief	シグネチャ取得(左辺参照 volatile メンバ関数)
	*
	**********************************************************************/
	template<class ClassType, class _ReturnType, class... _ArgsType>
	struct FunctionSignature<_ReturnType(ClassType::*)(_ArgsType...)volatile&> :
		public detail::GetSignatureMemberFunc<_ReturnType(ClassType::*)(_ArgsType...)volatile&, ClassType, _ReturnType, _ArgsType...>
	{
		[[nodiscard]]	static	consteval	bool	IsConst()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsVolatile()noexcept { return true; }
		[[nodiscard]]	static	consteval	bool	IsLvalueRef()noexcept { return true; }
		[[nodiscard]]	static	consteval	bool	IsRvalueRef()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsNoexcept()noexcept { return false; }
	};

	/*!********************************************************************
	*
	*	@brief	シグネチャ取得(左辺参照 const volatile メンバ関数)
	*
	**********************************************************************/
	template<class ClassType, class _ReturnType, class... _ArgsType>
	struct FunctionSignature<_ReturnType(ClassType::*)(_ArgsType...)const volatile&> :
		public detail::GetSignatureMemberFunc<_ReturnType(ClassType::*)(_ArgsType...)const volatile&, ClassType, _ReturnType, _ArgsType...>
	{
		[[nodiscard]]	static	consteval	bool	IsConst()noexcept { return true; }
		[[nodiscard]]	static	consteval	bool	IsVolatile()noexcept { return true; }
		[[nodiscard]]	static	consteval	bool	IsLvalueRef()noexcept { return true; }
		[[nodiscard]]	static	consteval	bool	IsRvalueRef()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsNoexcept()noexcept { return false; }
	};

	/*!********************************************************************
	*
	*	@brief	シグネチャ取得(右辺参照 メンバ関数)
	*
	**********************************************************************/
	template<class ClassType, class _ReturnType, class... _ArgsType>
	struct FunctionSignature<_ReturnType(ClassType::*)(_ArgsType...)&&> :
		public detail::GetSignatureMemberFunc<_ReturnType(ClassType::*)(_ArgsType...)&&, ClassType, _ReturnType, _ArgsType...>
	{
		[[nodiscard]]	static	consteval	bool	IsConst()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsVolatile()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsLvalueRef()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsRvalueRef()noexcept { return true; }
		[[nodiscard]]	static	consteval	bool	IsNoexcept()noexcept { return false; }
	};

	/*!********************************************************************
	*
	*	@brief	シグネチャ取得(右辺参照 const メンバ関数)
	*
	**********************************************************************/
	template<class ClassType, class _ReturnType, class... _ArgsType>
	struct FunctionSignature<_ReturnType(ClassType::*)(_ArgsType...)const&&> :
		public detail::GetSignatureMemberFunc<_ReturnType(ClassType::*)(_ArgsType...)const&&, ClassType, _ReturnType, _ArgsType...>
	{
		[[nodiscard]]	static	consteval	bool	IsConst()noexcept { return true; }
		[[nodiscard]]	static	consteval	bool	IsVolatile()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsLvalueRef()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsRvalueRef()noexcept { return true; }
		[[nodiscard]]	static	consteval	bool	IsNoexcept()noexcept { return false; }
	};

	/*!********************************************************************
	*
	*	@brief	シグネチャ取得(右辺参照 const メンバ関数)
	*
	**********************************************************************/
	template<class ClassType, class _ReturnType, class... _ArgsType>
	struct FunctionSignature<_ReturnType(ClassType::*)(_ArgsType...)volatile&&> :
		public detail::GetSignatureMemberFunc<_ReturnType(ClassType::*)(_ArgsType...)volatile&&, ClassType, _ReturnType, _ArgsType...>
	{
		[[nodiscard]]	static	consteval	bool	IsConst()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsVolatile()noexcept { return true; }
		[[nodiscard]]	static	consteval	bool	IsLvalueRef()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsRvalueRef() noexcept { return true; }
		[[nodiscard]]	static	consteval	bool	IsNoexcept()noexcept { return false; }
	};

	/*!********************************************************************
	*
	*	@brief	シグネチャ取得(右辺参照 const volatile メンバ関数)
	*
	**********************************************************************/
	template<class ClassType, class _ReturnType, class... _ArgsType>
	struct FunctionSignature<_ReturnType(ClassType::*)(_ArgsType...)const volatile&&> :
		public detail::GetSignatureMemberFunc<_ReturnType(ClassType::*)(_ArgsType...)const volatile&, ClassType, _ReturnType, _ArgsType...>
	{
		[[nodiscard]]	static	consteval	bool	IsConst()noexcept { return true; }
		[[nodiscard]]	static	consteval	bool	IsVolatile()noexcept { return true; }
		[[nodiscard]]	static	consteval	bool	IsLvalueRef()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsRvalueRef()noexcept { return true; }
		[[nodiscard]]	static	consteval	bool	IsNoexcept()noexcept { return false; }
	};

	/*!********************************************************************
	*
	*	@brief	シグネチャ取得(noexceptメンバ関数)
	*
	**********************************************************************/
	template<class ClassType, class _ReturnType, class... _ArgsType>
	struct FunctionSignature<_ReturnType(ClassType::*)(_ArgsType...)noexcept> :
		public detail::GetSignatureMemberFunc<_ReturnType(ClassType::*)(_ArgsType...)noexcept, ClassType, _ReturnType, _ArgsType...>
	{
		[[nodiscard]]	static	consteval	bool	IsConst()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsVolatile()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsLvalueRef()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsRvalueRef()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsNoexcept()noexcept { return true; }
	};

	/*!********************************************************************
	*
	*	@brief	シグネチャ取得(const noexcept メンバ関数)
	*
	**********************************************************************/
	template<class ClassType, class _ReturnType, class... _ArgsType>
	struct FunctionSignature<_ReturnType(ClassType::*)(_ArgsType...)const noexcept> :
		public detail::GetSignatureMemberFunc<_ReturnType(ClassType::*)(_ArgsType...)const noexcept, ClassType, _ReturnType, _ArgsType...>
	{
		[[nodiscard]]	static	consteval	bool	IsConst()noexcept { return true; }
		[[nodiscard]]	static	consteval	bool	IsVolatile()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsLvalueRef()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsRvalueRef()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsNoexcept()noexcept { return true; }
	};

	/*!********************************************************************
	*
	*	@brief	シグネチャ取得(volatile noexcept メンバ関数)
	*
	**********************************************************************/
	template<class ClassType, class _ReturnType, class... _ArgsType>
	struct FunctionSignature<_ReturnType(ClassType::*)(_ArgsType...)volatile noexcept> :
		public detail::GetSignatureMemberFunc<_ReturnType(ClassType::*)(_ArgsType...)volatile noexcept, ClassType, _ReturnType, _ArgsType...>
	{
		[[nodiscard]]	static	consteval	bool	IsConst()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsVolatile()noexcept { return true; }
		[[nodiscard]]	static	consteval	bool	IsLvalueRef()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsRvalueRef()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsNoexcept()noexcept { return true; }
	};

	/*!********************************************************************
	*
	*	@brief	シグネチャ取得(const volatile noexcept メンバ関数)
	*
	**********************************************************************/
	template<class ClassType, class _ReturnType, class... _ArgsType>
	struct FunctionSignature<_ReturnType(ClassType::*)(_ArgsType...)const volatile noexcept> :
		public detail::GetSignatureMemberFunc<_ReturnType(ClassType::*)(_ArgsType...)const volatile noexcept, ClassType, _ReturnType, _ArgsType...>
	{
		[[nodiscard]]	static	consteval	bool	IsConst()noexcept { return true; }
		[[nodiscard]]	static	consteval	bool	IsVolatile()noexcept { return true; }
		[[nodiscard]]	static	consteval	bool	IsLvalueRef()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsRvalueRef()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsNoexcept()noexcept { return true; }
	};

	/*!********************************************************************
	*
	*	@brief	シグネチャ取得(左辺参照 noexcept メンバ関数)
	*
	**********************************************************************/
	template<class ClassType, class _ReturnType, class... _ArgsType>
	struct FunctionSignature<_ReturnType(ClassType::*)(_ArgsType...) & noexcept> :
		public detail::GetSignatureMemberFunc<_ReturnType(ClassType::*)(_ArgsType...) & noexcept, ClassType, _ReturnType, _ArgsType...>
	{
		[[nodiscard]]	static	consteval	bool	IsConst()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsVolatile()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsLvalueRef()noexcept { return true; }
		[[nodiscard]]	static	consteval	bool	IsRvalueRef()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsNoexcept()noexcept { return true; }
	};

	/*!********************************************************************
	*
	*	@brief	シグネチャ取得(左辺参照 const noexcept メンバ関数)
	*
	**********************************************************************/
	template<class ClassType, class _ReturnType, class... _ArgsType>
	struct FunctionSignature<_ReturnType(ClassType::*)(_ArgsType...)const& noexcept> :
		public detail::GetSignatureMemberFunc<_ReturnType(ClassType::*)(_ArgsType...)const& noexcept, ClassType, _ReturnType, _ArgsType...>
	{
		[[nodiscard]]	static	consteval	bool	IsConst()noexcept { return true; }
		[[nodiscard]]	static	consteval	bool	IsVolatile()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsLvalueRef()noexcept { return true; }
		[[nodiscard]]	static	consteval	bool	IsRvalueRef()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsNoexcept()noexcept { return true; }
	};

	/*!********************************************************************
	*
	*	@brief	シグネチャ取得(左辺参照 volatile noexcept メンバ関数)
	*
	**********************************************************************/
	template<class ClassType, class _ReturnType, class... _ArgsType>
	struct FunctionSignature<_ReturnType(ClassType::*)(_ArgsType...)volatile& noexcept> :
		public detail::GetSignatureMemberFunc<_ReturnType(ClassType::*)(_ArgsType...)volatile& noexcept, ClassType, _ReturnType, _ArgsType...>
	{
		[[nodiscard]]	static	consteval	bool	IsConst()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsVolatile()noexcept { return true; }
		[[nodiscard]]	static	consteval	bool	IsLvalueRef()noexcept { return true; }
		[[nodiscard]]	static	consteval	bool	IsRvalueRef()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsNoexcept()noexcept { return true; }
	};

	/*!********************************************************************
	*
	*	@brief	シグネチャ取得(左辺参照 const volatile noexcept メンバ関数)
	*
	**********************************************************************/
	template<class ClassType, class _ReturnType, class... _ArgsType>
	struct FunctionSignature<_ReturnType(ClassType::*)(_ArgsType...)const volatile& noexcept> :
		public detail::GetSignatureMemberFunc<_ReturnType(ClassType::*)(_ArgsType...)const volatile& noexcept, ClassType, _ReturnType, _ArgsType...>
	{
		[[nodiscard]]	static	consteval	bool	IsConst()noexcept { return true; }
		[[nodiscard]]	static	consteval	bool	IsVolatile()noexcept { return true; }
		[[nodiscard]]	static	consteval	bool	IsLvalueRef()noexcept { return true; }
		[[nodiscard]]	static	consteval	bool	IsRvalueRef()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsNoexcept()noexcept { return true; }
	};

	/*!********************************************************************
	*
	*	@brief	シグネチャ取得(右辺参照 noexcept メンバ関数)
	*
	**********************************************************************/
	template<class ClassType, class _ReturnType, class... _ArgsType>
	struct FunctionSignature<_ReturnType(ClassType::*)(_ArgsType...) && noexcept> :
		public detail::GetSignatureMemberFunc<_ReturnType(ClassType::*)(_ArgsType...) && noexcept, ClassType, _ReturnType, _ArgsType...>
	{
		[[nodiscard]]	static	consteval	bool	IsConst()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsVolatile()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsLvalueRef()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsRvalueRef()noexcept { return true; }
		[[nodiscard]]	static	consteval	bool	IsNoexcept()noexcept { return true; }
	};

	/*!********************************************************************
	*
	*	@brief	シグネチャ取得(右辺参照 const noexcept メンバ関数)
	*
	**********************************************************************/
	template<class ClassType, class _ReturnType, class... _ArgsType>
	struct FunctionSignature<_ReturnType(ClassType::*)(_ArgsType...)const&& noexcept> :
		public detail::GetSignatureMemberFunc<_ReturnType(ClassType::*)(_ArgsType...)const&& noexcept, ClassType, _ReturnType, _ArgsType...>
	{
		[[nodiscard]]	static	consteval	bool	IsConst()noexcept { return true; }
		[[nodiscard]]	static	consteval	bool	IsVolatile()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsLvalueRef()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsRvalueRef()noexcept { return true; }
		[[nodiscard]]	static	consteval	bool	IsNoexcept()noexcept { return true; }
	};

	/*!********************************************************************
	*
	*	@brief	シグネチャ取得(右辺参照 const noexcept メンバ関数)
	*
	**********************************************************************/
	template<class ClassType, class _ReturnType, class... _ArgsType>
	struct FunctionSignature<_ReturnType(ClassType::*)(_ArgsType...)volatile&& noexcept> :
		public detail::GetSignatureMemberFunc<_ReturnType(ClassType::*)(_ArgsType...)volatile&& noexcept, ClassType, _ReturnType, _ArgsType...>
	{
		[[nodiscard]]	static	consteval	bool	IsConst()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsVolatile()noexcept { return true; }
		[[nodiscard]]	static	consteval	bool	IsLvalueRef()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsRvalueRef() noexcept { return true; }
		[[nodiscard]]	static	consteval	bool	IsNoexcept()noexcept { return true; }
	};

	/*!********************************************************************
	*
	*	@brief	シグネチャ取得(右辺参照 const volatile noexcept メンバ関数)
	*
	**********************************************************************/
	template<class ClassType, class _ReturnType, class... _ArgsType>
	struct FunctionSignature<_ReturnType(ClassType::*)(_ArgsType...)const volatile&& noexcept> :
		public detail::GetSignatureMemberFunc<_ReturnType(ClassType::*)(_ArgsType...)const volatile&& noexcept, ClassType, _ReturnType, _ArgsType...>
	{
		[[nodiscard]]	static	consteval	bool	IsConst()noexcept { return true; }
		[[nodiscard]]	static	consteval	bool	IsVolatile()noexcept { return true; }
		[[nodiscard]]	static	consteval	bool	IsLvalueRef()noexcept { return false; }
		[[nodiscard]]	static	consteval	bool	IsRvalueRef()noexcept { return true; }
		[[nodiscard]]	static	consteval	bool	IsNoexcept()noexcept { return true; }
	};

	/*!********************************************************************
	*
	*	@brief	シグネチャ取得(ラムダ式)
	*
	**********************************************************************/
	template<class _Lambda> requires(IsLambdaValue< _Lambda>)
		struct FunctionSignature<_Lambda> : public FunctionSignature<LambdaToFunctionType<_Lambda>>
	{

	};

	namespace concepts
	{
		/**
		 * @brief 解釈可能な関数型か
		*/
		template<class T>
		concept FunctionSignatureType = sizeof(FunctionSignature<T>) >= 0;
	}

	/**
	 * @brief 関数の戻り値の型
	*/
	template<concepts::FunctionSignatureType T>
	using FunctionReturnType = typename FunctionSignature<T>::RetType;;

	/**
	 * @brief 修飾子情報を全て外した関数型
	*/
	template<concepts::FunctionSignatureType T>
	using FunctionRawType = typename FunctionSignature<T>::RawFuncType;

	/**
	 * @brief メンバ関数のクラス型
	*/
	template<class T> requires(std::is_member_function_pointer_v<T>)
	using FunctionClassType = typename FunctionSignature<T>::ClassType;

	/**
	 * @brief 引数の型tuple
	*/
	template<concepts::FunctionSignatureType T>
	using FunctionArgsType = typename FunctionSignature<T>::ArgsType;

	template<concepts::FunctionSignatureType T>
	constexpr size_t FunctionArgsLength = std::tuple_size_v<FunctionArgsType<T>>;

	template<concepts::FunctionSignatureType T>
	constexpr bool IsFunctionNoexceptValue = FunctionSignature<T>::IsNoexcept();

	template<concepts::FunctionSignatureType T>
	constexpr bool IsFunctionConstValue = FunctionSignature<T>::IsConst();

	template<concepts::FunctionSignatureType T>
	constexpr bool IsFunctionLvalueValue = FunctionSignature<T>::IsLvalueRef();
	
	template<concepts::FunctionSignatureType T>
	constexpr bool IsFunctionRvalueValue = FunctionSignature<T>::IsRvalueRef();

	template<concepts::FunctionSignatureType T>
	constexpr bool IsFunctionVolatileValue = FunctionSignature<T>::IsVolatile();
}