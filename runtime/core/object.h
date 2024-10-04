//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	object.h
///	@brief	object
#pragma once
#include	"object_definition.h"

namespace nox
{
	/// @brief 基底オブジェクト
	class Object : public nox::reflection::ReflectionObject
	{
		NOX_DECLARE_OBJECT_ROOT(Object);
	public:
		~Object()override {}
	private:

	};
}