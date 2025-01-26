//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	easing.h
///	@brief	イージング関数
#pragma once
#include	"basic_type.h"
#include	"type_traits/concepts.h"

namespace nox
{
	namespace easing
	{
		enum class EasingType : nox::char8
		{
			Linear,
			QuadraticIn,
			QuadraticOut,
			QuadraticInOut,
			CubicIn,
			CubicOut,
			CubicInOut,
			QuarticIn,
			QuarticOut,
			QuarticInOut,
			QuinticIn,
			QuinticOut,
			QuinticInOut,
			SineIn,
			SineOut,
			SineInOut,
			ExponentialIn,
			ExponentialOut,
			ExponentialInOut,
			CircularIn,
			CircularOut,
			CircularInOut,
			ElasticIn,
			ElasticOut,
			ElasticInOut,
			BackIn,
			BackOut,
			BackInOut,
			BounceIn,
			BounceOut,
			BounceInOut,
		};
	}
}