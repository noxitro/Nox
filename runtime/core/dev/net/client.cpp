//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	client.cpp
///	@brief	client
#include	"pch.h"
#include	"client.h"

void nox::dev::net::Client::Initialize(const InitializeContext& context)
{
	initialize_context_ = context;
}