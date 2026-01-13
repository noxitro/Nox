//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	application.cpp
///	@brief	application
//import std;
#include	"stdafx.h"
#include	"application.h"
#include	"module_entry.h"

namespace
{
	inline	void HookException(nox::uint32 code, ::_EXCEPTION_POINTERS* const exception_ptr)
	{
	}
}

namespace nox::os
{
	void Update()
	{
		::MSG msg;

		while (true)
		{
			if (::PeekMessageW(&msg, nullptr, 0U, 0U, PM_NOREMOVE))
			{
				if (!::GetMessageW(&msg, nullptr, 0U, 0U))
				{
					break;
				}
				::TranslateMessage(&msg);
				::DispatchMessageW(&msg);
			}
		}
	}
}

nox::Application::Application()noexcept :
	module_entry_bitset_{},
	enabled_vsync_(false),
	target_frame_rate_(60),
	kill_(false),
	window_(nullptr)
{
}

nox::Application::~Application()
{
	nox::util::SafeDelete(window_);
}

void	nox::Application::Init()
{
	//	windowを生成
	{
		nox::os::WindowSetupDesc desc;
		desc.width = 1280;
		desc.height = 720;
		desc.window_style = nox::os::WindowStyle::Normal;
		desc.title_ptr = u"NOX ENGINE Application";

		window_ = &nox::os::Window::Create(desc);
		window_->Show();
	}

	//	モジュールエントリクラス群を収集
	nox::reflection::ForeachDerivedClassInfoList(nox::reflection::Typeof<nox::ModuleEntry>(),
		[this](const nox::reflection::ClassInfo& class_info) {

			nox::ModuleEntry* module_entry = static_cast<nox::ModuleEntry*>(class_info.GetType().CreateObject());
			module_entry_list_.emplace_back(*module_entry);
		});
}

void	nox::Application::Run()
{
	this->Init();

	for (const ModuleEntryInfo& entry_info : module_entry_info_list_table_[nox::util::ToUnderlying(UpdateCategory::Init)])
	{
		entry_info.func(*entry_info.entry);
	}

	for (const ModuleEntryInfo& entry_info : module_entry_info_list_table_[nox::util::ToUnderlying(UpdateCategory::Start)])
	{
		entry_info.func(*entry_info.entry);
	}

	stop_watch_.Start();

	nox::os::Thread game_thread;
	game_thread.SetThreadName(u"Game");
	game_thread.Dispatch([this]() {

		while (!this->kill_)
		{
			try
			{
				this->Update();
			}
			catch (const std::exception&)
			{
				//	
				break;
			}
		}
		});

	nox::os::Update();
	//	ここを抜けたらkill
	kill_ = true;

	game_thread.Wait();

	for (const ModuleEntryInfo& entry_info : module_entry_info_list_table_[nox::util::ToUnderlying(UpdateCategory::Terminal)])
	{
		entry_info.func(*entry_info.entry);
	}

	for (const ModuleEntryInfo& entry_info : module_entry_info_list_table_[nox::util::ToUnderlying(UpdateCategory::Finalize)])
	{
		entry_info.func(*entry_info.entry);
	}

	Exit();
}

void	nox::Application::InvokeModuleEntry(const UpdateCategory category)
{

}

void nox::Application::SetVSync(bool flag)noexcept
{
	enabled_vsync_ = flag;
}

void	nox::Application::Update()
{
	elapsed_milli_seconds_ = stop_watch_.ElapsedMilliseconds();
	if (enabled_vsync_)
	{
		if (elapsed_milli_seconds_ < next_elapsed_milli_seconds_)
		{
			nox::os::Thread::Sleep(1);
			return;
		}
	}

	for (const ModuleEntryInfo& entry_info : module_entry_info_list_table_[nox::util::ToUnderlying(UpdateCategory::Update)])
	{
		entry_info.func(*entry_info.entry);
	}

	++frame_counter_;

	//	次のフレーム更新時間
	next_elapsed_milli_seconds_ += (1000.0f / static_cast<nox::float_t>(target_frame_rate_));

	stop_watch_.Restart();
}

void	nox::Application::Exit()
{
	kill_ = true;
	for (nox::uint32 i = 0; i < module_entry_list_.size(); ++i)
	{
		nox::ModuleEntry& entry = module_entry_list_[i];
		delete (&entry);
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

void	nox::Application::RegisterModuleEntry(void(*func)(nox::ModuleEntry&), nox::ModuleEntry& entry, const nox::ModuleEntryCategory type)
{
	nox::Vector<ModuleEntryInfo>& vector = module_entry_info_list_table_[nox::util::ToUnderlying(ToUpdateCategory(type))];
	vector.emplace_back(ModuleEntryInfo{ .priority = type, .func = func, .entry = &entry});

	//	重複チェック

	NOX_ASSERT(module_entry_bitset_.test(nox::util::ToUnderlying(type)) == false, u"重複エントリ:{0}", (int)type);
	module_entry_bitset_.set(nox::util::ToUnderlying(type));
}