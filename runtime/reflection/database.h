//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	database.h
///	@brief	型情報データベース
#pragma once

namespace nox::reflection
{
	//	前方宣言
	class ClassInfo;
	class EnumInfo;
	class VariableInfo;
	class FunctionInfo;
	class ReflectionObject;

	/// @brief 初期化
	void Initialize();

	/// @brief 終了処理
	void Finalize();
	
#pragma region 検索
	//	クラスの取得
	const nox::reflection::ClassInfo* FindClassInfo(const nox::reflection::Type& type)noexcept;
	const nox::reflection::ClassInfo* FindClassInfo(std::uint32_t namehash)noexcept;
	inline const nox::reflection::ClassInfo* FindClassInfo(std::u8string_view fullName)noexcept { return nox::reflection::FindClassInfo(nox::util::Crc32(fullName)); }

	template<nox::concepts::ClassOrUnion T>
	inline const nox::reflection::ClassInfo* FindClassInfo()noexcept {
		return FindClassInfo(nox::reflection::Typeof<T>());
	}

	/// @brief 指定した型の派生クラスを列挙する
	/// @param type         基底クラスとして扱う型情報
	/// @param callback     列挙対象となったクラス情報ごとに呼び出されるコールバック
	/// @param include_self true の場合は基底クラス自身（type に対応するクラス）も最初に callback へ渡す
	/// @param recursive    true の場合は子孫クラス（孫以降の階層）まで再帰的に列挙する。false の場合は直下の派生クラスのみ
	NOX_ATTR(nox::reflection::attr::IgnoreReflection())
	void ForeachDerivedClassInfoList(const nox::reflection::Type& type, std::move_only_function<void(const nox::reflection::ClassInfo&)> callback, bool include_self = false, bool recursive = true);

	/// @brief 指定した型の派生クラスを列挙する
	/// @tparam T 基底クラスとして扱う型
	/// @param callback     列挙対象となったクラス情報ごとに呼び出されるコールバック
	/// @param include_self true の場合は基底クラス自身（type に対応するクラス）も最初に callback へ渡す
	/// @param recursive    true の場合は子孫クラス（孫以降の階層）まで再帰的に列挙する。false の場合は直下の派生クラスのみ

	template<class T> requires(nox::concepts::ClassOrUnion<T>)
	inline void ForeachDerivedClassInfoList(std::move_only_function<void(const nox::reflection::ClassInfo&)> callback, bool include_self = false, bool recursive = true)
	{
		nox::reflection::ForeachDerivedClassInfoList(nox::reflection::Typeof<T>(), std::move(callback), include_self, recursive);
	}

	template<class T>
	inline std::span<const nox::reflection::ClassInfo*> FindSubClassInfoList(std::span<const nox::reflection::ClassInfo*> dest, bool sublevel = true)
	{
		return {};
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
	inline const nox::reflection::FunctionInfo* FindFunctionInfo(std::u8string_view full_name)noexcept
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
	inline const nox::reflection::VariableInfo* FindVariableInfo(std::u8string_view full_name)noexcept
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
	bool IsBaseOf(const nox::reflection::ClassInfo& base, const nox::reflection::ClassInfo& derived)noexcept;
	
	inline bool IsBaseOf(const nox::reflection::Type& baseType, const nox::reflection::Type& derivedType)noexcept
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

	inline bool IsBaseOf(const nox::reflection::ClassInfo& base, const nox::reflection::Type& derivedType)noexcept
	{
		const nox::reflection::ClassInfo* const derived = nox::reflection::FindClassInfo(derivedType);

		if (derived == nullptr)
		{
			return false;
		}

		return IsBaseOf(base, *derived);
	}

	namespace detail
	{
		bool IsBaseOf(const nox::reflection::Type& base, const nox::reflection::ReflectionObject& from)noexcept;
	}

	template<class T>
		requires(std::derived_from<T, nox::reflection::ReflectionObject>)
	inline T* AsCast(nox::reflection::ReflectionObject& from) noexcept
	{
		if (nox::reflection::detail::IsBaseOf(nox::reflection::Typeof<T>(), from) == false)
		{
			return nullptr;
		}
		return static_cast<T*>(&from);
	}

	template<class T> requires (std::is_pointer_v<T>)
		inline T AsCast(nox::reflection::ReflectionObject* from) noexcept
	{
		if (from == nullptr)
		{
			return nullptr;
		}

		if (nox::reflection::detail::IsBaseOf(nox::reflection::Typeof<T>(), *from) == false)
		{
			return nullptr;
		}
		return reinterpret_cast<T>(from);
	}

	std::u8string_view GetEnumFullName(const nox::reflection::Type& type, nox::uint64 value)noexcept;
	template<nox::concepts::Enum T>
	inline std::u8string_view GetEnumFullName(T value)noexcept
	{
		return nox::reflection::GetEnumFullName(nox::reflection::Typeof<T>(), static_cast<nox::uint64>(value));
	}
#pragma endregion
}