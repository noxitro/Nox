//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	render_resource.h
///	@brief	render_resource
#pragma once

namespace nox::render
{
	class RenderResource : public nox::Object
	{
		NOX_DECLARE_OBJECT(nox::render::RenderResource, nox::Object);
	public:
		inline constexpr RenderResource()noexcept:
			ref_count_(0)
		{
		}

		inline constexpr ~RenderResource()override {}

		void AddRef();
		void Release();

	private:
		volatile nox::int32 ref_count_;
	};
}