// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	entity_node.cpp
/// @brief	entity_node
#include "pch.h"
#include "entity_node.h"

#include	"component.h"
#include	"transform.h"

namespace nox
{
	namespace iterator_tag
	{
		struct Tag {};

		struct Input : Tag
		{
		};
		struct Forward : Input
		{
		};
		struct Bidirectional : Forward
		{
		};
		struct RandomAccess : Bidirectional
		{
		};
	}

	namespace detail
	{
		class IteratorFacadeBase {};

		class IteratorBaseInterface {};

		template<class NodeType>
		class IteratorBase : public IteratorBaseInterface
		{
		public:
			inline constexpr IteratorBase()noexcept :
				node_(nullptr)
			{
			}

			inline constexpr explicit IteratorBase(NodeType& node)noexcept :
				node_(node)
			{
			}

		protected:
			NodeType node_;
		};


	}

	template<
		class BaseType,
		auto IncrementFunc,

		class NodeType = std::add_pointer_t<BaseType>,
		class Reference = std::remove_cv_t<BaseType>,
		auto DerefNode = +[](NodeType node)constexpr noexcept->decltype(auto) { return *node; }
	>
	class IteratorForward : public nox::detail::IteratorBase<NodeType>
	{
		using IteratorBaseType = typename nox::detail::IteratorBase<NodeType>;
	public:
		inline constexpr IteratorForward(BaseType& node)noexcept :
			IteratorBaseType(node)
		{
		}

		inline IteratorForward& operator++()noexcept
		{
			NOX_ASSERT(IteratorBaseType::node_ != nullptr, u"nullptrです");
			IncrementFunc(DerefNode(IteratorBaseType::node_));
			return *this;
		}

		inline BaseType operator*()const noexcept
		{
			NOX_ASSERT(IteratorBaseType::node_ != nullptr, u"nullptrです");
			return DerefNode(IteratorBaseType::node_);
		}
	};
}

nox::EntityNode::EntityNode() :
	transform_(nullptr),
	name_{}
{
}

nox::EntityNode::~EntityNode()
{
	transform_->ReleaseRef();
	transform_ = nullptr;
	//	nox::util::SafeDelete(transform_);
}

nox::IntrusivePtr<nox::EntityNode> nox::EntityNode::Create(nox::U8StringView name, const nox::Vec3& pos, const nox::Quat& rotation)
{
	//nox::IntrusivePtr<Component> a;

	nox::EntityNode* const gameObject = new nox::EntityNode();
	gameObject->AddRef();

	constexpr auto nse = std::derived_from< nox::Transform, nox::Component>;
	gameObject->transform_ = gameObject->CreateComponent<nox::Transform>();
	gameObject->transform_->AddRef();

	return nox::IntrusivePtr<nox::EntityNode>(gameObject);
}

void nox::EntityNode::Destroy(nox::EntityNode& gameObject)
{
	gameObject.ReleaseRef();
}

nox::Component* nox::EntityNode::GetComponent(const nox::reflection::Type& type)const noexcept
{
	const nox::reflection::ClassInfo* const class_info = type.GetUserDefinedCompoundTypeInfo();
	if (class_info == nullptr)
	{
		return nullptr;
	}

	for (Component* component = transform_; component != nullptr; component = component->GetComponentChain())
	{
		if (component->GetType().GetUserDefinedCompoundTypeInfo()->IsBaseOf(*class_info) == true)
		{
			return component;
		}
	}
	return nullptr;
}

nox::Component* nox::EntityNode::GetSameComponent(const nox::reflection::Type& type)const noexcept
{
	for (Component* component = transform_; component != nullptr; component = component->GetComponentChain())
	{
		if (component->GetType() == type)
		{
			return component;
		}

	}
	return nullptr;
}

nox::Component* nox::EntityNode::CreateComponent(const nox::reflection::Type& type)
{
	nox::Component* const component = static_cast<nox::Component*>(type.CreateObject());
	if (component == nullptr)
	{
		return nox::IntrusivePtr<Component>();
	}

	for (Component* component = transform_; component != nullptr; component = component->GetComponentChain())
	{
		//		if()
	}

	component->Loaded();

	return nox::IntrusivePtr(component);
}