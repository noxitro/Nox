// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	node.h
/// @brief	node
#pragma once
#include	"object.h"

namespace nox
{
	class Application;
	class EntityNode;
	class Node : public nox::Object
	{
		NOX_DECLARE_OBJECT(Node, nox::Object);
	public:
		inline nox::IntrusivePtr<nox::Node> GetParent()const noexcept { return parent_; }
		inline void SetParent(nox::Node* parent)noexcept { parent_ = parent; }

		inline void SetApplication(nox::Application& application)noexcept
		{
			application_ = &application;
		}

		inline nox::Application& GetApplication()const noexcept { return *application_; }
		inline nox::Application* GetApplicationPtr()const noexcept { return application_; }
	protected:
		inline Node() noexcept : 
			parent_(nullptr), 
			application_(nullptr) {}

	private:
		
		Node* parent_;
		nox::Application* application_;
	};
}
