//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	main.cpp
///	@brief	main

template<class T>
class TClass {};

inline auto testFunc()noexcept
{
	TClass<int> value;
	return sizeof(value);
}

#include	<cstdint>
#include	<type_traits>
#include	<vector>
//template<typename T>
//class Outer {
//public:
//	T outer_value;
//
//	template<typename U>
//	class Inner {
//	public:
//		U inner_value;
//
//		Inner(U value) : inner_value(value) {}
//
//		void display() {
//		}
//	};
//};
//
//using OI = Outer<int>::Inner<double>;
//Outer<int>::Inner<double> nested_class(1.23);
struct Header
{
	int a;
	int& a_ref = a;
	int* a_pointer = nullptr;
	int*& a_pointer_ref = a_pointer;
};

namespace app
{
	template<class T>
	std::int32_t gValue = sizeof(T);
	struct AppData {};
	::app::AppData appData;

	template<class S>
	struct Namespace
	{
		template<class T, template<class> class U = std::is_class, size_t I = 0> requires(std::is_class_v<T>)
			class Base
		{
			template<class Q>
			static inline std::array<S*, sizeof(Q)> value1_ ;
			T* value2_;
		public:
			inline T* Create()const noexcept { return nullptr; }

		};
	};

	template<class T>
	using Namespace2 = Namespace<T>;

	class Child : public app::Namespace2<int>::Base<Header>
	{
	public:

	};

	namespace game
	{
		using BaseUsing = app::Namespace<int>::Base<std::vector<Header>>;
		using BaseUsing2 = ::app::game::BaseUsing;

		template<class T>
		using BaseUsing3 = ::app::game::BaseUsing;
	}

	class Child2 : public game::BaseUsing
	{
	public:

	};

	class Child3 : public std::vector<int>
	{
	public:

	};
}

std::int32_t	main()
{
	return 0;
}