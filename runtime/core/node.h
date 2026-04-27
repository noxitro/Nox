// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	node.h
/// @brief	node
#pragma once
#include	"managed_object.h"

namespace nox
{
	class Application;
	class Node : public nox::ManagedObject
	{
		NOX_DECLARE_OBJECT(Node, nox::ManagedObject);
	public:
		inline nox::IntrusivePtr<nox::Node> GetParent()const noexcept { return parent_; }

		void SetApplication(nox::Application& application)noexcept
		{
			application_ = &application;
		}

		nox::Application& GetApplication()const noexcept { return *application_; }
	protected:
		inline Node() noexcept : 
			parent_(nullptr), 
			application_(nullptr) {}

	private:
		
		Node* parent_;
		nox::Application* application_;
	};
}