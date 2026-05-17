//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	object_definition.h
///	@brief	object_definition
#pragma once

namespace nox
{
	namespace detail
	{
		template<class T, class... Args> requires( std::is_constructible_v<T, Args...>)
		inline constexpr T& ObjectCtor(Args&&... args)noexcept(std::is_nothrow_constructible_v<T, Args...>)
		{
			return *new T(std::forward<Args>(args)...);
		}

		template<class T, class... Args>
		inline constexpr T& ObjectCtor(Args&&...)noexcept
		{
			NOX_ASSERT(false, u"ここには来ないはず");
			T*const dummy = nullptr;
			return *dummy;
		}
	}
}

///@brief	基底オブジェクトの定義
#define	NOX_DECLARE_OBJECT_ROOT(ClassType) \
	NOX_DECLARE_REFLECTION_OBJECT(ClassType)
//	end define

///@brief	オブジェクトの定義
#define	NOX_DECLARE_OBJECT(ClassType, BaseType) \
	NOX_DECLARE_REFLECTION_OBJECT(ClassType); \
	public:\
		template<class... Args> requires(std::is_constructible_v<ClassType, Args...>)\
		inline static constexpr ClassType& Ctor(Args&&... args)noexcept(std::is_nothrow_constructible_v<ClassType, Args...>)\
		{\
			return ::nox::detail::ObjectCtor<ClassType>(std::forward<Args>(args)...);\
			static_assert(!std::is_same_v<BaseType, ClassType>, "base type failed");\
			static_assert(std::is_base_of_v<BaseType, ClassType>, "base type failed");\
		}\
		using Base = BaseType
//	end define