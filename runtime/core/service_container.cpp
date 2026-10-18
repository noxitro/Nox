//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	service_container.cpp
///	@brief	service_container
#include	"pch.h"
#include	"service_container.h"


void nox::ServiceContainer::Register(const nox::reflection::Type& interface_type, const nox::reflection::Type& service_type, nox::detail::ServiceBase& service)
{
	NOX_ASSERT(interface_to_service_.find(&interface_type) == interface_to_service_.end(), u8"すでに登録済みのサービスです:{0}", interface_type.GetTypeName());
	Register(service_type, service);
	interface_to_service_.emplace(&interface_type, &service);
}

void nox::ServiceContainer::Register(const nox::reflection::Type& service_type, nox::detail::ServiceBase& service)
{
	NOX_ASSERT(services_.find(&service_type) == services_.end(), u8"すでに登録済みのサービスです:{0}", service_type.GetTypeName());
	services_.emplace(&service_type, &service);
}