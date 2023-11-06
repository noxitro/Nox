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

	//	非ポインタ型のconst型かどうか



	return false;
}

bool	nox::reflection::IsConvertible(const Type& a, const Type& b)
{
	if (a.Equal(b) == true)
	{
		return true;
	}

	

	return true;
}