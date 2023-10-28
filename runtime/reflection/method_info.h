///	@file	method_info.h
///	@brief	method_info
#pragma once
#include	"type.h"

namespace nox::reflection
{
	class MethodInfo
	{
	private:
		/// @brief 関数名
		std::string_view	fullname_;

		AccessLevel access_level_;

		const class ReflectionObject* const* const attribute_ptr_table_;
	};
}