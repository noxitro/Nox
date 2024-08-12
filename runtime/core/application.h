//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	application.h
///	@brief	application
#pragma once
#include	"object.h"
#include	"attribute_common.h"
namespace nox
{
#if clang

#endif // clang

	class 
		NOX_ATTR_TYPE(::nox::attr::dev::Description(U"Application"), nox::attr::dev::DisplayName(U"アプリケーション"))
		Application : public Object, public ISingleton<Application>
	{
		friend class ISingleton<Application>;
		NOX_DECLARE_OBJECT(Application, Object);
	public:
		inline Application() {}

		void	Initialize();
		void	Run();
		void	Finalize();

	private:
		inline	void	Update();
	};
}