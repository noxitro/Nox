//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	object.h
///	@brief	object
#pragma once
#include	"object_definition.h"

namespace nox
{
	/// @brief 基底オブジェクト
	class Object : public ::nox::reflection::ReflectionObject
	{
		NOX_DECLARE_OBJECT_ROOT(Object);
	public:
		constexpr Object() noexcept {}
		virtual constexpr ~Object() override {}

		/// @brief 文字列化　動的メモリ確保
		/// @return 
		nox::U8String	ToString()const;

		/// @brief 文字列化　バッファ指定
		virtual ::nox::U8StringView	ToString(std::span<::nox::char8> dest_buffer)const;
	protected:
		inline	std::span<void(*)()> GetVTable()const noexcept { return ::nox::util::GetVTable(this); }
		/*bool	IsOverride(const nox::uint64 function_id, std::span<void(*)()> vtable)const noexcept;
		inline bool	IsOverride(const nox::uint64 function_id)const noexcept
		{
			return IsOverride(function_id, GetVTable());
		}

		template<nox::concepts::EveryFunctionType T>
		inline	bool	IsOverride(std::span<void(*)()> vtable)const noexcept
		{
			return IsOverride(nox::util::GetFunctionPointerID<T>(), vtable);
		}
		template<nox::concepts::EveryFunctionType T>
		inline	bool	IsOverride()const noexcept
		{
			return IsOverride<T>(GetVTable());
		}*/

	private:

	};
}