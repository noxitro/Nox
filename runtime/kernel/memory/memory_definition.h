#pragma once
#include	"../if/basic_type.h"

namespace nox::memory
{
	enum class AreaType : uint8
	{
		Object,
		ManagedObject,
		Stl,
		Other,
		_Max
	};

	enum class SegmentType : uint8
	{
		Default,
		Develop,
	};
}