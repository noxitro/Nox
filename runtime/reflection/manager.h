///	@file	manager.h
///	@brief	manager
#pragma once

namespace nox::reflection
{
	class Reflection
	{
	private:
		struct ClassData
		{
			const class ClassInfo* class_info_ptr = nullptr;
			ClassData* next_ptr = nullptr;
			const ClassData* prev_ptr = nullptr;
			ClassData* child_ptr = nullptr;

		/*	inline constexpr explicit ClassData(const class ClassInfo* _classInfoPtr)noexcept:
				class_info_ptr(_classInfoPtr)
			{

			}*/
		};

		/// @brief 名前空間に所属する情報
		struct GlobalData
		{
			UnorderedMap<u32, const class ClassInfo*> class_info_ptr_map;
			const class GlobalInfo* global_into_ptr = nullptr;
		};
	public:
		
		const class ClassInfo* FindClassInfo(u32 typeID)const noexcept;
		const class ClassInfo* FindClassInfo(std::u8string_view fullName)const noexcept;

		template<concepts::ClassUnion T>
		inline const class ClassInfo* FindClassInfo()const noexcept {
			return FindClassInfo(util::GetUniqueTypeID<T>());
		}


		static	inline	void		CreateInstance() { instance_ = new Reflection(); }
		static	inline	void		DeleteInstance() { delete instance_; }
		static	inline	constexpr Reflection* Instance()noexcept { return instance_; }

#pragma region 登録関数
		void Register(const class ClassInfo& data);
		void Unregister(const class ClassInfo& data);

		void Register(const class EnumInfo& data);
		void Unregister(const class EnumInfo& data);
#pragma endregion


	private:
		static inline Reflection* instance_ = nullptr;

		/// @brief		クラスデータマップ
		/// @details	key: typeID, value: class data
		UnorderedMap<u32, ClassData> class_data_map_;

		/// @brief		クラスアドレスデータマップ
		///	@details	key: fullname, value: class data pointer
		UnorderedMap<std::u8string_view, const ClassData*> class_data_ptr_map_with_name_;

		/// @brief		列挙体ポインタマップ
		/// @details	key: typeID, value: enum info pointer
		UnorderedMap<u32, const class EnumInfo*> enum_info_map_;

		/// @brief		グローバルデータマップ
		///	@details	key: namespace hash, global data
		UnorderedMap<u32, GlobalData> global_data_map_;
	};
}