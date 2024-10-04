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
		void	AddRef();
		void	ReleaseRef();

		ManagedObject();
		~ManagedObject()override;

	private:
		/// @brief 参照カウンタ
		volatile int32 ref_count_;
	};

	template<>
	inline void IntrusivePtrAddReference< ManagedObject>(ManagedObject&v) 
	{
		v.AddRef();
	}

	template<>
	inline void IntrusivePtrReleaseReference< ManagedObject>(ManagedObject&v) 
	{
		v.ReleaseRef();
	}
}


