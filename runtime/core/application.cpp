//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	application.cpp
///	@brief	application
//import std;
#include	"stdafx.h"
#include	"application.h"
#include	"../kernel/multicast_delegate.h"

//import nox.math;

using namespace nox;

namespace
{
	inline	void HookException(uint32 code, ::_EXCEPTION_POINTERS* const exception_ptr)
	{

	}
}

void	Application::Initialize()
{
}

namespace
{
}

struct RuObj
{
//	RuObj(int v) {}

//	void Func() {}
	//[[__attribute__((alloc_align([]()constexpr qnoexcept {return 0; }())))]]
	
	int value = 12;

	void func(int a) {}

	enum class E
	{
		
		A,
	};
};

namespace
{
//	constexpr nox::reflection::FunctionArgumentInfo arg00(u"v", nox::reflection::TypeKind::Int32);
}

class RefArray
{

};

struct CopyDeleteObjectBase
{
	//virtual ~CopyDeleteObjectBase()noexcept = 0;
};

struct CopyDeleteObject : CopyDeleteObjectBase
{
public:
	Vec3 data;
	static CopyDeleteObject create(Vec3 v) {
		return CopyDeleteObject(v);
	}
	//inline constexpr ~CopyDeleteObject()noexcept override{}
private:
	CopyDeleteObject(Vec3 v):data(v) {}

protected:
	[[nodiscard]] inline constexpr CopyDeleteObject()noexcept = default;
	
private:
	inline constexpr CopyDeleteObject(const CopyDeleteObject&)noexcept = delete;
	inline constexpr CopyDeleteObject(CopyDeleteObject&&)noexcept = delete;

	inline constexpr void operator =(const CopyDeleteObject&)noexcept = delete;
	inline constexpr void operator =(CopyDeleteObject&&)noexcept = delete;
};

struct Rest
{
	inline auto getSpan(){
		return vtable;
	}

	std::span<std::reference_wrapper<CopyDeleteObjectBase>> vtable;

	inline Rest(std::span<std::reference_wrapper<CopyDeleteObjectBase>> s) :vtable(s) {}
};

template<class T>
struct Span
{
	T* ptr;
	std::size_t size;
};

template <typename T>
struct is_anonymous {
	static constexpr bool value = nox::util::GetTypeName<T>().find("unnamed") != std::string_view::npos;
};

namespace
{
	inline constexpr const nox::reflection::UserDefinedCompoundTypeInfo& GetBase();

	constexpr auto baseCompundDataTypeInfoBase = nox::reflection::UserDefinedCompoundTypeInfo(
		nox::reflection::InvalidType,
		U"abc",
		U"abc",
		U"abc",
		nox::reflection::InvalidType,
		nullptr,
		0,
		nullptr,
		0,
		nullptr,
		0,
		nullptr,
		0,
		nullptr,
		0,
		nullptr,
		0
	);

	constexpr auto baseCompundDataTypeInfoChild = nox::reflection::UserDefinedCompoundTypeInfo(
		nox::reflection::InvalidType,
		U"abc",
		U"abc",
		U"abc",
		nox::reflection::InvalidType,
		nullptr,
		0,
		nullptr,
		0,
		nullptr,
		0,
		nullptr,
		0,
		nullptr,
		0,
		nullptr,
		0
	);

	inline constexpr const nox::reflection::UserDefinedCompoundTypeInfo& GetBase()
	{
		return baseCompundDataTypeInfoChild;
	}
}

void popup(std::optional<int*>& intr)
{
	{
		int rvaluess = 333312;
		intr = &rvaluess;
	}
}

void	Application::Run()
{

	std::optional<int*> intr;
	popup(intr);
	auto vyuse = intr.value();
	if (intr != std::nullopt)
	{
	}

	auto opt = baseCompundDataTypeInfoChild.GetEnumInfo(0).GetValueList<int>();
	//	compound data types
	struct
	{

	}dataStruct;
	struct
	{

	}dataStruct2;
	using unnamedClass = decltype(dataStruct);
	using unnamedClass2 = decltype(dataStruct2);
	constexpr auto nameUnsaneee = is_anonymous<unnamedClass>::value;
	constexpr auto nameUnsaneee2 = util::GetTypeName<unnamedClass2>();

	{
		nox::reflection::ReflectionObject* obj = nullptr;
		CopyDeleteObject* obj2 = nullptr;
	//	auto sp2 = std::span(obj, 2);
	}
	auto data0 = CopyDeleteObject::create(Vec3::One());
	auto data1 = CopyDeleteObject::create(Vec3::AxisX());
	const std::array< CopyDeleteObjectBase, 2> dataArray{ CopyDeleteObject::create(Vec3::One()), CopyDeleteObject::create(Vec3::AxisX()) };
	const std::array ary99 = { std::reference_wrapper<const CopyDeleteObjectBase>(data0), std::reference_wrapper<const CopyDeleteObjectBase>(data1) };
	auto sto = std::span(ary99);

	decltype(auto) r00 = std::reference_wrapper<const CopyDeleteObjectBase>(data0);
	const CopyDeleteObjectBase& r0928 = r00;

	//std::span< sesese2 = sesese;

	//Rest rest(sesese);

	//auto span = rest.getSpan();
	//decltype(auto) sp0 = span[0];

	os::Thread game_thread;
	game_thread.SetThreadName(u"Game");
	game_thread.Dispatch([this]() {

		while (true)
		{
			try
			{
				this->Update();
				break;
			}
			catch (const std::exception&)
			{
				//	
			}
		}
		});

	game_thread.Wait();
}

void	Application::Finalize()
{
}

inline	void	Application::Update()
{
	try
	{
		NOX_ASSERT(false, U"🐘さんだね");

	}
	catch (const std::exception&)
	{

	}
}