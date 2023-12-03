///	@file	type.cpp
///	@brief	type
#include	"stdafx.h"
#include	"type.h"

using namespace nox::reflection;


bool	Type::IsConvertible(const Type& to)const noexcept
{
	if (*this == to)
	{
		return true;
	}

	//	修飾子を取り除いた型が一致するか
	if (GetDesugarType() == to)
	{
		return true;
	}

	//	変換先が参照型の場合
	if (to.IsReference() == true)
	{
		const Type& toPointeeType = to.GetPointeeType();
		if (*this == toPointeeType || GetDesugarType() == toPointeeType)
		{
			return true;
		}
	}

	//	参照型の場合、参照を取り除いた型でチェックする
	if (IsReference() == true)
	{
		const Type& pointeeType = GetPointeeType();
		if (pointeeType == to || pointeeType.GetDesugarType() == to)
		{
			return true;
		}

		//	変換先が参照型の場合
		if (to.IsReference() == true)
		{
			const Type& toPointeeType = to.GetPointeeType();
			if (pointeeType == toPointeeType || pointeeType.GetDesugarType() == toPointeeType)
			{
				return true;
			}
		}
	}



	return false;
}