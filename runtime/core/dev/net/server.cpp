//	Copyright (c) 2025 NOX ENGINE All rights reserved.

///	@file	server.cpp
///	@brief	server
#include	"stdafx.h"
#include	"server.h"

void nox::dev::net::Server::Initialize(const InitializeContext& context)
{
	initialize_context_ = context;
	shutdown_ = false;
	phase_ = Phase::None;
	is_error_ = false;
}