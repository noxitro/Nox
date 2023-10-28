#pragma once
#include	"../if/basic_type.h"

namespace nox::memory
{
	enum class AreaType : u8
	{
		Object,
		ManagedObject,
		Stl,
		Other,
		_Max
	};

	enum class SegmentType : u8
	{
		Default,
		Develop,
	};
}