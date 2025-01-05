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
		NOX_ATTR_TYPE(::nox::attr::dev::Description(U"Application"), nox::attr::dev::DisplayName(U"アプリケーション"))
		Application : public Object, public ISingleton<Application>
	{
		NOX_DECLARE_OBJECT(Application, Object);
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
			const nox::ModuleEntryCategory priority;
			void(*func)(nox::ModuleEntry&);
			nox::ModuleEntry& entry;
			

			/*inline constexpr ModuleEntryInfo(
				nox::ModuleEntryCategory _priority, 
				void(nox::ModuleEntry::*_func)(), 
				nox::ModuleEntry& _entry)noexcept
				: priority(_priority), func(_func), entry(&_entry)
			{
			}

			inline constexpr ModuleEntryInfo(const ModuleEntryInfo& info)noexcept
				: priority(info.priority), func(info.func), entry(info.entry)
			{
			}

			inline constexpr ModuleEntryInfo(ModuleEntryInfo&& rhs)noexcept:
				ModuleEntryInfo(rhs)
			{

			}*/

		};

	public:
		Application()noexcept;
		~Application()override;

		void	Run();

		void	RegisterModuleEntry(void(*func)(nox::ModuleEntry&), nox::ModuleEntry& entry,const nox::ModuleEntryCategory type);
	private:
		inline	void	Init();
		inline	void	Update();
		inline	void	Exit();
		inline static constexpr UpdateCategory	ToUpdateCategory(nox::ModuleEntryCategory category)noexcept;
		inline	void	InvokeModuleEntry(const UpdateCategory category);
	private:
		std::array<nox::Vector<ModuleEntryInfo>, nox::util::ToUnderlying(UpdateCategory::_Max)> module_entry_info_list_table_;

		nox::Vector<std::reference_wrapper<nox::ModuleEntry>> module_entry_list_;

		/// @brief モジュールエントリ重複チェック用ビットセット
		std::bitset<nox::util::ToUnderlying(nox::ModuleEntryCategory::_Max)> module_entry_bitset_;
	};
}