///	@file	manager.h
///	@brief	manager
#pragma once
#include	"definition.h"
#include	"type.h"

namespace nox::reflection
{
	class Reflection : public nox::ISingleton<Reflection>
	{
		friend class nox::ISingleton<Reflection>;
	private:
		/// @brief クラスデータ
		struct ClassData
		{
			const class UserDefinedCompoundTypeInfo* class_info_ptr = nullptr;
			ClassData* next_ptr = nullptr;
			const ClassData* prev_ptr = nullptr;
			ClassData* child_ptr = nullptr;
		};

		

		/// @brief 名前空間に所属する情報
		struct GlobalData
		{
			nox::UnorderedMap<std::uint32_t, const class UserDefinedCompoundTypeInfo*> class_info_ptr_map;
			nox::UnorderedMap<std::uint32_t, const class VariableInfo*> field_info_ptr_map;
			nox::UnorderedMap<std::uint32_t, const class FunctionInfo*> function_info_ptr_map;
			nox::UnorderedMap<std::uint32_t, const class EnumInfo*> enum_info_ptr_map;
		};

	/*	struct ModuleData
		{
			nox::UnorderedMap<std::uint32_t, GlobalData>
		};*/
	public:
		
		const class UserDefinedCompoundTypeInfo* FindUserDefinedCompoundTypeInfo(const nox::reflection::Type& type)const noexcept;
		const class UserDefinedCompoundTypeInfo* FindUserDefinedCompoundTypeInfoFromNameHash(std::uint32_t namehash)const noexcept;
		const class UserDefinedCompoundTypeInfo* FindUserDefinedCompoundTypeInfoFromNameHashFromName(ReflectionStringView fullName)const noexcept { return FindUserDefinedCompoundTypeInfoFromNameHash(nox::util::Crc32(fullName)); }

		template<nox::concepts::UserDefinedCompoundType T>
		inline const class UserDefinedCompoundTypeInfo* FindUserDefinedCompoundTypeInfo()const noexcept {
			return FindUserDefinedCompoundTypeInfo(nox::reflection::Typeof<T>());
		}

#pragma region 登録関数
		void Register(const class UserDefinedCompoundTypeInfo& data);
		void Unregister(const class UserDefinedCompoundTypeInfo& data);

		void Register(const class EnumInfo& data);
		void Unregister(const class EnumInfo& data);

		void Register(const class VariableInfo& data);
		void Unregister(const class VariableInfo& data);

		void Register(const class FunctionInfo& data);
		void Unregister(const class FunctionInfo& data);
#pragma endregion
	private:
		inline Reflection() {}

	private:

		/// @brief		クラスデータマップ
		/// @details	key: typeID, value: class data
		UnorderedMap<std::uint32_t, ClassData> class_data_map_;

		/// @brief		クラスアドレスデータマップ
		///	@details	key: fullname, value: class data pointer
		UnorderedMap<std::uint32_t, const ClassData*> class_data_ptr_map_with_name_;

		/// @brief		列挙体ポインタマップ
		/// @details	key: typeID, value: enum info pointer
		UnorderedMap<std::uint32_t, const class EnumInfo*> enum_info_map_;

		/// @brief		列挙体ポインタマップ
		/// @details	key: nameHash, value: enum info pointer
		UnorderedMap<std::uint32_t, const class EnumInfo*> enum_info_map_with_name_;

		/// @brief		グローバルデータマップ
		///	@details	key: namespace hash, global data
		UnorderedMap<std::uint32_t, GlobalData> global_data_map_;
	};
}