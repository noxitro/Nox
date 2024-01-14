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

using T = decltype(Test::v_ref);

constexpr auto field_info00 = nox::reflection::detail::CreateFieldInfo<Test>(
	u8"",
	u8"",
	u8"",
	nox::reflection::AccessLevel::Public,
	nullptr,
	0,
	nox::reflection::Typeof<decltype(Test::vc)>(),
	nullptr,
	[](nox::not_null<void*> outValue, nox::not_null<const void*> instance_ptr) { 
		using TempType = nox::RemoveConstPointerReferenceType<std::add_pointer_t<decltype(Test::vc)>>;
		TempType* pp = nullptr;
		*static_cast<TempType>(outValue.get()) = static_cast<const Test*>(instance_ptr.get())->vc;
	},
	nullptr,
	nullptr,
	nullptr,
	nullptr
);

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
};


/// @brief 型生成クラス
	/// @tparam T 
template<class T> 
struct ReflectionTypeActivator
{
	template<class... Args> requires(std::is_constructible_v<T, Args...>)
	[[nodiscard]]	static inline constexpr T	CreateInstance(Args&&... args)noexcept { return T{ args... }; }
};

class NOX_ATTR_TYPE(nox::attr::IgnoreDataMember()) PrivateClass
{
	friend struct ReflectionTypeActivator<PrivateClass>;
	PrivateClass(int a) 
	{
		std::printf("");
	}

	int a_ = 0;

	inline PrivateClass& operator=(PrivateClass& rhs)noexcept
	{
		a_ = rhs.a_;
		return *this;
	}
};


namespace nox::reflection::detail
{
	template<class _TupleType>
	inline bool CheckAttributes2()noexcept
	{
		return CheckAttributesTuple<_TupleType>::test();
	}
}


int WINAPI::WinMain(_In_ ::HINSTANCE hInstance, _In_opt_ ::HINSTANCE /*hPrevInstance*/, _In_ ::LPSTR /*lpCmdLine*/, _In_ int nCmdShow)
{
	using namespace nox;

	nox::String strbase;

	auto n0 = util::GetTypeName<int>();
	auto n1 = util::ConvertWString(util::GetTypeName<int>());
	constexpr auto lstr = L"aaabb {} is null";

	auto arg0 = util::ConvertWString("int");
	auto arg1 = nox::WString(L"int");
	auto fname1 = util::Format(lstr, arg0);
	auto fname2 = util::Format(lstr, arg1);

	reflection::Reflection::Instance();

	static constexpr std::array<reflection::MethodParameter, 2> method_param_list
	{
		reflection::detail::CreateMethodParameter<int32>(u8"", nullptr, 0, false),
		reflection::detail::CreateMethodParameter<int32>(u8"", nullptr, 0,true)
	};

	constexpr auto method_info = nox::reflection::detail::CreateMethodInfo<std::decay_t<decltype(TestFunc)>>(
		u8"",
		u8"",
		u8"",
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