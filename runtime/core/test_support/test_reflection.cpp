//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	test_reflection.cpp
///	@brief	test_reflection
#include	"pch.h"
#include	"test_reflection.h"
#include	"../reflection_generated/support_functions.h"


template<class R>
struct FuncHolder
{
	using FuncType = R(*)(void**);

	FuncType func;
	int numArgument;
};

int TestInvokeFunctionWarpper_0(void** args)
{
	int a = *static_cast<int*>(args[0]);
	int b = *static_cast<int*>(args[1]);
	return a + b;
}
int TestInvokeFunctionWarpper_1(void** args)
{
	int a = *static_cast<int*>(args[0]);
	return {};
}


constexpr std::tuple<int(*)(void**), int> holder_list[2] ={
	{ +[](void**)->int {return{}; } , 2},
	{ TestInvokeFunctionWarpper_1, 1 }
};

class Game
{
public:
	virtual void abst() = 0;
	int& a;
};

template<class T = int, class F, class... Args>
inline constexpr auto TestFunctor(F&& f, Args&&... args)
{
	return std::forward<F>(f).template operator() < T > (std::forward<Args>(args)...);
}

void nox::test::TestReflection()
{

	//using f = nox::ToMemberFunctionPointerType<nox::StringView(std::span<nox::char32>) const, nox::Object>;
	//static_cast<f>(& nox::Object::ToString);
	//static_cast<nox::ToMemberFunctionPointerType<nox::StringView(std::span<nox::char32>) const, nox::Object>>(nox::Object::ToString) > ;
	//nox::GetFunctionPointerId<static_cast<nox::ToMemberFunctionPointerType<nox::StringView(std::span<nox::char32>) const, nox::Object>>(&nox::Object::ToString)>(),	//	function_id
	////nox::ToMemberFunctionPointerType<void()const, nox::Object>;

	////	getter
	//void* out{};
	//void* instance{};
	//const void* const value{};
	//using VariableType = decltype(nox::ManagedObject::ref_count_);
	//using ClassType = nox::ManagedObject;
	//if constexpr (nox::concepts::Assignable<VariableType, std::remove_const_t<VariableType>>)
	//{
	//	*static_cast<const std::decay_t<VariableType>**>(out) = &static_cast<const ClassType*>(instance)->nox::ManagedObject::ref_count_;
	//	auto n = static_cast<const std::decay_t< VariableType>**>(&static_cast<const ClassType*>(instance)->nox::ManagedObject::ref_count_);
	//}

	////	getter
	//void* out{};
	//const void* instance{};
	//{
	//	using VariableType = decltype(nox::attr::dev::Description::description_);
	//	using ClassType = nox::attr::dev::Description;
	//	if constexpr (std::convertible_to<VariableType, std::remove_const_t<VariableType>>)
	//	{
	//		*static_cast<std::remove_const_t<std::decay_t<VariableType>>*>(out) = static_cast <const ClassType*> (instance)->nox::attr::dev::Description::description_;
	//	}
	//}
	//	
	//{
	//		using VariableType = decltype(nox::attr::dev::Description::description2_);
	//		using ClassType = const nox::attr::dev::Description;
	//		if constexpr (std::convertible_to<VariableType, std::remove_const_t<VariableType>>)
	//		{
	//			
	//			*static_cast<std::remove_reference_t<VariableType>**>(out) = &static_cast<ClassType*>(instance)->nox::attr::dev::Description::description2_;
	//			auto n2 = &static_cast<ClassType*>(instance)->nox::attr::dev::Description::description2_;
	//		}
	//}

	/*
	* void* out{};
	const void* instance{};
	using VariableType = decltype(nox::Behavior::enabled_function_types_);
	using ClassType = const nox::Behavior;
	if constexpr (std::convertible_to<VariableType, std::remove_const_t<VariableType>>)
	{
		*static_cast<std::remove_reference_t<VariableType>**>(out) = &static_cast<const ClassType*>(instance)->nox::Behavior::enabled_function_types_;
	}
	*/

	//	constexpr Desc d = Desc{.v = std::ref(vv)};

	/*{
		class LC
		{
		public:
			LC(int v) :value(v) {}
			LC(const LC& other) :value(other.value) 
			{
				NOX_INFO_LINE_OLD(U"LC Copy Constructor");
			}
			LC(LC&& other) :value(other.value) 
			{
				other.value = 0;
				NOX_INFO_LINE_OLD(U"LC Move Constructor");
			}

			~LC()
			{
				NOX_INFO_LINE_OLD(U"LC Destructor");
			}

			int Func(int a)const noexcept { return a + value; }
			inline int operator()(int v)const { return this->value + v; }

		private:
			int value = 0;
		};

		class LC2
		{
		public:
			LC2(int& v) :value_(v) {}

			LC2(const LC2& other) :value_(other.value_)
			{
			}

			int& value_;
		};

		int v = 123;
		const LC2 lc2(v);
		const LC2& lc2Ref = lc2;
		LC2* lc3 = nullptr;
		char buffer[sizeof(LC2)]{ 0 };
		lc3 = std::construct_at(reinterpret_cast<LC2*>(buffer), lc2Ref.value_);

		delegate = std::make_pair(nox::Nontype<&LC::Func>, LC(123));

		constexpr bool eaa = std::is_same<void(LC2::*)()&, void(LC2::*)()>::value;

	}*/

//	auto n = delegate(2);
	

//	auto local_class_unique_ptr = std::make_unique<LocalClass>(123);
//	std::shared_ptr<LocalClass> local_class_shared_ptr(local_class_unique_ptr.get());
//
//	LocalClass local_class(8);
//	const LocalClass local_class_const = LocalClass(123);
//
//	std::is_invocable<decltype(&LocalClass::Func), const std::shared_ptr<LocalClass>>::value;
////	using t = nox::util::ToAddressType<LocalClass>;
////	nox::InvokeResultTypeWithTupleLike<decltype(&LocalClass::Func), nox::TupleCatType<t, std::tuple<>>>;
//
//	decltype(auto) p = std::make_pair(nox::Nontype<&LocalClass::Func>, local_class);
//	using t2 = typename decltype(p)::second_type;
//
//	nox::Delegate<int(), 32> delegate2 = nullptr;
//	nox::util::TryToAddress(local_class);
//	delegate2 = std::make_pair(nox::Nontype<&LocalClass::Func>, std::ref(local_class));
//	auto n3 = delegate2;
//	auto n = delegate2();



}

