//	Copyright (c) 2025 NOX ENGINE All rights reserved.

///	@file	client.cpp
///	@brief	client
#include	"stdafx.h"
#include	"client.h"

void nox::dev::net::Client::Initialize(const InitializeContext& context)
{
	initialize_context_ = context;
	state_ = ConnectionState::Init;
}