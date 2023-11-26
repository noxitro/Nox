//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	main.cpp
///	@brief	main

#include	<cstdint>
#include	<type_traits>
struct Header
{
	int a;
	int& a_ref = a;
	int* a_pointer = nullptr;
	int*& a_pointer_ref = a_pointer;
};

struct Namespace
{
	template<class T, template<class> class U = std::is_class, size_t I = 0> requires(std::is_class_v<T>)
		class Base
	{
	public:
		inline T* Create()const noexcept { return nullptr; }
		inline static void* operator new(size_t) = delete;
	};
};

class Child : public Namespace::Base<Header>
{
public:

};

std::int32_t	main()
{
	return 0;
}