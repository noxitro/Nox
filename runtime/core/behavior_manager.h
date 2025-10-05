//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	behavior_manager.h
///	@brief	behavior_manager
#pragma once
#include	"object.h"

namespace nox
{
	class BehaviorManager : public nox::Object, public nox::ISingleton<BehaviorManager>
	{
		NOX_DECLARE_OBJECT(nox::BehaviorManager, nox::Object);
	private:
		struct BehaviorGroup
		{
			nox::int32 update_order;
			nox::Vector<std::reference_wrapper<class Behavior>> behavior_list;

			inline BehaviorGroup(nox::int32)noexcept:
				update_order(0)
			{

			}
		};

	public:

		void	Initialize();
		void	Update();
		void	LateUpdate();
		void	Finalize();

		void	Register(class Behavior& behavior);
		void	Unregister(class Behavior& behavior);
	private:
		nox::Vector<BehaviorGroup> behavior_group_list_;
		nox::os::Mutex mutex_;
	};
}