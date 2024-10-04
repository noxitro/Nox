///	@file	manager.cpp
///	@brief	manager
#include	"stdafx.h"
#include	"manager.h"



#include	"user_defined_compound_type_info.h"
#include	"enum_info.h"
#include	"variable_info.h"
#include	"function_info.h"

namespace
{

}

//
//template<class T, typename FuncType> 
//	requires(std::is_pointer_v<nox::FunctionReturnType<FuncType>>)
//inline constexpr nox::FunctionReturnType<FuncType> At(T& map, typename T::key_type key, FuncType func)
//{
//	if (map.contains(key) == false)
//	{
//		return nullptr;
//	}
//	return std::invoke(func, map.at(key));
//}

void	nox::reflection::Reflection::Finalize()
{
	artifact_map_.clear();
	all_type_id_map_.clear();
}

inline	nox::reflection::Reflection::Artifact& nox::reflection::Reflection::GetCreateArtifact(std::uint32_t name_hash)
{
	if (artifact_map_.contains(name_hash) == true)
	{
		return artifact_map_.at(name_hash);
	}

	return artifact_map_.emplace(name_hash, Artifact{}).first->second;
}

inline	void nox::reflection::Reflection::RegisterTypeIdMap(const nox::reflection::Type& type)
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

inline	void nox::reflection::Reflection::UnregisterTypeIdMap(const nox::reflection::Type& type)
{
	all_type_id_map_.erase(type.GetTypeID());
}

const nox::reflection::UserDefinedCompoundTypeInfo* nox::reflection::Reflection::FindClassInfo(const nox::reflection::Type& type)const noexcept
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

const nox::reflection::UserDefinedCompoundTypeInfo* nox::reflection::Reflection::FindClassInfo(std::uint32_t namehash)const noexcept
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

const nox::reflection::EnumInfo* nox::reflection::Reflection::FindEnumInfo(const nox::reflection::Type& type)const noexcept
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

const class nox::reflection::EnumInfo* nox::reflection::Reflection::FindEnumInfo(const std::uint32_t artiifact_name_hash, const nox::reflection::Type& type)const noexcept
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

const nox::reflection::FunctionInfo* nox::reflection::Reflection::FindFunctionInfo(const std::uint64_t id)const noexcept
{
	for (const Artifact& artifact : artifact_map_ | std::views::values)
	{
		const auto it = artifact.chunk_with_type_id.function_map.find(id);
		if (it != artifact.chunk_with_type_id.function_map.end())
		{
			return &it->second.get();
		}
	}
	return nullptr;

}

bool nox::reflection::Reflection::IsBaseOf(const nox::reflection::UserDefinedCompoundTypeInfo& base, const nox::reflection::UserDefinedCompoundTypeInfo& derived)
{
	const nox::reflection::Type& base_type = base.GetType();

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

void	nox::reflection::Reflection::Register(const std::uint32_t artifact_name_hash, const nox::reflection::UserDefinedCompoundTypeInfo& data)
{
	const auto id = data.GetType().GetTypeID();

	nox::reflection::Reflection::Artifact& artifact = GetCreateArtifact(artifact_name_hash);
	++artifact.counter_;

	if (data.GetType().IsUnion() == true)
	{
		artifact.chunk_with_type_id.union_map.emplace(data.GetType().GetTypeID(), data);
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

void nox::reflection::Reflection::Unregister(const std::uint32_t artifact_name_hash, const nox::reflection::UserDefinedCompoundTypeInfo& data)
{
}

//
//void Reflection::Register(const UserDefinedCompoundTypeInfo& data)
//{
//	NOX_ASSERT(class_data_map_.contains(data.GetType().GetTypeID()) == true, U"");
//
//	class_data_map_.emplace(data.GetType().GetTypeID(), ClassData{.class_info_ptr = &data});
//
//	//	親子関係の構築
//	//	親より先に子が登録された場合の保険処理
//	ClassData& newClassData = class_data_map_.at(data.GetType().GetTypeID());
//	auto baseTypeList = data.GetBaseTypeList();
//	for (const UserDefinedCompoundTypeInfo& baseType : baseTypeList)
//	{
//		if (class_data_map_.contains(baseType.GetType().GetTypeID()) == false)
//		{
//			class_data_map_.emplace(baseType.GetType().GetTypeID(), ClassData());
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
//void Reflection::Unregister(const class UserDefinedCompoundTypeInfo& data)
//{
//	if (class_data_map_.erase(data.GetTypeID()) == 0)
//	{
////		NOX_CONDITINAL_DEVELOP(dev::Assert(false, util::Format(u"クラスデータの登録に失敗しました:{}", data.GetFullName())));
//	}
//}

void nox::reflection::Reflection::Register(const std::uint32_t artifact_name_hash, const nox::reflection::EnumInfo& data)
{
	const auto id = data.GetType().GetTypeID();

	nox::reflection::Reflection::Artifact& artifact = GetCreateArtifact(artifact_name_hash);
	++artifact.counter_;

	artifact.chunk_with_type_id.enum_map.emplace(id, data);
	artifact.chunk_with_name_hash.enum_map.emplace(nox::util::Crc32(data.GetFullName()), data);
}

void nox::reflection::Reflection::Unregister(const std::uint32_t artifact_name_hash, const nox::reflection::EnumInfo& data)
{
}



void nox::reflection::Reflection::Register(const std::uint32_t artifact_name_hash, const nox::reflection::VariableInfo& data)
{
	const std::uint64_t id = data.GetID();
	
	nox::reflection::Reflection::Artifact& artifact = GetCreateArtifact(artifact_name_hash);
	++artifact.counter_;

	artifact.chunk_with_type_id.variable_map.emplace(id, data);
	artifact.chunk_with_name_hash.variable_map.emplace(nox::util::Crc32(data.GetFullName()), data);

}

void nox::reflection::Reflection::Unregister(const std::uint32_t artifact_name_hash, const nox::reflection::VariableInfo& data)
{

}

void nox::reflection::Reflection::Register(const std::uint32_t artifact_name_hash, const nox::reflection::FunctionInfo& data)
{
	const std::uint64_t id = data.GetID();

	nox::reflection::Reflection::Artifact& artifact = GetCreateArtifact(artifact_name_hash);
	++artifact.counter_;

	artifact.chunk_with_type_id.function_map.emplace(id, data);
	artifact.chunk_with_name_hash.function_map.emplace(nox::util::Crc32(data.GetFullName()), data);
}

void nox::reflection::Reflection::Unregister(const std::uint32_t artifact_name_hash, const nox::reflection::FunctionInfo& data)
{

}