//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	managed_object.h
///	@brief	managed_object
#pragma once
#include	"object.h"

namespace nox
{
	/// @brief ガベージコレクション対象基底クラス
	class ManagedObject : public Object
	{
		NOX_DECLARE_MANAGED_OBJECT(ManagedObject, Object);

	public:

	};
}