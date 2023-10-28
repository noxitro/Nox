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

struct Data {};

int WINAPI::WinMain(_In_ ::HINSTANCE hInstance, _In_opt_ ::HINSTANCE /*hPrevInstance*/, _In_ ::LPSTR /*lpCmdLine*/, _In_ int nCmdShow)
{

	Data dArray[5] = {};
	Data* ary = dArray;

	std::vector<Data> v;

	auto f = std::ranges::find_if(ary, ary + 5, [](const auto& x) {return true; });
	auto f2 = std::ranges::find_if(v.begin(), v.end(), [](const auto& x) {return true; });
	auto aa = &*f2;

	using namespace nox;
	
	return 0;
}