///	@file	manager.h
///	@brief	manager
#pragma once
#include	"definition.h"

namespace nox::reflection
{
	class Reflection : public ISingleton<Reflection>
	{
		friend class ISingleton<Reflection>;
	private:
		/// @brief クラスデータ
		struct ClassData
		{
			const class ClassInfo* class_info_ptr = nullptr;
			ClassData* next_ptr = nullptr;
			const ClassData* prev_ptr = nullptr;
			ClassData* child_ptr = nullptr;
		};

		/// @brief 名前空間に所属する情報
		struct GlobalData
		{
			UnorderedMap<std::uint32_t, const class ClassInfo*> class_info_ptr_map;
			UnorderedMap<std::uint32_t, const class FieldInfo*> field_info_ptr_map;
			UnorderedMap<std::uint32_t, const class FunctionInfo*> function_info_ptr_map;
			UnorderedMap<std::uint32_t, const class EnumInfo*> enum_info_ptr_map;
		};
	public:
		
		const class ClassInfo* FindClassInfo(std::uint32_t typeID)const noexcept;
		const class ClassInfo* FindClassInfoFromNameHash(std::uint32_t namehash)const noexcept;
		const class ClassInfo* FindClassInfoFromName(ReflectionStringView fullName)const noexcept { return FindClassInfoFromNameHash(util::Crc32(fullName)); }

		template<concepts::ClassUnion T>
		inline const class ClassInfo* FindClassInfo()const noexcept {
			return FindClassInfo(util::GetUniqueTypeID<T>());
		}

		const class EnumInfo* FindEnumInfo(std::uint32_t typeID)const noexcept;
		const class EnumInfo* FindEnumInfoFromNameHash(std::uint32_t namehash)const noexcept;
		const class EnumInfo* FindEnumInfoFromName(ReflectionStringView fullName)const noexcept { return FindEnumInfoFromNameHash(util::Crc32(fullName)); }

#pragma region 登録関数
		void Register(const class ClassInfo& data);
		void Unregister(const class ClassInfo& data);

		void Register(const class EnumInfo& data);
		void Unregister(const class EnumInfo& data);

		void Register(const class FieldInfo& data);
		void Unregister(const class FieldInfo& data);

		void Register(const class FunctionInfo& data);
		void Unregister(const class FunctionInfo& data);
#pragma endregion
	private:
		inline Reflection() {}

	private:
		static inline Reflection* instance_ = nullptr;

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