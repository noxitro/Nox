//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	application.cpp
///	@brief	application
//import std;
#include	"stdafx.h"
#include	"application.h"
#include	"../kernel/multicast_delegate.h"

//import nox.math;

using namespace nox;

namespace
{
	inline	void HookException(uint32 code, ::_EXCEPTION_POINTERS* const exception_ptr)
	{

	}
}

void	Application::Initialize()
{
}

struct RuObj
{
//	RuObj(int v) {}

//	void Func() {}
	//[[__attribute__((alloc_align([]()constexpr qnoexcept {return 0; }())))]]
	
	int value = 12;

	void func(int a) {}

	enum class E
	{
		
		A,
	};
};

namespace
{
//	constexpr nox::reflection::FunctionArgParameter arg00(u"v", nox::reflection::TypeKind::Int32);
}

void	Application::Run()
{
	os::Thread game_thread;
	game_thread.SetThreadName(u"Game");
	game_thread.Dispatch([this]() {

		while (true)
		{
			try
			{
				this->Update();
				break;
			}
			catch (const std::exception&)
			{
				//	
			}
		}
		});

	game_thread.Wait();
}

void	Application::Finalize()
{
}

inline	void	Application::Update()
{
	try
	{
		NOX_ASSERT(false, U"🐘さんだね");

	}
	catch (const std::exception&)
	{

	}
}