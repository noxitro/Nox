//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	module_entry.h
///	@brief	module_entry
#pragma once
#include	"module_entry_category.h"
#include	"object.h"

namespace nox
{
	/// @brief モジュールエントリ基底クラス
	class ModuleEntry : public nox::Object
	{
		NOX_DECLARE_OBJECT(nox::ModuleEntry, nox::Object);
	public:
		inline constexpr ModuleEntry()noexcept = default;
		virtual ~ModuleEntry() = default;

		/// @brief 登録
		virtual void Entry() = 0;

	protected:
		template<std::derived_from<nox::ModuleEntry> T>
		inline	void	Register(void(T::* func)(), const nox::ModuleEntryCategory type)
		{
			Register(static_cast<void(nox::ModuleEntry::*)()>(func), type);
		}
	
	private:
		void	Register(void(ModuleEntry::* func)(), const nox::ModuleEntryCategory type);
	};
}