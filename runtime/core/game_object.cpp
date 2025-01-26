//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	game_object.cpp
///	@brief	game_object
#include	"stdafx.h"
#include	"game_object.h"

#include	"component.h"
#include	"transform.h"

namespace nox
{
	namespace iterator_tag
	{
		struct Tag {};

		struct Input : Tag
		{};
		struct Forward : Input
		{};
		struct Bidirectional : Forward
		{};
		struct RandomAccess : Bidirectional
		{};
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
			{}

			inline constexpr explicit IteratorBase(NodeType& node)noexcept :
				node_(node)
			{}

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
		{}

		inline IteratorForward& operator++()noexcept
		{
			NOX_ASSERT(IteratorBaseType::node_ != nullptr, U"nullptrです");
			IncrementFunc(DerefNode(IteratorBaseType::node_));
			return *this;
		}

		inline BaseType operator*()const noexcept
		{
			NOX_ASSERT(IteratorBaseType::node_ != nullptr, U"nullptrです");
			return DerefNode(IteratorBaseType::node_);
		}
	};
}

nox::GameObject::GameObject():
	transform_(nullptr),
	name_{}
{
}

nox::GameObject::~GameObject()
{
	transform_->ReleaseRef();
	transform_ = nullptr;
	//	nox::util::SafeDelete(transform_);
}

nox::IntrusivePtr<nox::GameObject> nox::GameObject::Create(nox::StringView name, const nox::Vec3& pos, const nox::Quat& rotation)
{
	//nox::IntrusivePtr<Component> a;

	nox::GameObject* const gameObject = new nox::GameObject();
	gameObject->AddRef();

	gameObject->transform_ = gameObject->CreateComponent<nox::Transform>().Get();
	gameObject->transform_->AddRef();

	return nox::IntrusivePtr<nox::GameObject>(gameObject);
}

void nox::GameObject::Destroy(nox::GameObject& gameObject)
{
	gameObject.ReleaseRef();
}

nox::Component* nox::GameObject::GetComponent(const nox::reflection::Type& type)noexcept
{
	const nox::reflection::ClassInfo* const class_info = type.GetUserDefinedCompoundTypeInfo();
	if (class_info == nullptr)
	{
		return nullptr;
	}

	for (Component* component = transform_; component != nullptr; component = component->GetComponentChain())
	{
		if (component->GetUnderlyingType().GetUserDefinedCompoundTypeInfo()->IsBaseOf(*class_info) == true)
		{
			return component;
		}
	}
	return nullptr;
}

nox::Component* nox::GameObject::GetSameComponent(const nox::reflection::Type& type)noexcept
{
	for (Component* component = transform_; component != nullptr; component = component->GetComponentChain())
	{
		if (component->GetUnderlyingType() == type)
		{
			return component;
		}

	}
	return nullptr;
}

nox::IntrusivePtr<nox::Component> nox::GameObject::CreateComponent(const nox::reflection::Type& type)
{
	nox::Component*const component = static_cast<nox::Component*>(type.CreateObject());
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