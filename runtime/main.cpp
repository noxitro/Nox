#include	"stdafx.h"

#define STRICT
#define NOMINMAX
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP
#define NOMCX
#define NOSERVICE
#define NOHELP

// ヘッダーからあまり使われない関数を省く
#define WIN32_LEAN_AND_MEAN
#include	<Windows.h>
#undef	near
#undef	far

#undef STRICT
#undef NOMINMAX
#undef NODRAWTEXT
#undef NOGDI
#undef NOBITMAP
#undef NOMCX
#undef NOSERVICE
#undef NOHELP

struct Test
{
	nox::Vec3 v;
	const nox::Vec3 vc;
	nox::Vec3* v_ptr;
	const nox::Vec3* v_c_ptr;
	const nox::Vec3& v_ref;

	inline constexpr Test()noexcept :
		v(nox::Vec3(1.0f, 1.0f, 1.0f)),
		vc(nox::Vec3(1.0f, 1.0f, 1.0f)),
		v_ptr(&v),
		v_c_ptr(&v),
		v_ref(v){}
};

inline constexpr nox::Vec3	TestFunc(nox::int32 a, nox::int32 b = 1)noexcept
{
	return nox::Vec3(a, b, a + b);
}

class Base 
{
public:
	inline static void* operator new(size_t size) = delete;
	inline static void* operator new[](size_t size) = delete;
};

class Obj : public Base
{
public:
	inline static void* operator new(size_t size) { return nullptr; }
	inline static void* operator new[](size_t size) {return nullptr; }

		inline Obj(int) {}
	inline Obj(const Obj&) = delete;
	inline Obj(const Obj&&) = delete;

	void MemberFunc() 
	{
		std::printf("abc");
	}
};


namespace nox
{

	template<class _FuncType>
	class IMulticastDelegate
	{

	};

	template<class _FuncType, uint32 _BufferSize = 32>
	class MulticastDelegate : public IMulticastDelegate<_FuncType>
	{
	public:


	private:
		nox::Vector<Delegate<_FuncType, _BufferSize>> function_list_;
	};

	
}

struct Data00
{
	int&& dataF;

	Data00(int&& _dataF) :dataF(std::forward<int>(_dataF)) {}
};


namespace nox
{
	
}

namespace nox
{
	template<>
	inline void IntrusivePtrAddReference<Obj>(Obj&)
	{

	}

	inline void IntrusivePtrReleaseReference(Obj&)
	{

	}

	template<>
	inline void IDelegateRegistered<Obj*>(Obj*&)
	{

	}

	template<class U>
	inline void IDelegateRegistered(IntrusivePtr<U>&)
	{

	}

	template<class T>
	inline constexpr decltype(auto) IDelegateGetInstance(IntrusivePtr<T>& v) noexcept
	{
		return v.Get();
	}

	template<class T>
	struct IsCallable_IntrusivePtr2
	{
	private:
		template<typename U>
		static inline constexpr auto Test(int) -> std::conditional_t<std::is_void_v<std::void_t<decltype(IntrusivePtrAddReference(*(U*)nullptr))>>, std::true_type, std::false_type >;
		template<typename U>
		static inline constexpr std::false_type Test(...);

	public:
		static constexpr bool value =
			std::is_same_v<decltype(Test<T>(nullptr)), std::true_type>// &&
			//	std::is_same_v<decltype(Test2<T>(nullptr)), std::true_type> 
			;
	};

	
}

int WINAPI::WinMain(_In_ ::HINSTANCE hInstance, _In_opt_ ::HINSTANCE /*hPrevInstance*/, _In_ ::LPSTR /*lpCmdLine*/, _In_ int nCmdShow)
{
	using namespace nox;
	
	const bool bool0 = nox::detail::IsCallable_IntrusivePtr<Obj>::value;
	const bool bool1 = IsCallable_IntrusivePtr2<Obj>::value;
	const bool bool2 = nox::detail::is_callable<Obj>::value;
	const bool bool3 = nox::detail::IsCallable_IDelegateGetInstance<nox::IntrusivePtr<Obj>>::value;
	Obj fObj(2);
	nox::IntrusivePtr<Obj> ptr(fObj);

	IntrusivePtrAddReference(fObj);
	nox::detail::IsCallable_IntrusivePtr<Obj>::value;
	nox::IntrusivePtrReleaseReference(fObj);

	sizeof(nox::Delegate<void()>);
	sizeof(std::function<void()>);

	int inta = 0;
//	IDelegateRegistered<int>(inta);
//	IDelegateRegistered<Obj>(fObj);
//	IDelegateRegistered(ptr);

//	IDelegateRegistered<int>(inta);
//	IDelegateRegistered(&fObj);

//	decltype(auto) get_instance_0 = IDelegateGetInstance<int>(inta);
//	decltype(auto) get_instance_1 = IDelegateGetInstance<Obj>(fObj);
//	decltype(auto) get_instance_2 = IDelegateGetInstance(ptr);
	
//	using tttt = std::decay_t<decltype(static_cast<void(*)(Obj&)>(DelegateRegistered))>;

//	constexpr bool bbolconstat = ExistsFunction<Obj>::value;
//	static_assert(concepts::IDelegateRegister<Obj>);

	int dummyBuffer[120];
	Delegate<void(), 890> func;
	func.Bind([dummyBuffer]() {});

	Delegate<void()> func1([&]() {});
	//func1.Bind([]() {});

	Delegate<void()> func2;
	func2.Bind(&Obj::MemberFunc, ptr);
	func2.Invoke();

	Delegate<void()> func3;
	func3.Bind(&Obj::MemberFunc, fObj);
	func3.Invoke();

	//bool isF0 = func.Equal(func1);
	bool isF00 = func.Equal(func2);
	bool isF01 = func2.Equal(func3);


	nox::String strbase;

	auto n0 = util::GetTypeName<int>();
	auto n1 = unicode::ConvertWString(util::GetTypeName<int>());
	constexpr auto lstr = L"aaabb {} is null";

	auto arg0 = unicode::ConvertWString("int");
	auto arg1 = nox::WString(L"int");
	auto fname1 = util::Format(lstr, arg0);
	auto fname2 = util::Format(lstr, arg1);

	reflection::Reflection::Instance();

	static constexpr std::array<reflection::FunctionArgParameter, 2> method_param_list
	{
		reflection::detail::CreateMethodParameter<int32>(U"", nullptr, 0, false),
		reflection::detail::CreateMethodParameter<int32>(U"", nullptr, 0,true)
	};

	constexpr auto method_info = nox::reflection::detail::CreateMethodInfo<std::decay_t<decltype(TestFunc)>>(
		U"",
		U"",
		U"",
		nox::reflection::AccessLevel::Public,
		nullptr,
		0,
		method_param_list.data(),
		static_cast<uint8>(method_param_list.size()),
		false,
		false,
		+[](void* a)noexcept->decltype(auto) {return TestFunc(*reinterpret_cast<const int32*>(a)); },
		+[](void* a, void* b)noexcept->decltype(auto) {return TestFunc(*reinterpret_cast<const int32*>(a), *reinterpret_cast<const int32*>(b)); }
	);

	int32 out;
	constexpr int aaa = 10;
	int bbb = 10;
	nox::Vec3 r = method_info.Invoke<nox::Vec3>(aaa);

	constexpr auto nn12= nox::Vec3::Zero();

	constexpr auto& ntype = reflection::detail::GetTypeChunk<int>();

	return 0;
}