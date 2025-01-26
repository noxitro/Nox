//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	database.cpp
///	@brief	database
#include	"stdafx.h"
#include	"database.h"

#include	"user_defined_compound_type_info.h"
#include	"enum_info.h"
#include	"variable_info.h"
#include	"function_info.h"

namespace
{
	/// @brief クラスデータ
	struct ClassNode
	{
		std::reference_wrapper<const nox::reflection::ClassInfo> class_info;
		ClassNode* next_ptr;
		const ClassNode* prev_ptr;
		ClassNode* child_ptr;

		inline constexpr ClassNode(const nox::reflection::ClassInfo& _class_info)noexcept :
			class_info(_class_info),
			next_ptr(nullptr),
			prev_ptr(nullptr),
			child_ptr(nullptr)
		{

		}

		inline constexpr ClassNode(const ClassNode& rhs)noexcept :
			class_info(rhs.class_info),
			next_ptr(rhs.next_ptr),
			prev_ptr(rhs.prev_ptr),
			child_ptr(rhs.child_ptr)
		{
		}

		inline ClassNode(const ClassNode&&)noexcept = delete;

		inline ClassNode& operator =(const ClassNode& rhs)noexcept
		{
			class_info = rhs.class_info;
			next_ptr = rhs.next_ptr;
			prev_ptr = rhs.prev_ptr;
			child_ptr = rhs.child_ptr;
			return *this;
		}

		inline ClassNode& operator =(ClassNode&& rts)noexcept = delete;
	};

	/// @brief 翻訳単位(project)の情報
	struct Artifact
	{
		std::int32_t counter_;

		//	型IDをキーとした検索用
		struct
		{
			//	型IDをキーとした検索用
			nox::UnorderedMap<std::uint32_t, ClassNode> class_node_map;
			nox::UnorderedMap<std::uint32_t, std::reference_wrapper<const nox::reflection::ClassInfo>> union_map;
			nox::UnorderedMap<const nox::ObjectPointerId*, std::reference_wrapper<const nox::reflection::VariableInfo>> variable_map;
			nox::UnorderedMap<const nox::FunctionPointerId*, std::reference_wrapper<const nox::reflection::FunctionInfo>> function_map;
			nox::UnorderedMap<std::uint32_t, std::reference_wrapper<const nox::reflection::EnumInfo>> enum_map;
		}chunk_with_type_id;

		//	名前のハッシュをキーとした検索用
		struct
		{
			nox::UnorderedMap<std::uint32_t, std::reference_wrapper<const ClassNode>> class_node_map;
			nox::UnorderedMap<std::uint32_t, std::reference_wrapper<const nox::reflection::ClassInfo>> union_map;
			nox::UnorderedMap<std::uint32_t, std::reference_wrapper<const nox::reflection::VariableInfo>> variable_map;
			nox::UnorderedMap<std::uint32_t, std::reference_wrapper<const nox::reflection::FunctionInfo>> function_map;
			nox::UnorderedMap<std::uint32_t, std::reference_wrapper<const nox::reflection::EnumInfo>> enum_map;

		}chunk_with_name_hash;

		inline Artifact() :counter_(0)
		{}

		inline Artifact(const Artifact&)noexcept
		{
			NOX_ASSERT(false, U"failed");
		}

		inline Artifact(Artifact&& rhs)noexcept
		{
			counter_ = rhs.counter_;

			chunk_with_type_id.class_node_map = std::move(rhs.chunk_with_type_id.class_node_map);
			chunk_with_type_id.union_map = std::move(rhs.chunk_with_type_id.union_map);
			chunk_with_type_id.variable_map = std::move(rhs.chunk_with_type_id.variable_map);
			chunk_with_type_id.function_map = std::move(rhs.chunk_with_type_id.function_map);
			chunk_with_type_id.enum_map = std::move(rhs.chunk_with_type_id.enum_map);

			chunk_with_name_hash.class_node_map = std::move(rhs.chunk_with_name_hash.class_node_map);
			chunk_with_name_hash.union_map = std::move(rhs.chunk_with_name_hash.union_map);
			chunk_with_name_hash.variable_map = std::move(rhs.chunk_with_name_hash.variable_map);
			chunk_with_name_hash.function_map = std::move(rhs.chunk_with_name_hash.function_map);
			chunk_with_name_hash.enum_map = std::move(rhs.chunk_with_name_hash.enum_map);
		}
	};

	//	DB

	/// @brief 全ての翻訳単位の情報を格納するマップ
	nox::UnorderedMap<std::uint32_t, Artifact> artifact_map_;

	/// @brief 全ての型情報を格納するマップ
	nox::UnorderedMap<std::uint32_t, std::reference_wrapper<const nox::reflection::Type>> all_type_id_map_;

	inline	Artifact& GetCreateArtifact(std::uint32_t name_hash)
	{
		if (artifact_map_.contains(name_hash) == true)
		{
			return artifact_map_.at(name_hash);
		}

		return artifact_map_.emplace(name_hash, Artifact{}).first->second;
	}

	inline	void RegisterTypeIdMap(const nox::reflection::Type& type)
	{
		const auto id = type.GetTypeID();
		if (all_type_id_map_.contains(id) == false)
		{
			all_type_id_map_.emplace(type.GetTypeID(), type);
		}
		else
		{
			NOX_ASSERT(false, nox::util::Format(U"TypeIdが重複していますA:{0}, B:{1}", all_type_id_map_.at(id).get().GetTypeName(), type.GetTypeName()));
		}
	}

	inline	void UnregisterTypeIdMap(const nox::reflection::Type& type)
	{
		all_type_id_map_.erase(type.GetTypeID());
	}
}

void nox::reflection::Initialize()
{
	artifact_map_.clear();
	all_type_id_map_.clear();
}

void nox::reflection::Finalize()
{
	artifact_map_ = {};
	all_type_id_map_ = {};
}

const nox::reflection::ClassInfo* nox::reflection::FindClassInfo(const nox::reflection::Type& type)noexcept
{
	const auto id = type.GetTypeID();

	for (const Artifact& artifact : artifact_map_ | std::views::values)
	{
		const auto it = artifact.chunk_with_type_id.class_node_map.find(id);
		if (it != artifact.chunk_with_type_id.class_node_map.end())
		{
			return &it->second.class_info.get();
		}
	}

	return nullptr;
}

const nox::reflection::ClassInfo* nox::reflection::FindClassInfo(std::uint32_t namehash)noexcept
{
	for (const Artifact& artifact : artifact_map_ | std::views::values)
	{
		const auto it = artifact.chunk_with_name_hash.class_node_map.find(namehash);
		if (it != artifact.chunk_with_name_hash.class_node_map.end())
		{
			return &it->second.get().class_info.get();
		}
	}
	return nullptr;
}

const nox::reflection::EnumInfo* nox::reflection::FindEnumInfo(const nox::reflection::Type& type)noexcept
{
	const auto id = type.GetTypeID();

	for (const Artifact& artifact : artifact_map_ | std::views::values)
	{
		const auto it = artifact.chunk_with_type_id.enum_map.find(id);
		if (it != artifact.chunk_with_type_id.enum_map.end())
		{
			return &it->second.get();
		}
	}
	return nullptr;
}

const nox::reflection::EnumInfo* nox::reflection::FindEnumInfo(const std::uint32_t artiifact_name_hash, const nox::reflection::Type& type)noexcept
{
	const auto id = type.GetTypeID();

	const auto artifact_it = artifact_map_.find(artiifact_name_hash);
	if (artifact_it == artifact_map_.end())
	{
		return nullptr;
	}

	const auto enum_it = artifact_it->second.chunk_with_type_id.enum_map.find(id);
	if (enum_it == artifact_it->second.chunk_with_type_id.enum_map.end())
	{
		return nullptr;
	}

	return &enum_it->second.get();
}

const nox::reflection::FunctionInfo* nox::reflection::FindFunctionInfo(const nox::FunctionPointerId& id)noexcept
{
	for (const Artifact& artifact : artifact_map_ | std::views::values)
	{
		const auto it = artifact.chunk_with_type_id.function_map.find(&id);
		if (it != artifact.chunk_with_type_id.function_map.end())
		{
			return &it->second.get();
		}
	}
	return nullptr;
}

const nox::reflection::FunctionInfo* nox::reflection::FindFunctionInfoWithNameHash(const std::uint32_t name_hash)noexcept
{
	for (const Artifact& artifact : artifact_map_ | std::views::values)
	{
		const auto it = artifact.chunk_with_name_hash.function_map.find(name_hash);
		if (it != artifact.chunk_with_name_hash.function_map.end())
		{
			return &it->second.get();
		}
	}
	return nullptr;
}

const nox::reflection::VariableInfo* nox::reflection::FindVariableInfo(const nox::ObjectPointerId& id)noexcept
{
	for (const Artifact& artifact : artifact_map_ | std::views::values)
	{
		const auto it = artifact.chunk_with_type_id.variable_map.find(&id);
		if (it != artifact.chunk_with_type_id.variable_map.end())
		{
			return &it->second.get();
		}
	}
	return nullptr;
}

const nox::reflection::VariableInfo* nox::reflection::FindVariableInfoWithNameHash(const std::uint32_t name_hash)noexcept
{
	for (const Artifact& artifact : artifact_map_ | std::views::values)
	{
		const auto it = artifact.chunk_with_name_hash.variable_map.find(name_hash);
		if (it != artifact.chunk_with_name_hash.variable_map.end())
		{
			return &it->second.get();
		}
	}
	return nullptr;
}

bool nox::reflection::IsBaseOf(const nox::reflection::ClassInfo& base, const nox::reflection::ClassInfo& derived)
{
	const nox::reflection::Type& base_type = base.GetUnderlyingType();

	//TODO:	遅い
	for (const nox::reflection::Type& tmp_base_type : derived.GetBaseTypeList())
	{
		if (tmp_base_type == base_type)
		{
			return true;
		}

		if (IsBaseOf(base_type, tmp_base_type) == true)
		{
			return true;
		}
	}

	return false;
}

void	nox::reflection::Register(const std::uint32_t artifact_name_hash, const nox::reflection::ClassInfo& data)
{
	const auto id = data.GetUnderlyingType().GetTypeID();

	Artifact& artifact = GetCreateArtifact(artifact_name_hash);
	++artifact.counter_;

	if (data.GetUnderlyingType().IsUnion() == true)
	{
		artifact.chunk_with_type_id.union_map.emplace(data.GetUnderlyingType().GetTypeID(), data);
		artifact.chunk_with_name_hash.union_map.emplace(nox::util::Crc32(data.GetFullName()), data);
	}
	else
	{
		ClassNode& new_class_node = artifact.chunk_with_type_id.class_node_map.emplace(id, ClassNode(data)).first->second;
		artifact.chunk_with_name_hash.class_node_map.emplace(nox::util::Crc32(data.GetFullName()), new_class_node);


		for (const nox::reflection::Type& baseType : data.GetBaseTypeList())
		{
			ClassNode& parent_class_node = artifact.chunk_with_type_id.class_node_map.at(baseType.GetTypeID());
			if (parent_class_node.child_ptr == nullptr)
			{
				parent_class_node.child_ptr = &new_class_node;
			}
			else
			{
				for (ClassNode* class_node = parent_class_node.child_ptr;/*none*/; class_node = class_node->next_ptr)
				{
					if (class_node->next_ptr == nullptr)
					{
						class_node->next_ptr = &new_class_node;
						new_class_node.prev_ptr = class_node;
						break;
					}
				}
			}
		}
	}
}

void nox::reflection::Unregister(const std::uint32_t artifact_name_hash, const nox::reflection::ClassInfo& data)
{
}

//
//void Reflection::Register(const ClassInfo& data)
//{
//	NOX_ASSERT(class_data_map_.contains(data.GetUnderlyingType().GetTypeID()) == true, U"");
//
//	class_data_map_.emplace(data.GetUnderlyingType().GetTypeID(), ClassData{.class_info_ptr = &data});
//
//	//	親子関係の構築
//	//	親より先に子が登録された場合の保険処理
//	ClassData& newClassData = class_data_map_.at(data.GetUnderlyingType().GetTypeID());
//	auto baseTypeList = data.GetBaseTypeList();
//	for (const ClassInfo& baseType : baseTypeList)
//	{
//		if (class_data_map_.contains(baseType.GetUnderlyingType().GetTypeID()) == false)
//		{
//			class_data_map_.emplace(baseType.GetUnderlyingType().GetTypeID(), ClassData());
//		}
//	}
//
//	for (const Type*const baseType : baseTypeList)
//	{
//		ClassData& parentClassData = class_data_map_.at(baseType->GetTypeID());
//		if (parentClassData.child_ptr == nullptr)
//		{
//			parentClassData.child_ptr = &newClassData;
//		}
//		else
//		{
//			for (ClassData* classDataPtr = parentClassData.child_ptr;/*none*/; classDataPtr = classDataPtr->next_ptr)
//			{
//				if (classDataPtr->next_ptr == nullptr)
//				{
//					classDataPtr->next_ptr = &newClassData;
//					newClassData.prev_ptr = classDataPtr;
//					break;
//				}
//			}
//		}
//	}
//
//	//	グローバル空間への登録
//	const std::uint32_t namespaceID = data.GetNamespaceID();
//	if (global_data_map_.contains(namespaceID) == false)
//	{
//		global_data_map_.emplace(namespaceID, GlobalData());
//	}
//	GlobalData& globalData = global_data_map_.at(namespaceID);
//	globalData.class_info_ptr_map.emplace(data.GetTypeID(), &data);
//}
//
//void Reflection::Unregister(const class ClassInfo& data)
//{
//	if (class_data_map_.erase(data.GetTypeID()) == 0)
//	{
////		NOX_CONDITINAL_DEVELOP(dev::Assert(false, util::Format(u"クラスデータの登録に失敗しました:{}", data.GetFullName())));
//	}
//}

void nox::reflection::Register(const std::uint32_t artifact_name_hash, const nox::reflection::EnumInfo& data)
{
	const auto id = data.GetUnderlyingType().GetTypeID();

	Artifact& artifact = GetCreateArtifact(artifact_name_hash);
	++artifact.counter_;

	artifact.chunk_with_type_id.enum_map.emplace(id, data);
	artifact.chunk_with_name_hash.enum_map.emplace(nox::util::Crc32(data.GetFullName()), data);
}

void nox::reflection::Unregister(const std::uint32_t artifact_name_hash, const nox::reflection::EnumInfo& data)
{
}

void nox::reflection::Register(const std::uint32_t artifact_name_hash, const nox::reflection::VariableInfo& data)
{
	Artifact& artifact = GetCreateArtifact(artifact_name_hash);
	++artifact.counter_;

	artifact.chunk_with_type_id.variable_map.emplace(&data.GetObjectPointerId(), data);
	artifact.chunk_with_name_hash.variable_map.emplace(nox::util::Crc32(data.GetFullName()), data);

}

void nox::reflection::Unregister(const std::uint32_t artifact_name_hash, const nox::reflection::VariableInfo& data)
{

}

void nox::reflection::Register(const std::uint32_t artifact_name_hash, const nox::reflection::FunctionInfo& data)
{
	Artifact& artifact = GetCreateArtifact(artifact_name_hash);
	++artifact.counter_;

	artifact.chunk_with_type_id.function_map.emplace(&data.GetFunctionId(), data);
	artifact.chunk_with_name_hash.function_map.emplace(nox::util::Crc32(data.GetFullName()), data);
}

void nox::reflection::Unregister(const std::uint32_t artifact_name_hash, const nox::reflection::FunctionInfo& data)
{

}