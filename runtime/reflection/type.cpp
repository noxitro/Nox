///	@file	type.cpp
///	@brief	type
#include	"stdafx.h"
#include	"type.h"

using namespace nox::reflection;

bool	nox::reflection::IsConvertible(const Type& a, const Type& b)
{
	if (a.Equal(b) == true)
	{
		return true;
	}


	return true;
}