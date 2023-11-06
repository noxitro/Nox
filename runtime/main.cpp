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

int WINAPI::WinMain(_In_ ::HINSTANCE hInstance, _In_opt_ ::HINSTANCE /*hPrevInstance*/, _In_ ::LPSTR /*lpCmdLine*/, _In_ int nCmdShow)
{
	Test test;

	constexpr const auto& type = nox::reflection::Typeof<int>();
	const auto& n1 = type.GetPointeeType();

	nox::Vec3* out = nullptr;
	field_info00.TryGetValueAddress(out, test);

	using namespace nox;
	
	return 0;
}