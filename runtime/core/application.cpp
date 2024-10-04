//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	application.cpp
///	@brief	application
//import std;
#include	"stdafx.h"
#include	"application.h"
#include	"module_entry.h"
//import nox.math;

//	test
#include	"core_entry.h"

namespace
{
	inline	void HookException(nox::uint32 code, ::_EXCEPTION_POINTERS* const exception_ptr)
	{
		
	}
}

void	nox::Application::Init()
{
	//	モジュールエントリクラス群を収集
	//	auto class_info = nox::reflection::Reflection::Instance().FindClassInfo<nox::ModuleEntry>();
	nox::ModuleEntry* const core_entry = static_cast<nox::ModuleEntry*>(nox::reflection::Typeof<nox::CoreEntry>().CreateObject());
	if (core_entry != nullptr)
	{
		core_entry->Entry();

		module_entry_list_.emplace_back(*core_entry);
	}

	//	モジュールエントリの準備


}

void	nox::Application::Run()
{
	this->Init();

	decltype(auto) list = module_entry_info_list_table_[nox::util::ToUnderlying(UpdateCategory::Init)];

	for (auto&& entry_info : list)
	{
		(*entry_info.entry.*entry_info.func)();
		entry_info.Invoke();
	}

	//nox::os::Thread game_thread;
	//game_thread.SetThreadName(u"Game");
	//game_thread.Dispatch([this]() {

	//	while (true)
	//	{
	//		try
	//		{
	//			this->Update();
	//		}
	//		catch (const std::exception&)
	//		{
	//			//	
	//			break;
	//		}
	//	}
	//	});

	//game_thread.Wait();

	Exit();
}

void	nox::Application::InvokeModuleEntry(const UpdateCategory category)
{

}

void	nox::Application::Update()
{

}

void	nox::Application::Exit()
{
	for (nox::ModuleEntry& entry : module_entry_list_)
	{
		nox::util::SafeDelete(&entry);
	}

	module_entry_list_.clear();
	module_entry_list_.shrink_to_fit();
}

constexpr nox::Application::UpdateCategory	nox::Application::ToUpdateCategory(nox::ModuleEntryCategory category)noexcept
{
	if (category < nox::ModuleEntryCategory::_Setup)
	{
		return UpdateCategory::Init;
	}
	if (category < nox::ModuleEntryCategory::_Start)
	{
		return UpdateCategory::Setup;
	}
	if (category < nox::ModuleEntryCategory::_Update)
	{
		return UpdateCategory::Start;
	}
	if (category < nox::ModuleEntryCategory::_Terminal)
	{
		return UpdateCategory::Update;
	}
	if (category < nox::ModuleEntryCategory::_Finalize)
	{
		return UpdateCategory::Terminal;
	}
	return UpdateCategory::Finalize;
}

void	nox::Application::RegisterModuleEntry(void(nox::ModuleEntry::*const func)(), nox::ModuleEntry& entry, const nox::ModuleEntryCategory type)
{
	nox::Vector<ModuleEntryInfo>& vector = module_entry_info_list_table_[nox::util::ToUnderlying(ToUpdateCategory(type))];
	vector.emplace_back(ModuleEntryInfo{ type, func, entry  });
}