//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	database.h
///	@brief	型情報データベース
#pragma once
#include	"type.h"

namespace nox::reflection
{
	//	前方宣言
	class ClassInfo;
	class EnumInfo;
	class VariableInfo;
	class FunctionInfo;

	/// @brief 初期化
	void Initialize();

	/// @brief 終了処理
	void Finalize();
	
#pragma region 検索
	//	クラスの取得
	const nox::reflection::ClassInfo* FindClassInfo(const nox::reflection::Type& type)noexcept;
	const nox::reflection::ClassInfo* FindClassInfo(std::uint32_t namehash)noexcept;
	inline const nox::reflection::ClassInfo* FindClassInfo(std::u32string_view fullName)noexcept { return nox::reflection::FindClassInfo(nox::util::Crc32(fullName)); }

	template<nox::concepts::ClassOrUnion T>
	inline const nox::reflection::ClassInfo* FindClassInfo()noexcept {
		return FindClassInfo(nox::reflection::Typeof<T>());
	}

	//	列挙体の取得
	const nox::reflection::EnumInfo* FindEnumInfo(const nox::reflection::Type& type)noexcept;
	template<nox::concepts::Enum T>
	inline const nox::reflection::EnumInfo* FindEnumInfo()noexcept
	{
		return FindEnumInfo(nox::reflection::Typeof<T>());
	}
	const nox::reflection::EnumInfo* FindEnumInfo(const std::uint32_t artiifact_name_hash, const nox::reflection::Type& type)noexcept;
	template<nox::concepts::Enum T>
	inline const nox::reflection::EnumInfo* FindEnumInfo(const std::uint32_t artiifact_name_hash)noexcept
	{
		return FindEnumInfo(artiifact_name_hash, nox::reflection::Typeof<T>());
	}
	
	//	関数の取得
	const nox::reflection::FunctionInfo* FindFunctionInfo(const nox::FunctionPointerId& id)noexcept;
	const nox::reflection::FunctionInfo* FindFunctionInfoWithNameHash(const std::uint32_t name_hash)noexcept;
	inline const nox::reflection::FunctionInfo* FindFunctionInfo(std::u32string_view full_name)noexcept
	{
		return nox::reflection::FindFunctionInfoWithNameHash(::nox::util::Crc32(full_name));
	}
	template<auto Func>// requires(nox::concepts::EveryFunctionType<decltype(Func)>
	inline const nox::reflection::FunctionInfo* FindFunctionInfo()noexcept
	{
		::nox::GetFunctionPointerId<Func>();
		return nullptr;
//		return FindFunctionInfo(::nox::GetFunctionPointerID<Func>());
	}

	//	変数の取得
	const nox::reflection::VariableInfo* FindVariableInfo(const nox::ObjectPointerId& id)noexcept;
	const nox::reflection::VariableInfo* FindVariableInfoWithNameHash(const std::uint32_t name_hash)noexcept;
	inline const nox::reflection::VariableInfo* FindVariableInfo(std::u32string_view full_name)noexcept
	{
		return nox::reflection::FindVariableInfoWithNameHash(nox::util::Crc32(full_name));
	}

	template<auto Ptr> requires(std::is_pointer_v<decltype(Ptr)>)
	inline const nox::reflection::VariableInfo* FindVariableInfo()noexcept
	{
	//	return FindFunctionInfo(::nox::GetObjectPointerID<Ptr>());
		return nullptr;
	}

#pragma endregion

#pragma region Utility
	bool IsBaseOf(const nox::reflection::ClassInfo& base, const nox::reflection::ClassInfo& derived);

	inline bool IsBaseOf(const nox::reflection::Type& baseType, const nox::reflection::Type& derivedType)
	{
		if (baseType.IsClass() == false || derivedType.IsClass() == false)
		{
			return false;
		}

		const nox::reflection::ClassInfo* const base = nox::reflection::FindClassInfo(baseType);
		const nox::reflection::ClassInfo* const derived = nox::reflection::FindClassInfo(derivedType);

		if (base == nullptr || derived == nullptr)
		{
			return false;
		}

		return IsBaseOf(*base, *derived);
	}

	
#pragma endregion

#pragma region 登録処理
	void Register(const std::uint32_t artifact_name_hash, const nox::reflection::ClassInfo& data);
	void Unregister(const std::uint32_t artifact_name_hash, const nox::reflection::ClassInfo& data);

	void Register(const std::uint32_t artifact_name_hash, const nox::reflection::EnumInfo& data);
	void Unregister(const std::uint32_t artifact_name_hash, const nox::reflection::EnumInfo& data);

	void Register(const std::uint32_t artifact_name_hash, const nox::reflection::VariableInfo& data);
	void Unregister(const std::uint32_t artifact_name_hash, const nox::reflection::VariableInfo& data);

	void Register(const std::uint32_t artifact_name_hash, const nox::reflection::FunctionInfo& data);
	void Unregister(const std::uint32_t artifact_name_hash, const nox::reflection::FunctionInfo& data);

#pragma endregion

}