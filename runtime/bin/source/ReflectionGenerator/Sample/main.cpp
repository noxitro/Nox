//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	main.cpp
///	@brief	main
/// 
/// 
/// 

#define	PP_CAT(x,y) x##y
#define	PP_CAT_I(x, y) PP_CAT(x, y)
#define	PP_STR(x) #x
#define	PP_STR_I(x) PP_STR(x)

#define ATTR_1(x) __attribute__((annotate(PP_STR_I(PP_CAT_I(NOX_REFLECTION_ATTR_, x)))))
#define ATTR_2(x, ...) ATTR_1(x) ATTR_1(__VA_ARGS__)
#define ATTR_3(x, ...) ATTR_1(x) ATTR_2(__VA_ARGS__)
#define ATTR_4(x, ...) ATTR_1(x) ATTR_3(__VA_ARGS__)
// 以下、必要なだけ続ける

#define GET_MACRO(_1,_2,_3,_4,NAME,...) NAME


#define ATTR(...) // GET_MACRO(__VA_ARGS__, ATTR_4, ATTR_3, ATTR_2, ATTR_1)(__VA_ARGS__)

// 使用例
#if _DEBUG
constexpr auto preprocesserValue = _DEBUG;
#endif // _DEBUG

class DisplayName
{
public:
	inline explicit DisplayName(const char32_t* name) :
		name_(name) {}

private:
	const char32_t* name_;
};

class IgnoreDataMember
{
public:
	inline constexpr IgnoreDataMember()noexcept = default;
};

namespace app
{
	ATTR(DisplayName(u""), IgnoreDataMember())
		int Do() { return 0; }
}

namespace app
{
	struct Int {
		int v;
		constexpr Int(int _v) :v(_v) {}
	};

	Int intValue = Int(22);

	struct LiteralArgument
	{
		int v;
		constexpr LiteralArgument(int _v, const char8_t* c)noexcept :v(_v) {}
	};

	
		template<class T>
		struct LiteralData
		{
			LiteralArgument v;
			constexpr LiteralData(LiteralArgument _v)noexcept :v(_v) {}
		};
		struct NDOT
		{
			template<class T, class U>
			static consteval LiteralData<T> MakeLiteralData(LiteralArgument vaaa, U = U())noexcept {
				return LiteralData<T>(vaaa);
			}
		};

	template<class T, class U>
	class Same {};

	struct Namespace00
	{
		template<class T, class U>
		class Same2 {};
	};

	template<int _Index, class T, LiteralData<T> data>
	class As {};

template<class T>
class TClass 
{
	Namespace00::Same2<decltype(1), decltype(nullptr)> same2;
//	Same<T, T> same;
	Same<app::Same<T, T>, Namespace00::Same2<T, T>> same;
	As<12,int, NDOT::MakeLiteralData<int, float>(LiteralArgument(int(23), u8""))> asBody;
	As<12, int, LiteralData<int>(LiteralArgument(123, u8"abc"))> asValue;

	using ttttt = decltype(&NDOT::MakeLiteralData<int, float>);

};
using TClass2 = TClass<int>;

	inline auto testFunc()noexcept
	{
		TClass2 value;
		return sizeof(value);
	}
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
		template<class T, template<class, class> class U = std::is_same, template<class> class U2 = std::is_abstract, size_t I = 0> //requires(std::is_class_v<T>)
			class Base
		{
			template<class ___T, class _U>
			class Child {
				___T** mmv = nullptr;
				_U& re;
			};

			U<float, U2<int>> use22;

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