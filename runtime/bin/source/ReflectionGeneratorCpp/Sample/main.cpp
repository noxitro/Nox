//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	main.cpp
///	@brief	main
/// 
/// 
/// 
//
//#define	PP_CAT(x,y) x##y
//#define	PP_CAT_I(x, y) PP_CAT(x, y)
//#define	PP_STR(x) #x
//#define	PP_STR_I(x) PP_STR(x)
//
//#define ATTR_1(x) __attribute__((annotate(PP_STR_I(PP_CAT_I(NOX_REFLECTION_ATTR_, x)))))
//#define ATTR_2(x, ...) ATTR_1(x) ATTR_1(__VA_ARGS__)
//#define ATTR_3(x, ...) ATTR_1(x) ATTR_2(__VA_ARGS__)
//#define ATTR_4(x, ...) ATTR_1(x) ATTR_3(__VA_ARGS__)
//// 以下、必要なだけ続ける
//
//#define GET_MACRO(_1,_2,_3,_4,NAME,...) NAME
//
//
//#define ATTR(...) // GET_MACRO(__VA_ARGS__, ATTR_4, ATTR_3, ATTR_2, ATTR_1)(__VA_ARGS__)
//
//// 使用例
//#if _DEBUG
//constexpr auto preprocesserValue = _DEBUG;
//#endif // _DEBUG
//
//class DisplayName
//{
//public:
//	inline explicit DisplayName(const char32_t* name) :
//		name_(name) {}
//
//private:
//	const char32_t* name_;
//};
//
//class IgnoreDataMember
//{
//public:
//	inline constexpr IgnoreDataMember()noexcept = default;
//};
//	#include	<type_traits>

#if false
namespace gggg
{
	template<class... T>
	class Class002 {};
}
struct CppTest
{
	template<class _a, class _b, class _c>
	inline void func0() {}

	void minsexImpl()
	{
		gggg::Class002<int, float ,double>();

//		func0<12, 3.0f,int>();
		func0<double, float ,int>();
	}

};



const auto sizesizsesize = sizeof(decltype(&CppTest::func0<long, int, float>));

using AddPointerType3 = gggg::Class002<decltype(+[]() {return sizeof(12); }), size_t > * const** const;



template<class T>
inline constexpr auto GetUniqueName()
{
	return __FUNCSIG__;
}

auto unique_func_class2()noexcept
{
	struct unique_class
	{
		int v;
	};

	return 2;
}

auto unique_func_class()noexcept
{
	struct unique_class
	{
		int v;
	};

	return unique_class{};
}
using aaabbcc = decltype(unique_func_class());

#if defined(__clang__)
#define	ATTR(x) __attribute__((annotate("#x")))
#else
#define	ATTR(x)
#endif

struct ATTR(abc)  NEW{
	unsigned long*  Data1;
	unsigned short* Data2;
	unsigned short* Data3;
	unsigned char  Data4[8];
};

namespace app::app2::app3
{
	using NEWPPP = NEW**&&;
}

namespace app2ref = app::app2::app3;

 namespace app::app2
{
	using NEWPPP2 = app2ref::NEWPPP;
}

template <class T>
constexpr T pi = static_cast<T>(3.14159265358979323846);

namespace global
{
	template<class T>
	struct add_pointer
	{
		using type = T*;
	};

	template<class T>
	using add_pointer_t = typename add_pointer<T>::type;
}

namespace global
{
	struct float_t {};
}

namespace app2
{
	using namespace global;

	class FuncClassgg
	{
	public:


		template<class T>
		inline T TdeclFunc(int a) {}

		inline void declFunc(int a)
		{
			TdeclFunc < add_pointer_t<decltype([]()->float_t {}()) >> (a);
			pi<int>;
			int*** intpp;
		}
	};
}

namespace app
{
	template<class T>
	consteval inline void testFunc(T value)noexcept
	{
	}

	inline void _main()
	{
		testFunc<int>(1);
	}

	enum ENUM
	{
		_ENUMA, _ENUMB, _ENUMC
	};

	enum class SCOPED_ENUM
	{
		_A, _B, _C = sizeof(decltype( 2))
	};


	struct LiteralValueClass_Test 
	{
		constexpr explicit LiteralValueClass_Test(int _v = 222)noexcept:v(_v)
		{
		}
	private:
		const int v = 0;
	};

	struct LiteralValueClass 
	{
	//	inline constexpr LiteralValueClass()noexcept:mV(SCOPED_ENUM::_A) {}

		constexpr explicit LiteralValueClass(SCOPED_ENUM _v = SCOPED_ENUM::_A)noexcept :
			mV(_v), mV2(_v)
		{
			mV2 = SCOPED_ENUM::_C;
		//	mV3 = SCOPED_ENUM::_B;
		}

		/*constexpr explicit LiteralValueClass(LiteralValueClass_Test v)noexcept :mV2(v)
		{
		}*/

		SCOPED_ENUM mV;
		SCOPED_ENUM mV2;
//		LiteralValueClass_Test mV2;
	};

	class NormalClass {  };
	using NormalClass2 = NormalClass;

	template<class T, class U>
	struct MinSizeType
	{
		using type = T;
		int index = 0;
		inline static int static_index = 0;

		class Child21
		{
		};
	};

//	using abtype = decltype([]()->auto {return typename MinSizeType <float, float>::type();; }());

	template<class T = int, class U = decltype([]()->auto {return typename MinSizeType <decltype(typename MinSizeType <T, T>::type()), T>::type(); }), LiteralValueClass l = LiteralValueClass( SCOPED_ENUM::_B ) >
	class IOOClass
	{
		using _Type = T;

		T* value;
		inline static T* staticValue;
	};

	template<>
	class IOOClass < IOOClass<int>, char32_t, LiteralValueClass(SCOPED_ENUM::_A) > ;

	IOOClass<> iooClass;
	IOOClass<float>* iooClassPtr;

	template<class T>
	inline consteval auto getF() -> T { return T(); }

	template<class T>
	struct uint_lambbaer { using Type = T; };

	using uint0 = decltype(getF<char32_t>);
	using uint1 = typename uint_lambbaer<char32_t*>::Type;
	//using uint1 = decltype(&FuncClass<int>::declFunc<float>);
	//using uint2 = decltype([](uint1 x) {return 0; });

	//using uint3 = app::uint1;
	//using uint4 = uint1;

	using lambdaType = decltype(getF<char32_t****>);
	using uint32 = unsigned int;

	template<int Size = 12, class T = decltype([](){}), class ABC = decltype(23), class U = uint32>
	class ClassApp {

		template<class OIJ , class K = T>
		class ChildTL {
			T* p;
			OIJ* p2;
		};

		IOOClass<char32_t> iooClass23323;
	};
}

namespace app
{
	template<class T>
	class FuncClass
	{
	public:
		template<class U>
		inline void declFunc(U* value) {}
	};

}

namespace gggg
{
	template<class T>
	class Class00 {};
}
using namespace gggg;
template<class T>
using AddPointerType = Class00<decltype([]() {return sizeof(T); }) > * const** const;

template<class T>
using AddPointerTypeWWW = Class00<T>* const** const;

template<class T>
using AddPointerTypeXXX = Class00<int>&;


AddPointerType<int> globalPointer = nullptr;
Class00<int> globalPointer2;

template<class T, class U>
using AddPointerType2 = T*;

template<class T>
class OwnerClass
{
	using ValueType = T;

	template<class U  = T>
	class ChildClass
	{
		inline void Func(T*, U*) {}
	};
};



namespace app
{
//	ATTR(DisplayName(u""), IgnoreDataMember())
//		int Do() { return 0; }
	using int32 = int;
	using UClass = AddPointerTypeWWW<AddPointerType<int32>>;
	using UClass2 = AddPointerTypeWWW<AddPointerTypeWWW<AddPointerTypeXXX<AddPointerType<int32>>>>;
	UClass* uClass = nullptr;
	AddPointerType<int>* uClass2 = nullptr;

	template<class T = decltype([]() {return sizeof(int32); }) > requires(sizeof(T) >= 0)
//	template<class T = AddPointerType<decltype([]() {return 0; }())>> requires(sizeof(T) >= 0)
//	template<class T = AddPointerType2<char*, int*>> requires(sizeof(T) >= 0)
	class TestClass00
	{
		void testfff()
		{
			constexpr auto n = GetUniqueName<UClass>();
			constexpr auto n2 = GetUniqueName<UClass2>();

			getF<int>();
			getF<float>();
			getF<char32_t>();

			constexpr auto nn323 = sizeof(U"abc");
		}
	};

	template<class T = AddPointerType<char*>> requires(sizeof(T) >= 0)
		class TestClass001
	{

	};

	template<class T = decltype([]() {return 10; }) >
	class TestClass01
	{

	};

	template<int V>
	class TestClass02
	{

	};

	template<int V = 13>
	class TestClass03
	{

	};

	template<template<class> class T>
	class TestClass04
	{

	};

	template<class... Types>
	class TestClass05
	{

	};

	template<int... Values>
	class TestClass06
	{

	};

	template<template<int> class T>
	class TestClass07
	{

	};

	template<template<class...> class T>
	class TestClass08
	{

	};

	template<template<class> class... T>
	class TestClass09
	{

	};

	template<template<class...> class... T>
	class TestClass010
	{

	};

	template<template<template<class>class U> class T >
	class TestClass11
	{
	};

	template<int V = []()constexpr {return 10; }() >
	class TestClass12;

	template<class T>
	class TestClass13;

	template<>
	class TestClass13<int> {};
}
#endif

int	main()
{
	return 0;
}