//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	singleton.h
///	@brief	singleton
#pragma once
#include	"assertion.h"

#include	"advanced_definition.h"
#include	"type_traits/type_name.h"

namespace nox
{
	namespace detail
	{
		void	CheckSingletonCreateInstance(void* instance_ptr, std::string_view type_name)noexcept(false);
		void	CheckSingletonDeleteInstance(void* instance_ptr, std::string_view type_name)noexcept(false);
	}

	/// @brief Singletonインターフェース
	/// @tparam T 
	template<class T> requires(
		(std::is_class_v<T> || std::is_union_v<T>)
		)
		class ISingleton
	{
	public:
		static	inline	void	CreateInstance()noexcept(false) {
			nox::detail::CheckSingletonCreateInstance(instance_, nox::util::GetTypeName<T>());
			instance_ = new T();
		}
		static	inline	void	DeleteInstance()noexcept(false) {
			nox::detail::CheckSingletonDeleteInstance(instance_, nox::util::GetTypeName<T>());
			delete instance_;
			instance_ = nullptr;
		}
		static	inline	bool	HasInstance()noexcept { return instance_ != nullptr; }
		static	inline	T& Instance()noexcept { return util::Deref(instance_); }

	private:
		static inline T* instance_ = nullptr;
	};

}