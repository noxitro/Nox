// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	system.cpp
/// @brief	system
#include "pch.h"
#include "system.h"

namespace nox
{
	namespace
	{

		struct HpComponent
		{
			nox::float32 current;
			nox::float32 max;
		};

		struct MpComponent
		{
			nox::float32 current;
			nox::float32 max;
		};

		class TestSystem00 : nox::ComponentSystem<HpComponent, const MpComponent>
		{
			void OnUpdate(Manager& manager) override
			{
				manager.ForEach([](nox::EntityId entity, HpComponent& hp, const MpComponent& mp)
					{
						hp.current += 1.0f;
						if (hp.current > hp.max)
						{
							hp.current = hp.max;
						}
					});
			}
		};

		class INewSystemBase
		{
		public:
			void Update() { OnUpdate(); }
		private:
			virtual void OnUpdate() {}
		};

		template<class T>
		class NewSystemBase : public nox::INewSystemBase
		{
		public:
			inline constexpr NewSystemBase()
			{
				CheckDerived();
			}
		private:
			inline consteval void CheckDerived()const noexcept
			{
				//	OnExecute()を実装していない場合はコンパイルエラーにする
				std::is_invocable_v<decltype(&T::OnExecute), T>;
			}

			void OnUpdate()override
			{
				static_cast<T*>(this)->OnExecute();
			}
		};

		class NewSystemTest : public nox::NewSystemBase<NewSystemTest>
		{
		public:
			void OnExecute(const nox::EntityId entity, const HpComponent& hp, const MpComponent& mp, const Service*const serviceA)
			{
			}
		};

		inline void testFunc00()
		{
			NewSystemTest system;
		}
	}
}