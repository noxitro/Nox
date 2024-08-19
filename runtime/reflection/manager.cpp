///	@file	manager.cpp
///	@brief	manager
#include	"stdafx.h"
#include	"manager.h"

#include	"user_defined_compound_type_info.h"
#include	"enum_info.h"
#include	"variable_info.h"
#include	"function_info.h"

using namespace nox;
using namespace nox::reflection;

template<class T, typename FuncType> 
	requires(std::is_pointer_v<FunctionReturnType<FuncType>>)
inline constexpr FunctionReturnType<FuncType> At(T& map, typename T::key_type key, FuncType func)
{
	if (map.contains(key) == false)
	{
		return nullptr;
	}
	return std::invoke(func, map.at(key));
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

void Reflection::Register(const EnumInfo& data)
{
}

void Reflection::Unregister(const EnumInfo& data)
{
}

void Reflection::Register(const VariableInfo& data)
{

}

void Reflection::Unregister(const VariableInfo& data)
{

}

void Reflection::Register(const FunctionInfo& data)
{
}

void Reflection::Unregister(const FunctionInfo& data)
{

}