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
		class ISingletonBase
		{
			friend class SingletonManager;
		public:
			inline constexpr ISingletonBase()noexcept:
				next_(nullptr),
				prev_(nullptr)
			{}

		private:
			ISingletonBase* next_;
			ISingletonBase* prev_;
		};

		class SingletonManager
		{
		public:
			static	void	Register(ISingletonBase& obj);
			static	void	Unregister(ISingletonBase& obj);

			static	void	CheckReak();
		private:
			SingletonManager() = delete;
			~SingletonManager() = delete;

		private:
			static inline constinit ISingletonBase* root_ = nullptr;
		};

		void	CheckSingletonCreateInstance(void* instance_ptr, std::string_view type_name)noexcept(false);
		void	CheckSingletonDeleteInstance(void* instance_ptr, std::string_view type_name)noexcept(false);
	}

	/// @brief Singletonインターフェース
	/// @tparam T 
	template<class T> requires(
		(std::is_class_v<T>)
		)
		class ISingleton : public nox::detail::ISingletonBase
	{
	public:
		static	inline	void	CreateInstance()noexcept(false) {
			nox::detail::CheckSingletonCreateInstance(instance_, nox::util::GetTypeName<T>());
			instance_ = new T();

			nox::detail::SingletonManager::Register(*instance_);
		}
		static	inline	void	DeleteInstance()noexcept(false) {
			nox::detail::SingletonManager::Unregister(*instance_);

			nox::detail::CheckSingletonDeleteInstance(instance_, nox::util::GetTypeName<T>());
			delete instance_;
			instance_ = nullptr;
		}
		static	inline	bool	HasInstance()noexcept { return instance_ != nullptr; }
		static	inline	decltype(auto) Instance()noexcept { return util::Deref(instance_); }

	protected:
		inline ISingleton()noexcept(false)
		{
			NOX_ASSERT(ISingleton<T>::instance_ == nullptr, U"Singleton instance is already created");
		}

	private:
		static inline T* instance_ = nullptr;
	
	};

}