//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	test_reflection.cpp
///	@brief	test_reflection
#include	"stdafx.h"
#include	"test_reflection.h"

namespace nox
{
	/// @brief 配列からポインタへの型変換
	/// @tparam T 
	template<class T>
	using ArrayToPointerType = std::add_pointer_t<std::remove_extent_t<T>>;
}

namespace
{
	class TestClass
	{
	public:
		constexpr TestClass()
		{

		}

		int value0 = 0;
		const int value1 = 0;
		int* value2 = 0;
		const int* value3 = 0;
		int value4[3]{};
		const int value5[3]{};
		int& value6 = value9;
	//	int&& value7 = std::forward<int&&>(value9);
	//	volatile int value8 = 0;
		static inline int value9 = 0;
		int(*value10)(int) = nullptr;
		int(TestClass::*value11)(int) = nullptr;
		int(TestClass::*value12)()const = nullptr;
		int(TestClass::*value13)()const noexcept= nullptr;
		int(TestClass::*value14)()&&= nullptr;

	};


	void getValue0Impl(nox::not_null<void*> out, nox::not_null<const void*> instance)
	{
		using TestClassType = const TestClass;

		using T0 = std::decay_t<decltype(TestClass::value0)>;
		*static_cast<T0*>(out.get()) = std::decay_t<T0>(static_cast<TestClassType*>(instance.get())->value0);

		using T1 = std::decay_t<decltype(TestClass::value1)>;
		*static_cast<T1*>(out.get()) = std::decay_t<T1>(static_cast<TestClassType*>(instance.get())->value1);

		using T2 = std::decay_t<decltype(TestClass::value2)>;
		*static_cast<T2*>(out.get()) = std::decay_t<T2>(static_cast<TestClassType*>(instance.get())->value2);

		using T3 = std::decay_t<decltype(TestClass::value3)>;
		*static_cast<T3*>(out.get()) = std::decay_t<T3>(static_cast<TestClassType*>(instance.get())->value3);

		using T4 = std::decay_t<decltype(TestClass::value4)>;
		*static_cast<T4*>(out.get()) = std::decay_t<T4>(static_cast<TestClassType*>(instance.get())->value4);

		using T5 = std::decay_t<decltype(TestClass::value5)>;
		*static_cast<T5*>(out.get()) = std::decay_t<T5>(static_cast<TestClassType*>(instance.get())->value5);

		using T6 = std::decay_t<decltype(TestClass::value6)>;
		*static_cast<T6*>(out.get()) = std::decay_t<T6>(static_cast<TestClassType*>(instance.get())->value6);

	//	using T7 = std::decay_t<decltype(TestClass::value7)>;
	//	*static_cast<T7*>(out.get()) = std::decay_t<T7>(static_cast<TestClassType*>(instance.get())->value7);

	//	using T8 = std::decay_t<decltype(TestClass::value8)>;
	//	*static_cast<T8*>(out.get()) = std::decay_t<T8>(static_cast<TestClassType*>(instance.get())->value8);

		using T9 = std::decay_t<decltype(TestClass::value9)>;
		*static_cast<T9*>(out.get()) = std::decay_t<T9>(static_cast<TestClassType*>(instance.get())->value9);

		using T10 = std::decay_t<decltype(TestClass::value10)>;
		*static_cast<T10*>(out.get()) = std::decay_t<T10>(static_cast<TestClassType*>(instance.get())->value10);

		using T11 = std::decay_t<decltype(TestClass::value11)>;
		*static_cast<T11*>(out.get()) = std::decay_t<T11>(static_cast<TestClassType*>(instance.get())->value11);

		using T12 = std::decay_t<decltype(TestClass::value12)>;
		*static_cast<T12*>(out.get()) = std::decay_t<T12>(static_cast<TestClassType*>(instance.get())->value12);

		using T13 = std::decay_t<decltype(TestClass::value13)>;
		*static_cast<T13*>(out.get()) = std::decay_t<T13>(static_cast<TestClassType*>(instance.get())->value13);

		using T14 = std::decay_t<decltype(TestClass::value14)>;
		*static_cast<T14*>(out.get()) = std::decay_t<T14>(static_cast<TestClassType*>(instance.get())->value14);
	}

	template<class T, class U>
	inline void getValue0(T& out, U&& instance)
	{
		getValue0Impl(&out, &instance);
	}

	template<class VariablePointer>
	inline void getVariableImpl(VariablePointer variable_pointer, nox::not_null<void*> out, nox::not_null<const void*> instance)
	{
		using T0 = std::decay_t<VariablePointer>;
		decltype(auto) vvvv = static_cast<const nox::FieldClassType<VariablePointer>*>(instance.get())->*variable_pointer;
//		*static_cast<T0*>(out.get()) = std::decay_t<T0>(static_cast<const nox::FieldClassType<VariablePointer>*>(instance.get())->*variable_pointer);
	}

	template<class VariablePointer, class T, class U> requires(std::is_const_v<T> == false)
	inline void getVariable(VariablePointer variable_pointer, T& out, U&& instance)
	{
		::getVariableImpl(variable_pointer, &out, &instance);
	}

	template<class T>
	inline constexpr decltype(auto) GetDecay(T&& v)noexcept
	{
		return static_cast<std::decay_t<T>>(v);
	}

	void getAddr(nox::not_null<void*> out, nox::not_null<void*> instance)
	{
	/*	using T0 = std::decay_t<decltype(TestClass::value0)>;
		*static_cast<const T0**>(out.get()) = &static_cast<TestClass*>(instance.get())->value0;

		using T1 = std::decay_t<decltype(TestClass::value1)>;
		*static_cast<const T1**>(out.get()) = &static_cast<TestClass*>(instance.get())->value1;

		using T2 = std::decay_t<decltype(TestClass::value2)>;
		*static_cast<const T2**>(out.get()) = &static_cast<TestClass*>(instance.get())->value2;

		using T3 = std::decay_t<decltype(TestClass::value3)>;
		*static_cast<const T3**>(out.get()) = &static_cast<TestClass*>(instance.get())->value3;

		using T4 = std::decay_t<decltype(TestClass::value4)>;
		using t = decltype(TestClass::value4);
		*static_cast<T4**>(out.get()) = reinterpret_cast<T4*>(&static_cast<TestClass*>(instance.get())->value4);
	
		using T5 = std::decay_t<decltype(TestClass::value5)>;
		*static_cast<const T5**>(out.get()) = reinterpret_cast<T5*>(&static_cast<TestClass*>(instance.get())->value5);

		using T6 = std::decay_t<decltype(TestClass::value6)>;
		*static_cast<T6**>(out.get()) = &static_cast<TestClass*>(instance.get())->value6;

		using T7 = std::decay_t<decltype(TestClass::value7)>;
		*static_cast<const T7**>(out.get()) = &static_cast<TestClass*>(instance.get())->value7;

		using T8 = std::decay_t<decltype(TestClass::value8)>;
		*static_cast<T8**>(out.get()) = &static_cast<TestClass*>(instance.get())->value8;*/
	}

	inline void TestGet()
	{
		constexpr TestClass ce_test_class = TestClass();
		const TestClass c_test_class;
		TestClass test_class;

		test_class.*& TestClass::value4;

		int value0 = 0;
		getVariable(&TestClass::value0, value0, test_class);
		getVariable(&TestClass::value0, value0, c_test_class);

	/*	int value1 = 0;
		getVariable(&TestClass::value1, value1, test_class);
		getVariable(&TestClass::value1, value1, c_test_class);

		int* value2 = nullptr;
		getVariable(&TestClass::value2, value2, test_class);
		getVariable(&TestClass::value2, value2, c_test_class);

		const int* value3 = nullptr;
		getVariable(&TestClass::value3, value3, test_class);
		getVariable(&TestClass::value3, value3, c_test_class);

		int value4[3]{};
		getVariable(&TestClass::value4, value4, test_class);
		getVariable(&TestClass::value4, value4, c_test_class);

		int value5[3]{};
		getVariable(&TestClass::value5, value5, test_class);
		getVariable(&TestClass::value5, value5, c_test_class);*/
	}
}

void nox::test::TestReflection()
{
	TestGet();
}

#if false

struct RuObj
{
	RuObj(int v) :value(v)
	{
		value_ptr = new int[12121212];
	}
	RuObj(RuObj&& r)noexcept
	{
		value = r.value;
		value_ptr = r.value_ptr;
		r.value_ptr = nullptr;

	}

	~RuObj()
	{
		delete[] value_ptr;
	}

	RuObj& operator=(const RuObj& r)noexcept
	{
		value = r.value;
		value_ptr = r.value_ptr;
		return *this;
	}

	RuObj& operator =(RuObj&& r)noexcept
	{
		value = r.value;
		value_ptr = r.value_ptr;
		r.value_ptr = nullptr;
		return *this;
	}

	int value;
	const int value2;
	int value3[3];
	const int value4[3];

	static inline int static_value = 0;
	int& value5 = static_value;
	const int& value6 = static_value;
	volatile int value7 = static_value;

	int* value_ptr;
	const int* const value_ptr2;
	nox::Vec3* vec3{};

	std::string func(int a)
	{
		std::string s;
		s += std::to_string(a);
		s += std::to_string(value);
		return s;
	}

	RuObj func2(const int* a)
	{
		//		RuObj data(*a);
		return { *a };
	}
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
	CopyDeleteObject(Vec3 v) :data(v) {}

protected:
	[[nodiscard]] inline constexpr CopyDeleteObject()noexcept = default;

private:
	inline constexpr CopyDeleteObject(const CopyDeleteObject&)noexcept = delete;
	inline constexpr CopyDeleteObject(CopyDeleteObject&&)noexcept = delete;

	inline constexpr void operator =(const CopyDeleteObject&)noexcept = delete;
	inline constexpr void operator =(CopyDeleteObject&&)noexcept = delete;
};

template<class T, class U>
	requires(std::is_pointer_v<std::remove_cvref_t<T>>&& std::is_void_v<std::remove_pointer_t<std::remove_cvref_t<T>>>)
inline void SetCastValue(T&& ptr, U&& value)
{
	*static_cast<U*>(ptr) = std::move(std::forward<U>(value));
}

#pragma optimize( "", off )
namespace
{
	inline constexpr const nox::reflection::UserDefinedCompoundTypeInfo& GetBase();

	constexpr const auto enumeratorInfo = nox::reflection::EnumeratorInfo(
		210,
		U"abc",
		U"abc",
		nullptr,
		0
	);

	constexpr const std::reference_wrapper<const nox::reflection::EnumeratorInfo> enumeratorInfoList[] = { enumeratorInfo };

	constexpr auto vv = enumeratorInfoList;

	constexpr const nox::reflection::EnumInfo enumInfo = nox::reflection::EnumInfo(
		nox::reflection::Typeof<int>(),
		U"abc",
		U"abc",
		U"abc",
		nox::reflection::AccessLevel::Public,
		nullptr,
		0,
		enumeratorInfoList,
		1
	);

	constexpr const std::reference_wrapper<const nox::reflection::EnumInfo> enumInfoList[] = { enumInfo };

	inline constexpr volatile const nox::reflection::UserDefinedCompoundTypeInfo baseCompundDataTypeInfoBase = nox::reflection::UserDefinedCompoundTypeInfo(
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
		enumInfoList,
		1,
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

	constexpr auto function_arg00 = nox::reflection::FunctionArgumentInfo(
		U"",
		nullptr,
		0,
		nox::reflection::Typeof<const int*>()
	);

	constexpr const std::reference_wrapper<const nox::reflection::FunctionArgumentInfo> function_arg_list[]{
		function_arg00
	};


	inline constexpr auto function_info = nox::reflection::detail::CreateFunctionInfo(
		&RuObj::func2,
		U"",
		U"",
		U"",
		nox::reflection::AccessLevel::Public,
		nullptr,
		0,
		function_arg_list,
		1,
		false,
		false,
		false,
		+[](void* result, void* instance, const void* arg00) {
			if (result != nullptr)
			{
				*static_cast<RuObj*>(result) =
					std::move(
						static_cast<RuObj*>(instance)->func2(*static_cast<const int* const*>(arg00)));

				//	SetCastValue(result, static_cast<RuObj*>(instance)->func2(*static_cast<const int* const*>(arg00)));
	//				result = static_cast<RuObj*>(instance)->func(*static_cast<const int* const*>(arg00));
			}
			else
			{
				static_cast<RuObj*>(instance)->func2(*static_cast<const int* const*>(arg00));
			}
		}
	);
}

inline void getterFunc(nox::not_null<void*> out, nox::not_null<const void*> instance)
{
	using T = std::decay_t<decltype(RuObj::value3)>;
	*static_cast<T*>(out.get()) = std::decay_t<T>(static_cast<const RuObj*>(instance.get())->RuObj::value3);

	using T2 = std::decay_t<decltype(RuObj::value)>;
	*static_cast<T2*>(out.get()) = std::decay_t<T2>(static_cast<const RuObj*>(instance.get())->value);

	using T3 = std::decay_t<decltype(RuObj::value_ptr)>;
	*static_cast<T3*>(out.get()) = std::decay_t<T3>(static_cast<const RuObj*>(instance.get())->value_ptr);

	using T4 = std::decay_t<decltype(RuObj::value_ptr2)>;
	*static_cast<T4*>(out.get()) = std::decay_t<T4>(static_cast<const RuObj*>(instance.get())->value_ptr2);

	using T5 = std::decay_t<decltype(RuObj::value3)>;
	*static_cast<T5*>(out.get()) = std::decay_t<T5>(static_cast<const RuObj*>(instance.get())->RuObj::value3);

	using T6 = std::decay_t<decltype(RuObj::value5)>;
	*static_cast<T6*>(out.get()) = std::decay_t<T6>(static_cast<const RuObj*>(instance.get())->RuObj::value3);

	using T7 = std::decay_t<decltype(RuObj::value6)>;
	*static_cast<T7*>(out.get()) = std::decay_t<T7>(static_cast<const RuObj*>(instance.get())->RuObj::value3);

	using T8 = std::decay_t<decltype(RuObj::value7)>;
	*static_cast<T8*>(out.get()) = std::decay_t<T8>(static_cast<const RuObj*>(instance.get())->RuObj::value3);
}

inline void getterFuncAddr(nox::not_null<void*> out, nox::not_null<void*> instance)
{
	//	using T = decltype(RuObj::value3);
	//	*static_cast<T**>(out.get()) = &static_cast<const RuObj*>(instance.get())->value3;

	using T1 = std::decay_t<decltype(RuObj::value3)>;
	*static_cast<T1**>(out.get()) = &static_cast<const RuObj*>(instance.get())->RuObj::value3;

	using T2 = std::decay_t<decltype(RuObj::value)>;
	*static_cast<T2*>(out.get()) = std::decay_t<T2>(static_cast<const RuObj*>(instance.get())->value);

	using T3 = std::decay_t<decltype(RuObj::value_ptr)>;
	*static_cast<T3*>(out.get()) = std::decay_t<T3>(static_cast<const RuObj*>(instance.get())->value_ptr);

	using T4 = std::decay_t<decltype(RuObj::value_ptr2)>;
	*static_cast<T4*>(out.get()) = std::decay_t<T4>(static_cast<const RuObj*>(instance.get())->value_ptr2);

	using T5 = std::decay_t<decltype(RuObj::value3)>;
	*static_cast<T5*>(out.get()) = std::decay_t<T5>(static_cast<const RuObj*>(instance.get())->RuObj::value3);

	using T6 = std::decay_t<decltype(RuObj::value5)>;
	*static_cast<T6*>(out.get()) = std::decay_t<T6>(static_cast<const RuObj*>(instance.get())->RuObj::value3);

	using T7 = std::decay_t<decltype(RuObj::value6)>;
	*static_cast<T7*>(out.get()) = std::decay_t<T7>(static_cast<const RuObj*>(instance.get())->RuObj::value3);

	using T8 = std::decay_t<decltype(RuObj::value7)>;
	*static_cast<T8*>(out.get()) = std::decay_t<T8>(static_cast<const RuObj*>(instance.get())->RuObj::value3);

}

inline void setterFuncImpl(nox::not_null<void*> instance, void* value)
{
	using T = decltype(RuObj::vec3);
	static_cast<RuObj*>(instance.get())->vec3 = *static_cast<T*>(value);
}

inline void setterFuncImpl_c(nox::not_null<void*> instance, const void* value)
{
	using T = decltype(RuObj::vec3);
	//	static_cast<RuObj*>(instance.get())->vec3 = *static_cast<const T*>(value);
}

template<class T>
inline void setterFunc(nox::not_null<void*> instance, T&& value)
{
	const int* ppp00 = nullptr;
	const int** p = &ppp00;
	void* pp = &ppp00;

	decltype(auto) v = &value;
	if constexpr (std::is_const_v<std::remove_pointer_t<std::remove_reference_t<T>>> == true)
	{
		::setterFuncImpl_c(instance, v);
	}
	else {
		::setterFuncImpl(instance, v);
	}
}


template<class T, class F>
inline void InvokeTestImpl(void* out, F&& f)
{
	*static_cast<T*>(out) = f();
}


class SampleClass {
public:
	virtual void sampleFunction() {
	}

	virtual void sampleFunction2() = 0;

	static void sampleFunction3() {
	}

	int value_;
	static int s_value_;
};

class SampleClass2 : public SampleClass
{
public:
	void sampleFunction()override {
	}

	static inline int static_value = 10;
};

constexpr auto variable_info = nox::reflection::detail::VariableInfoImpl<decltype(&SampleClass2::value_)>(
	&SampleClass2::value_,
	U"",
	U"",
	U"",
	nox::reflection::AccessLevel::Public,
	0,
	0,
	nullptr,
	0,
	nox::reflection::FieldAttributeFlag::None,
	nox::reflection::InvalidType,
	nox::reflection::InvalidType,
	+[](nox::not_null<void*> instance_ptr, nox::not_null<void*> value) {},
	+[](not_null<void*> outPtr, not_null<const void*> instance_ptr) {},
	+[](not_null<void*> outPtr, not_null<const void*> instance_ptr) {},
	+[](not_null<void*> instance_ptr, const void* const valuePtr, const std::uint32_t index) {return true; },
	+[](not_null<void*> outPtr, not_null<const void*> instance_ptr, const std::uint32_t index) {return true; },
	+[](not_null<void*> outPtr, not_null<const void*> instance_ptr, const std::uint32_t index) {return true; }
);

#endif // false
