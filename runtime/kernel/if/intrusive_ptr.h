///	@file	intrusive_ptr.h
///	@brief	intrusive_ptr
#pragma once

namespace nox
{
	namespace concetps
	{
		template<class T>
		concept IntrusivePtrConcept = requires(T & x)
		{
			x.AddReference();
			x.ReleaseReference();
		};
	}

	/// @brief		侵入型スマートポインタ
	/// @details	リソースカウンタアクセサ、解放メソッドは各自用意
	template<class T>
	class IntrusivePtr
	{
		private:



	};
}