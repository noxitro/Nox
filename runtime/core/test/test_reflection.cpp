//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	test_reflection.cpp
///	@brief	test_reflection
#include	"stdafx.h"
#include	"test_reflection.h"

#include	"game_object.h"
#include	"test_behavior.h"

namespace nox
{
	/// @brief 配列からポインタへの型変換
	/// @tparam T 
	template<class T>
	using ArrayToPointerType = std::add_pointer_t<std::remove_extent_t<T>>;

	template<class T> requires(!std::is_same_v < T, std::decay_t<T>>)
		inline constexpr std::decay_t<T> ToDecay(T&& value)noexcept
	{
		return std::forward<T>(value);
	}

	template<class T> requires(std::is_same_v<T, std::decay_t<T>>)
		inline constexpr decltype(auto) ToDecay(T&& value)noexcept
	{
		return value;
	}
}

namespace
{
	class TestClass
	{
	public:
		TestClass()
		{

		}

		int value0 = 10;
		const int value1 = 120;
		int* value2 = nullptr;
		const int* value3 = nullptr;
		int value4[3]{5, 6, 7};
		const int value5[3]{88, 99, 22};
		int& value6 = value9;
		int&& value7 = (int&&)(value9);
	//	volatile int value8 = 0;
		static inline int value9 = 770;
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
		decltype(auto) vvvv = static_cast<const nox::MemberObjectPointerClassType<VariablePointer>*>(instance.get())->*variable_pointer;
//		*static_cast<T0*>(out.get()) = std::decay_t<T0>(static_cast<const nox::MemberObjectPointerClassType<VariablePointer>*>(instance.get())->*variable_pointer);
	}

	template<class VariablePointer, class T, class U> requires(std::is_const_v<T> == false)
	inline void getVariable(VariablePointer variable_pointer, T& out, U&& instance)
	{
		::getVariableImpl(variable_pointer, &out, &instance);
	}

	template<class VariablePointer>
	inline void getVariableAddrImpl(VariablePointer variable_pointer, nox::not_null<void*> out, nox::not_null<const void*> instance)
	{
		using T0 = std::decay_t<VariablePointer>;
		*static_cast<T0**>(out.get()) = &static_cast<TestClass*>(const_cast<void*>(instance.get()))->value0;

	}

	void getAddr(nox::not_null<void*> out, nox::not_null<const void*> instance)
	{

	}

	inline void TestGet()
	{
		//constexpr TestClass ce_test_class = TestClass();
		const TestClass c_test_class;
		TestClass test_class;

		test_class.*& TestClass::value4;

		int value0 = 0;
		getVariable(&TestClass::value0, value0, test_class);
		getVariable(&TestClass::value0, value0, c_test_class);
		
		int value1 = 0;
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
		getVariable(&TestClass::value5, value5, c_test_class);

		int value6 = 0;
		[](int& out, const TestClass& instance) {
			out = instance.value6;
			}(value6, test_class);
	}

	inline void TestGetAddr()
	{
		const TestClass c_test_class;
		TestClass test_class;

		std::pair<nox::not_null<const void*>, const nox::reflection::Type&> pair0 =
			std::make_pair(static_cast<const void*>(&test_class), nox::reflection::Typeof<int>());
		std::pair<nox::not_null<const void*>, const nox::reflection::Type&> table[] = { pair0 };

		const void* instance = &test_class;
		const void* instance_c = &c_test_class;

		void* const instance_ptr = const_cast<void*>(instance);
		void* const instance_c_ptr = const_cast<void*>(instance_c);

		std::decay_t<decltype(TestClass::value0)>* out0 = nullptr;
		void* const out_ptr0 = &out0;
		using T0 = std::decay_t<decltype(TestClass::value0)>;
		*static_cast<const T0**>(out_ptr0) = &static_cast<TestClass*>(instance_ptr)->TestClass::value0;

		std::decay_t<decltype(TestClass::value1)>* out1 = nullptr;
		void*const out_ptr1 = &out1;
		using T1 = std::decay_t<decltype(TestClass::value1)>;
		*static_cast<const T1**>(out_ptr1) = &static_cast<TestClass*>(instance_ptr)->value1;

		std::decay_t<decltype(TestClass::value4)*> out4 = nullptr;
		void* const out_ptr4 = &out4;
		using T4 = std::decay_t<decltype(TestClass::value4)>;
		//*static_cast<T4**>(out_ptr4) = &static_cast<const TestClass*>(instance_ptr)->value4;

		std::decay_t<decltype(TestClass::value6)>* out6 = nullptr;
		void* const out_ptr6 = &out6;
		using T6 = std::decay_t<decltype(TestClass::value6)>;
		*static_cast<T6**>(out_ptr6) = &static_cast<TestClass*>(instance_ptr)->value6;

		std::decay_t<decltype(TestClass::value7)>* out7 = nullptr;
		void* const out_ptr7 = &out7;
		using T7 = std::decay_t<decltype(TestClass::value7)>;
		*static_cast<const T7**>(out_ptr7) = &static_cast<TestClass*>(instance_ptr)->value7;

		constexpr auto nn = nox::util::BitOr(
			 nox::reflection::VariableAttributeFlag::None,
			nox::reflection::VariableAttributeFlag::None
		);

		constexpr auto nnse = nox::util::BitNot(nox::reflection::VariableAttributeFlag::Constexpr);

		nox::DebugBreak();
		/*	using T2 = std::decay_t<decltype(TestClass::value2)>;
		*static_cast<const T2**>(out.get()) = &static_cast<TestClass*>(instance_ptr)->value2;

		using T3 = std::decay_t<decltype(TestClass::value3)>;
		*static_cast<const T3**>(out.get()) = &static_cast<TestClass*>(instance_ptr)->value3;

		using T4 = std::decay_t<decltype(TestClass::value4)>;
		*static_cast<T4**>(out.get()) = reinterpret_cast<T4*>(&static_cast<TestClass*>(instance_ptr)->value4);

		using T5 = std::decay_t<decltype(TestClass::value5)>;
		*static_cast<const T5**>(out.get()) = reinterpret_cast<const T5*>(&static_cast<TestClass*>(instance_ptr)->value5);

		*/
	}
}

class ClassBaseA
{
public:
	virtual void Func1() {}

};

class ClassBaseB
{
public:
	virtual void Func1() {}
	virtual void Func2() {}

};

class ClassChild1 : public ClassBaseA
{
public:
	void Func1()override {}
};

class ClassChild2 : public virtual ClassBaseA, public virtual ClassBaseB
{
public:
	void Func1()override {}
	void Func2()override {}
};

#include	<iostream>

namespace
{
	class Obj
	{
	public:

		Obj(int _v):value(_v)
		{
			NOX_INFO_LINE(U"Default Constructor");
		}

		Obj(const Obj& obj)
		{
			NOX_INFO_LINE(U"Copy Constructor");
		}

		Obj(Obj&& obj)noexcept
		{
			NOX_INFO_LINE(U"Move Constructor");
		}

		virtual ~Obj()
		{
			NOX_INFO_LINE(U"Destructor");
		}

		int value = 0;
	};

	class ObjHolderBase {};

	template<class T>
	class ObjHolder : public ObjHolderBase
	{
	public:
		template<class U>
		ObjHolder(U&& obj) :obj(std::forward<U>(obj)) {}

		T&& obj;
	};
}

namespace nox2
{


	//template<class, class...>
	//struct IsFunctionObject : std::false_type {};

	///// @brief 関数オブジェクト
	//template<class T, class... Args> requires(std::is_void_v<decltype(std::declval<T>().T::operator()(std::declval<Args>()...))>)
	//struct IsFunctionObject<T, Args...> : std::true_type {};

	
	//template<class T, class... Args>
	//struct IsFunctionObject
	//{
	//private:
	//	static constexpr std::false_type test(...) = delete;
	//	static constexpr std::true_type test(std::void_t<decltype(std::declval<T>().T::operator()(std::declval<Args>()...))>*) = delete;

	//public:
	//	static constexpr bool value = sizeof(IsFunctionObject<T, Args...>::test(nullptr));
	//};
	//
}

namespace concepts2
{
	/// @brief 関数オブジェクトか
	template<class T, class... Args>
	concept FunctionObject = requires(T&& t)
	{
		t.T::operator()(std::declval<Args>()...);
		
		std::is_same_v<nox::FunctionArgsTupleType<decltype(static_cast<decltype(t.T::operator()(std::declval<Args>()...))(T::*)(Args...)>(&T::operator())) >, std::tuple<Args...>>;
	};

	template<class T, class... Args>
	using TTT = decltype(std::declval<T>().T::operator()(std::declval<Args>()...));
}

class FO
{
public:
	int operator()()const
	{
		return 0;
	}

	int operator()(int)const
	{
		return 0;
	}
};
#include	"type_id_test.h"
#include	"delegate_test.h"
namespace
{
	class LocalClass 
	{
	public:
		explicit LocalClass(const nox::int32 v) :v(v)
		{
			NOX_INFO_LINE(U"LocalClass Constructor");
		}

		~LocalClass()
		{
			NOX_INFO_LINE(U"LocalClass Destructor");
		}

		LocalClass(const LocalClass& obj)
		{
			NOX_INFO_LINE(U"LocalClass Copy Constructor");
		}

		LocalClass(LocalClass&& obj)noexcept
		{
			NOX_INFO_LINE(U"LocalClass Move Constructor");
		}

		nox::int32 Func()const { return v; }

		nox::int32 v = 0;
	};
}

namespace nox
{
	template<class T, class U>
	inline constexpr decltype(auto) MakePairEmplace(T&& t, U&& u)noexcept
	{
		return std::make_pair(std::forward<T>(t), std::forward<U>(u));
	}

	class NoxObject
	{
	public:
		NoxObject() {}

		NoxObject(int* int_ptr) {}

		inline NoxObject(const NoxObject& other)noexcept 
		{
			NoxObject* copy = new NoxObject(other.int_ptr);
		}

		int* int_ptr = nullptr;
	};
}

#include <iostream>
inline void MultiCastTest()
{
	nox::MulticastDelegate<int(int)> delegate;
	//delegate.Resize(4);

	struct Local
	{
		int Func(int a)const noexcept { return a + 1; }
	};

	auto l = [](int a) {return a + 1; };
	delegate += +[](int a) {return a + 1; };
	delegate -= +[](int a) {return a + 1; };

	Local local;
	delegate += std::make_pair(nox::Nontype<&Local::Func>, &local);
	delegate -= std::make_pair(nox::Nontype<&Local::Func>, &local);

	delegate += [](int a) {return a + 2; };

	delegate += [](int a) {return a + 3; };

	std::cout << delegate(1) << std::endl;

	auto n = delegate(1);
}

void nox::test::TestReflection()
{

//	nox::util::RemoveEraseIf(v, +[](const nox::Delegate<int()>& a) {return false; });
	MultiCastTest();

	nox::Delegate<int()> delegate = []() {return 10; };
	auto n = delegate();



	constexpr int vv = 0;
//	constexpr Desc d = Desc{.v = std::ref(vv)};

	/*{
		class LC
		{
		public:
			LC(int v) :value(v) {}
			LC(const LC& other) :value(other.value) 
			{
				NOX_INFO_LINE(U"LC Copy Constructor");
			}
			LC(LC&& other) :value(other.value) 
			{
				other.value = 0;
				NOX_INFO_LINE(U"LC Move Constructor");
			}

			~LC()
			{
				NOX_INFO_LINE(U"LC Destructor");
			}

			int Func(int a)const noexcept { return a + value; }
			inline int operator()(int v)const { return this->value + v; }

		private:
			int value = 0;
		};

		class LC2
		{
		public:
			LC2(int& v) :value_(v) {}

			LC2(const LC2& other) :value_(other.value_)
			{
			}

			int& value_;
		};

		int v = 123;
		const LC2 lc2(v);
		const LC2& lc2Ref = lc2;
		LC2* lc3 = nullptr;
		char buffer[sizeof(LC2)]{ 0 };
		lc3 = std::construct_at(reinterpret_cast<LC2*>(buffer), lc2Ref.value_);

		delegate = std::make_pair(nox::Nontype<&LC::Func>, LC(123));

		constexpr bool eaa = std::is_same<void(LC2::*)()&, void(LC2::*)()>::value;

	}*/

//	auto n = delegate(2);
	

//	auto local_class_unique_ptr = std::make_unique<LocalClass>(123);
//	std::shared_ptr<LocalClass> local_class_shared_ptr(local_class_unique_ptr.get());
//
//	LocalClass local_class(8);
//	const LocalClass local_class_const = LocalClass(123);
//
//	std::is_invocable<decltype(&LocalClass::Func), const std::shared_ptr<LocalClass>>::value;
////	using t = nox::util::ToAddressType<LocalClass>;
////	nox::InvokeResultTypeWithTupleLike<decltype(&LocalClass::Func), nox::TupleCatType<t, std::tuple<>>>;
//
//	decltype(auto) p = std::make_pair(nox::Nontype<&LocalClass::Func>, local_class);
//	using t2 = typename decltype(p)::second_type;
//
//	nox::Delegate<int(), 32> delegate2 = nullptr;
//	nox::util::TryToAddress(local_class);
//	delegate2 = std::make_pair(nox::Nontype<&LocalClass::Func>, std::ref(local_class));
//	auto n3 = delegate2;
//	auto n = delegate2();


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
		nox::reflection::GetInvalidType(),
		U"abc",
		U"abc",
		U"abc",
		nox::reflection::GetInvalidType(),
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
		nox::reflection::GetInvalidType(),
		U"abc",
		U"abc",
		U"abc",
		nox::reflection::GetInvalidType(),
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
	nox::reflection::GetInvalidType(),
	nox::reflection::GetInvalidType(),
	+[](nox::not_null<void*> instance_ptr, nox::not_null<void*> value) {},
	+[](not_null<void*> outPtr, not_null<const void*> instance_ptr) {},
	+[](not_null<void*> outPtr, not_null<const void*> instance_ptr) {},
	+[](not_null<void*> instance_ptr, const void* const valuePtr, const std::uint32_t index) {return true; },
	+[](not_null<void*> outPtr, not_null<const void*> instance_ptr, const std::uint32_t index) {return true; },
	+[](not_null<void*> outPtr, not_null<const void*> instance_ptr, const std::uint32_t index) {return true; }
);

#endif // false
