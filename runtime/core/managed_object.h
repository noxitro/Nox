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

		ManagedObject()noexcept;
		~ManagedObject()override;

		/// @brief 参照カウンタ取得
		[[nodiscard]] inline	nox::int32	GetRefCount()const noexcept{ return ref_count_; }
	private:
		/// @brief 参照カウンタ
		volatile nox::int32 ref_count_;
	};

//	template<>
	inline void IntrusivePtrAddReference(ManagedObject&v) 
	{
		v.AddRef();
	}

//	template<>
	inline void IntrusivePtrReleaseReference(ManagedObject&v) 
	{
		v.ReleaseRef();
	}
}


