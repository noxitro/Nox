//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	application.h
///	@brief	application
#pragma once
#include	"object.h"
#include	"attribute_common.h"

#include	"module_entry_category.h"

namespace nox
{
	class ModuleEntry;

	/// @brief Coreの管理クラス
	class 
		NOX_ATTR_TYPE(::nox::attr::dev::Description(u"Application"), nox::attr::dev::DisplayName(u"アプリケーション"))
		Application : public nox::Object, public nox::ISingleton<Application>
	{
		NOX_DECLARE_OBJECT(Application, nox::Object);
	private:
		enum class UpdateCategory : uint8
		{
			Init,
			Setup,
			Start,
			Update,
			Terminal,
			Finalize,
			_Max
		};

		struct ModuleEntryInfo
		{
			nox::ModuleEntryCategory priority;
			void(*func)(nox::ModuleEntry&);
			nox::not_null<nox::ModuleEntry*> entry;
		};

	public:
		Application()noexcept;
		~Application()override;
		
		void	Run();

		void	RegisterModuleEntry(void(*func)(nox::ModuleEntry&), nox::ModuleEntry& entry,const nox::ModuleEntryCategory type);

		inline	constexpr nox::uint32 GetFrameCount()const noexcept { return frame_counter_; }
		inline	constexpr nox::uint16 GetTargetFrameRate()const noexcept { return target_frame_rate_; }
		inline	constexpr bool EnabledVSync()const noexcept { return enabled_vsync_; }
		void SetVSync(bool flag)noexcept;

		inline	constexpr bool IsKill()const noexcept { return kill_; }
	private:
		inline	void	Init();
		inline	void	Update();
		inline	void	Exit();
		inline static constexpr UpdateCategory	ToUpdateCategory(nox::ModuleEntryCategory category)noexcept;
		inline	void	InvokeModuleEntry(const UpdateCategory category);
	private:
		bool kill_;
		nox::uint16 target_frame_rate_;
		bool enabled_vsync_;

		nox::StopWatch stop_watch_;
		nox::uint32 frame_counter_;
		nox::float_t elapsed_milli_seconds_;
		nox::float_t next_elapsed_milli_seconds_;

		std::array<nox::Vector<ModuleEntryInfo>, nox::util::ToUnderlying(UpdateCategory::_Max)> module_entry_info_list_table_;

		nox::Vector<std::reference_wrapper<nox::ModuleEntry>> module_entry_list_;

		/// @brief モジュールエントリ重複チェック用ビットセット
		std::bitset<nox::util::ToUnderlying(nox::ModuleEntryCategory::_Max)> module_entry_bitset_;
	};
}