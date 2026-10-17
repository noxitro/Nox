//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	service_container.h
///	@brief	service_container
#pragma once

namespace nox
{
	namespace detail
	{
		class ServiceBase;
	}

	class ServiceContainer final
	{
	public:
		template<std::derived_from<nox::detail::ServiceBase> Interface, std::derived_from<Interface> T>
			requires std::is_abstract_v<Interface> && std::is_default_constructible_v<T>
		inline	void Register()
		{
			Register(nox::reflection::Typeof<Interface>(), nox::reflection::Typeof<T>(), *new T());
		}

		template<std::derived_from<nox::detail::ServiceBase> T>
			requires std::is_default_constructible_v<T>
		inline	void Register()
		{
			Register(nox::reflection::Typeof<T>(), *new T());
		}

		template<std::derived_from<nox::detail::ServiceBase> T>
		inline T* Get()const noexcept
		{
			if constexpr (std::is_abstract_v<T>)
			{
				const auto it = interface_to_service_.find(&nox::reflection::Typeof<T>());
				if (it != interface_to_service_.end())
				{
					return static_cast<T*>(it->second);
				}
			}
			else
			{
				const auto it = services_.find(&nox::reflection::Typeof<T>());
				if (it != services_.end())
				{
					return static_cast<T*>(it->second);
				}
			}
			return nullptr;
		}

	private:
		void Register(const nox::reflection::Type& interface_type, const nox::reflection::Type& service_type, nox::detail::ServiceBase& service);
		void Register(const nox::reflection::Type& service_type, nox::detail::ServiceBase& service);

	private:
		nox::UnorderedMap<const nox::reflection::Type*, nox::detail::ServiceBase*> services_;
		nox::UnorderedMap<const nox::reflection::Type*, nox::detail::ServiceBase*> interface_to_service_;
	};
}