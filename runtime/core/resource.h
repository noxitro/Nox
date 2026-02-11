//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	resource.h
///	@brief	resource
#pragma once
#include	"managed_object.h"
#include	"attribute_common.h"

namespace nox
{
	namespace io
	{
		class Stream
		{
		public:

		};
	}

	class Resource : public nox::ManagedObject
	{
		NOX_DECLARE_OBJECT(Resource, nox::ManagedObject);
	public:
		Resource() {}
		~Resource()override {}

		void initialize(nox::U8StringView path);

	private:
		virtual void onInitialize(const nox::io::Stream& stream) = 0;

	private:
//		nox::String path_;
	};
}