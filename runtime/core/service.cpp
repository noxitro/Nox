// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	service.cpp
/// @brief	service
#include "pch.h"
#include "service.h"

nox::ServiceContext::ServiceContext(
	const std::span<const std::string_view> declared_type_names,
	const std::span<const nox::detail::ServiceLookupEntry> services)noexcept :
	declared_type_names_(declared_type_names),
	services_(services),
	undeclared_type_name_()
{
}

nox::Service* nox::ServiceContext::TryGetDeclared(const std::string_view type_name)noexcept
{
	//	宣言(Depends)に無い型は、登録済みでも渡さない。宣言が依存解析の唯一の入力であり、
	//	宣言外の依存は初期化順が保証されないため。最初の1件だけ覚えて起動失敗の理由にする。
	if (std::ranges::find(declared_type_names_, type_name) == declared_type_names_.end())
	{
		if (undeclared_type_name_.empty() == true)
		{
			undeclared_type_name_ = type_name;
		}
		return nullptr;
	}

	//	Dependsの解決はWorldが初期化前に済ませているので、宣言済みなら必ず見つかる。
	const auto found = std::ranges::find(services_, type_name, &nox::detail::ServiceLookupEntry::type_name);
	return (found != services_.end()) ? found->service : nullptr;
}
