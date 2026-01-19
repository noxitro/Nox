//	Copyright (C) 2026 NOX ENGINE All rights reserved.

///	@file	entry.h
///	@brief	entry
#pragma once

namespace nox::render
{
	class RenderEntry : public nox::ModuleEntry
	{
		NOX_DECLARE_OBJECT(nox::render::RenderEntry, nox::ModuleEntry);
	public:
		RenderEntry();
		~RenderEntry()noexcept = default;
	};
}