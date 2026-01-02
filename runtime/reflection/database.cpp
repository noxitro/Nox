//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	database.cpp
///	@brief	database
#include	"stdafx.h"
#include	"database.h"

#include	"class_info.h"
#include	"enum_info.h"
#include	"variable_info.h"
#include	"function_info.h"
#include	"reflection_object.h"

#include	"reflection_generated_register.h"
#include	"log_id.h"

namespace nox::util
{
	//template<class Key, class Value, class Hasher = std::hash<Key>, class Keyeq = std::equal_to<Key>>
	//inline constexpr 
	//	std::conditional_t<
	//	IsReferenceWrapperV<Value>, 
	//	std::optional<Value>, 
	//	std::optional<std::reference_wrapper<std::remove_reference_t<Value>>>
	//	>
	//	Find(const nox::UnorderedMap<Key, Value, Hasher, Keyeq>& container, Key&& key)
	//{
	//	auto it = container.find(std::forward<Key>(key));
	//	if (it != container.end())
	//	{
	//		return it->second;
	//	}
	//	return std::nullopt;
	//}

}

namespace nox::reflection
{
	/// @brief クラスデータ
	struct ClassNode
	{
		std::reference_wrapper<const nox::reflection::Type> type;
		//	ノード構築時に不明な場合があるのでポインタで保持
		//	子を登録する時、親が登録されていないことがあるため
		const nox::reflection::ClassInfo* class_info;
		ClassNode* next_ptr;
		const ClassNode* prev_ptr;
		ClassNode* child_ptr;

		inline const nox::reflection::ClassInfo& getClassInfo()const noexcept
		{
			NOX_ASSERT(class_info != nullptr, u"ClassInfo is null");
			return *class_info;
		}

		inline constexpr explicit ClassNode(const nox::reflection::Type& _type)noexcept :
			type(_type),
			class_info(nullptr),
			next_ptr(nullptr),
			prev_ptr(nullptr),
			child_ptr(nullptr)
		{
		}

		inline constexpr explicit ClassNode(const nox::reflection::ClassInfo& _class_info)noexcept :
			type(_class_info.GetType()),
			class_info(&_class_info),
			next_ptr(nullptr),
			prev_ptr(nullptr),
			child_ptr(nullptr)
		{
		}

		inline constexpr ClassNode(const ClassNode& rhs)noexcept :
			type(rhs.type),
			class_info(rhs.class_info),
			next_ptr(rhs.next_ptr),
			prev_ptr(rhs.prev_ptr),
			child_ptr(rhs.child_ptr)
		{
		}

		inline ClassNode(const ClassNode&&)noexcept = delete;

		inline ClassNode& operator =(const ClassNode& rhs)noexcept
		{
			type = rhs.type;
			class_info = rhs.class_info;
			next_ptr = rhs.next_ptr;
			prev_ptr = rhs.prev_ptr;
			child_ptr = rhs.child_ptr;
			return *this;
		}

		inline ClassNode& operator =(ClassNode&& rts)noexcept = delete;
	};

	//	DB
	// //	型情報をキーとした検索用
	struct
	{
		nox::UnorderedMap<const nox::reflection::Type*, ClassNode> class_node_map;
		nox::UnorderedMap<const nox::reflection::Type*, std::reference_wrapper<const nox::reflection::ClassInfo>> union_map;
		nox::UnorderedMap<const nox::ObjectPointerId*, std::reference_wrapper<const nox::reflection::VariableInfo>> variable_map;
		nox::UnorderedMap<const nox::FunctionPointerId*, std::reference_wrapper<const nox::reflection::FunctionInfo>> function_map;
		nox::UnorderedMap<const nox::reflection::Type*, std::reference_wrapper<const nox::reflection::EnumInfo>> enum_map;
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
	/// @brief 全ての翻訳単位の情報を格納するマップ

	nox::HashSet<const nox::reflection::Type*> all_type_hash_set_;
	nox::Vector<std::reference_wrapper<const nox::reflection::Type>> all_type_list_;

	inline	const nox::reflection::ClassNode& GetRootClassNode()noexcept
	{
		auto r = chunk_with_type_id.class_node_map.find(&nox::reflection::Typeof<nox::reflection::ReflectionObject>());
		NOX_ASSERT(r != chunk_with_type_id.class_node_map.end(), u"ReflectionObject class node not found");
		return r->second;
	}

	inline	void RegisterTypeIdMap(const nox::reflection::Type&)
	{
	}

	inline	void UnregisterTypeIdMap(const nox::reflection::Type&)
	{
	}
	
	inline const ClassNode* FindClassNode(const nox::reflection::Type& type)
	{
		const auto& class_node_map = chunk_with_type_id.class_node_map;

		auto it = class_node_map.find(&type);
		if (it != class_node_map.end())
		{
			return &it->second;
		}
		return nullptr;
	}
}

namespace nox::util
{
	inline std::optional<std::reference_wrapper<nox::reflection::ClassNode>> Find(nox::UnorderedMap<const nox::reflection::Type*, nox::reflection::ClassNode>& dict, const nox::reflection::Type* key)
	{
		auto it = dict.find(key);
		if (it != dict.end())
		{
			return std::ref(it->second);
		}

		return std::nullopt;
	}
}

void nox::reflection::Initialize()
{
	{
#if !NOX_MASTER
		NOX_LOCAL_SCOPE(nox::util::ScopeProfile(u"reflection initialize"));
#endif // NOX_MASTER

		nox::reflection::InitializeGen();
	}

	//TODO	object下のクラス情報の整合性チェック

	//	
	{
		NOX_INFO_LINE(nox::log_id::Reflection, u"=====typedb=====");
		nox::uint32 class_count = 0;
		nox::uint32 enum_count = 0;
		nox::uint32 function_count = 0;
		nox::uint32 variable_count = 0;

		{
			class_count += static_cast<nox::uint32>(chunk_with_type_id.class_node_map.size());
			enum_count += static_cast<nox::uint32>(chunk_with_type_id.enum_map.size());
			function_count += static_cast<nox::uint32>(chunk_with_type_id.function_map.size());
			variable_count += static_cast<nox::uint32>(chunk_with_type_id.variable_map.size());
		}
		NOX_INFO_LINE(nox::log_id::Reflection, u"class type count:{0}", class_count);
		NOX_INFO_LINE(nox::log_id::Reflection, u"enum type count:{0}", enum_count);
		NOX_INFO_LINE(nox::log_id::Reflection, u"function count:{0}", function_count);
		NOX_INFO_LINE(nox::log_id::Reflection, u"variable count:{0}", variable_count);
		NOX_INFO_LINE(nox::log_id::Reflection, u"=====typedb end=====");
	}

	//	dump class node
	{
		NOX_INFO_LINE(nox::log_id::Reflection, u"======class node dump======");


		constexpr std::array<nox::char16, 1024> space = []()constexpr noexcept->auto
		{
			std::array<nox::char16, 1024> result = {};
			for (nox::uint16 i = 0; i < result.size(); ++i)
			{
				result[i] = u'\t';
			}
			return result;
			}();

		const auto dump_class_node = [&space](this auto self, const nox::reflection::ClassNode& node, nox::uint16 depth = 0) -> void
			{
				if (depth > 0)
				{
					NOX_INFO_LINE(nox::log_id::Reflection, u"{0}{1}", std::u16string_view(space.data(), depth), node.getClassInfo().GetFullName());
				}
				else 
				{
					NOX_INFO_LINE(nox::log_id::Reflection, u"{0}", node.getClassInfo().GetFullName());
				}

				if (node.child_ptr != nullptr)
				{
					self(*node.child_ptr, depth + 1);
				}

				if (node.next_ptr != nullptr)
				{
					self(*node.next_ptr, depth);
				}
			};

		dump_class_node(GetRootClassNode());

		NOX_INFO_LINE(nox::log_id::Reflection, u"======class node dump end======");
	}
}

void nox::reflection::Finalize()
{
	nox::reflection::FinalizeGen();
	chunk_with_type_id = {};
	chunk_with_name_hash = {};
	//all_type_id_map_ = {};
}

const nox::reflection::ClassInfo* nox::reflection::FindClassInfo(const nox::reflection::Type& type)noexcept
{
	{
		const auto it = chunk_with_type_id.class_node_map.find(&type);
		if (it != chunk_with_type_id.class_node_map.end())
		{
			return it->second.class_info;
		}
	}

	return nullptr;
}

const nox::reflection::ClassInfo* nox::reflection::FindClassInfo(std::uint32_t namehash)noexcept
{
	{
		const auto it = chunk_with_name_hash.class_node_map.find(namehash);
		if (it != chunk_with_name_hash.class_node_map.end())
		{
			return it->second.get().class_info;
		}
	}
	return nullptr;
}

namespace nox::reflection
{
}

void nox::reflection::ForeachDerivedClassInfoList(const nox::reflection::Type& type, std::move_only_function<void(const nox::reflection::ClassInfo&)> callback, bool include_self, bool recursive)
{
	const auto search = [&type, &callback, include_self, recursive](this auto self, const nox::reflection::ClassNode& node, bool target_child = false)->void
		{
			const bool target_self = node.type == type;
			if (target_child || (include_self && target_self))
			{
				callback(node.getClassInfo());
			}

			if (node.child_ptr != nullptr)
			{
				self(*node.child_ptr, target_self);
			}

			if (node.next_ptr != nullptr)
			{
				self(*node.next_ptr);
			}
		};

	search(nox::reflection::GetRootClassNode());
}

const nox::reflection::EnumInfo* nox::reflection::FindEnumInfo(const nox::reflection::Type& type)noexcept
{
	{
		const auto it = chunk_with_type_id.enum_map.find(&type);
		if (it != chunk_with_type_id.enum_map.end())
		{
			return &it->second.get();
		}
	}
	return nullptr;
}

const nox::reflection::EnumInfo* nox::reflection::FindEnumInfo(const std::uint32_t artiifact_name_hash, const nox::reflection::Type& type)noexcept
{
	const auto enum_it = chunk_with_type_id.enum_map.find(&type);
	if (enum_it == chunk_with_type_id.enum_map.end())
	{
		return nullptr;
	}

	return &enum_it->second.get();
}

const nox::reflection::FunctionInfo* nox::reflection::FindFunctionInfo(const nox::FunctionPointerId& id)noexcept
{
	{
		const auto it = chunk_with_type_id.function_map.find(&id);
		if (it != chunk_with_type_id.function_map.end())
		{
			return &it->second.get();
		}
	}
	return nullptr;
}

const nox::reflection::FunctionInfo* nox::reflection::FindFunctionInfoWithNameHash(const std::uint32_t name_hash)noexcept
{
	{
		const auto it = chunk_with_name_hash.function_map.find(name_hash);
		if (it != chunk_with_name_hash.function_map.end())
		{
			return &it->second.get();
		}
	}
	return nullptr;
}

const nox::reflection::VariableInfo* nox::reflection::FindVariableInfo(const nox::ObjectPointerId& id)noexcept
{
	{
		const auto it = chunk_with_type_id.variable_map.find(&id);
		if (it != chunk_with_type_id.variable_map.end())
		{
			return &it->second.get();
		}
	}
	return nullptr;
}

const nox::reflection::VariableInfo* nox::reflection::FindVariableInfoWithNameHash(const std::uint32_t name_hash)noexcept
{
	{
		const auto it = chunk_with_name_hash.variable_map.find(name_hash);
		if (it != chunk_with_name_hash.variable_map.end())
		{
			return &it->second.get();
		}
	}
	return nullptr;
}

bool nox::reflection::IsBaseOf(const nox::reflection::ClassInfo& base, const nox::reflection::ClassInfo& derived)noexcept
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

bool nox::reflection::detail::IsBaseOf(const nox::reflection::Type& base, const nox::reflection::ReflectionObject& from)noexcept
{
	return nox::reflection::IsBaseOf(base, from.GetType());
}

void	nox::reflection::Register(const nox::reflection::ClassInfo& data)
{
	const nox::reflection::Type& type = data.GetType();

	if (data.GetType().IsUnion() == true)
	{
		chunk_with_type_id.union_map.emplace(&type, data);
		chunk_with_name_hash.union_map.emplace(nox::util::Crc32(data.GetFullName()), data);
		return;
	}
	else
	{
		//	継承関係の構築
		ClassNode& self_class_node = [&]()->ClassNode& {
			const auto r = nox::util::Find(chunk_with_type_id.class_node_map, &type);
			if (r)
			{
				//	既に存在する場合
				ClassNode& node = r.value();
				NOX_ASSERT(node.class_info == nullptr, u"already registered class info:{}", data.GetFullName());
				node.class_info = &data;

				chunk_with_name_hash.class_node_map.emplace(nox::util::Crc32(data.GetFullName()), node);
				return node;
			}
			else
			{
				ClassNode& new_class_node = chunk_with_type_id.class_node_map.emplace(&type, ClassNode(data)).first->second;
				chunk_with_name_hash.class_node_map.emplace(nox::util::Crc32(data.GetFullName()), new_class_node);
				return new_class_node;
			}
			}();

		for (const nox::reflection::Type& base_type : data.GetBaseTypeList())
		{
			if (base_type.IsInterface())
			{
				continue;
			}

			if (chunk_with_type_id.class_node_map.contains(&base_type) == false)
			{
				chunk_with_type_id.class_node_map.emplace(&base_type, ClassNode(base_type));
			}

			ClassNode& parent_class_node = chunk_with_type_id.class_node_map.at(&base_type);
			if (parent_class_node.child_ptr == nullptr)
			{
				parent_class_node.child_ptr = &self_class_node;
			}
			else
			{
				for (ClassNode* class_node = parent_class_node.child_ptr;/*none*/; class_node = class_node->next_ptr)
				{
					if (class_node->next_ptr == nullptr)
					{
						class_node->next_ptr = &self_class_node;
						self_class_node.prev_ptr = class_node;
						break;
					}
				}
			}
		}
	}
}

void nox::reflection::Unregister(const nox::reflection::ClassInfo& data)
{
	const std::uint32_t artifact_name_hash = nox::util::Crc32(data.GetFullName());
}

//
//void Reflection::Register(const ClassInfo& data)
//{
//	NOX_ASSERT(class_data_map_.contains(data.GetType().GetTypeID()) == true, U"");
//
//	class_data_map_.emplace(data.GetType().GetTypeID(), ClassData{.class_info_ptr = &data});
//
//	//	親子関係の構築
//	//	親より先に子が登録された場合の保険処理
//	ClassData& newClassData = class_data_map_.at(data.GetType().GetTypeID());
//	auto baseTypeList = data.GetBaseTypeList();
//	for (const ClassInfo& baseType : baseTypeList)
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
//void Reflection::Unregister(const class ClassInfo& data)
//{
//	if (class_data_map_.erase(data.GetTypeID()) == 0)
//	{
////		NOX_CONDITINAL_DEVELOP(dev::Assert(false, util::Format(u"クラスデータの登録に失敗しました:{}", data.GetFullName())));
//	}
//}

void nox::reflection::Register(const nox::reflection::EnumInfo& data)
{
	const nox::reflection::Type& type = data.GetType();

	chunk_with_type_id.enum_map.emplace(&type, data);
	chunk_with_name_hash.enum_map.emplace(nox::util::Crc32(data.GetFullName()), data);
}

void nox::reflection::Unregister(const nox::reflection::EnumInfo& data)
{
	const std::uint32_t artifact_name_hash = nox::util::Crc32(data.GetFullName());
}

void nox::reflection::Register(const nox::reflection::VariableInfo& data)
{
//	NOX_INFO_LINE(nox::log_id::Reflection, U"Register VariableInfo:{0}", data.GetFullName());

	chunk_with_type_id.variable_map.emplace(&data.GetObjectPointerId(), data);
	chunk_with_name_hash.variable_map.emplace(nox::util::Crc32(data.GetFullName()), data);
}

void nox::reflection::Unregister(const nox::reflection::VariableInfo& data)
{
}

void nox::reflection::Register(const nox::reflection::FunctionInfo& data)
{
	chunk_with_type_id.function_map.emplace(&data.GetFunctionId(), data);
	chunk_with_name_hash.function_map.emplace(nox::util::Crc32(data.GetFullName()), data);
}

void nox::reflection::Unregister(const nox::reflection::FunctionInfo& data)
{
	chunk_with_type_id.function_map.erase(&data.GetFunctionId());
	chunk_with_name_hash.function_map.erase(nox::util::Crc32(data.GetFullName()));
}