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

	protected:
		/// @brief 関数の登録
		/// @tparam member_function_addr 
		/// @param category 
		template<auto member_function_addr> requires(std::is_member_function_pointer_v<decltype(member_function_addr)>)
		inline	void	Register(const nox::ModuleEntryCategory category)
		{
			RegisterImpl(+[](nox::ModuleEntry& entry)
				{
				using T = nox::FunctionClassType<decltype(member_function_addr)>;
				T& entry_impl = static_cast<T&>(entry);
				(entry_impl.*member_function_addr)();

				}, category);
		}
	
	private:
		void	RegisterImpl(void(*func)(nox::ModuleEntry&), const nox::ModuleEntryCategory type);
	};
}