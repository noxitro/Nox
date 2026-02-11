//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	update_order_attribute.h
///	@brief	update_order_attribute
#pragma once
#include	"attribute.h"

namespace nox::attr
{
	class UpdateOrder : public nox::attr::Attribute
	{
		NOX_DECLARE_OBJECT(UpdateOrder, nox::attr::Attribute);
	public:
		inline constexpr explicit UpdateOrder(nox::int32 priority)noexcept :
			priority_(priority)
		{
		}

		inline constexpr nox::int32 GetPriority()const noexcept { return priority_; }
	private:
		const nox::int32 priority_;
	};

}