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
	/// @brief 関数を登録した時に呼ばれる
	template<class T>
	inline constexpr void IDelegateRegistered(std::add_lvalue_reference_t<T>)noexcept { }

	/// @brief 関数を登録解除した時に呼ばれる
	template<class T>
	inline constexpr void IDelegateUnregistered(std::add_lvalue_reference_t<T>)noexcept {}

	/// @brief メンバ関数を呼び出すためのインスタンスを取得する
	template<class T>
	constexpr decltype(auto) IDelegateGetInstance(std::add_lvalue_reference_t<T> v)noexcept { return v; }

	template<template<class> class T, class U>
	constexpr decltype(auto) IDelegateGetInstance(std::add_lvalue_reference_t<T<U>> v)noexcept { return v; }

	namespace detail
	{
		/// @brief IDelegateRegisterを呼び出し可能かどうか
		template<class T>
		struct ExistsFunction_IDelegateRegistered
		{
		private:
			template<class _T>
			static consteval std::true_type Test(const decltype(IDelegateRegistered<_T>(*static_cast<std::add_pointer_t<_T>>(nullptr)))*) noexcept;

			template<class>
			static consteval std::false_type Test(...) noexcept;

		public:
			static constexpr bool value = std::is_same_v<decltype(Test<T>(nullptr)), std::true_type>;
		};

		template<template<class> class T>
		struct ExistsFunction_IDelegateRegisteredT1
		{
		private:
			template<template<class> class _T>
			static consteval std::true_type Test(const decltype(IDelegateRegistered<_T>())) noexcept;

			template<template<class> class _T>
			static consteval std::false_type Test(...) noexcept;

		public:
			static constexpr bool value = std::is_same_v<decltype(Test<T>(nullptr)), std::true_type>;
		};


		template<class T>
		struct ExistsFunction_IDelegateUnregistered
		{
		private:
			template<class _T>
			static consteval std::true_type Test(const decltype(IDelegateUnregister<_T>(*static_cast<std::add_pointer_t<_T>>(nullptr)))*) noexcept;

			template<class>
			static consteval std::false_type Test(...) noexcept;

		public:
			static constexpr bool value = std::is_same_v<decltype(Test<T>(nullptr)), std::true_type>;
		};

		template<class T>
		struct ExistsFunction_IDelegateGetInstance
		{
		private:
			template<class _T>
			static consteval std::true_type Test(const decltype(IDelegateGetInstance<_T>(*static_cast<std::add_pointer_t<_T>>(nullptr)))*) noexcept;

			template<class>
			static consteval std::false_type Test(...) noexcept;

		public:
			static constexpr bool value = std::is_same_v<decltype(Test<T>(nullptr)), std::true_type>;
		};

		template<template<class> class T, class U, class ReturnType>
		struct ExistsFunction_IDelegateGetInstanceT1
		{
		private:
			template<template<class> class _T, class _U, class _ReturnType>
			static consteval std::true_type Test(const decltype(IDelegateGetInstance<_T, _U, _ReturnType>(*static_cast<std::add_pointer_t<_T<_U>>>(nullptr)))*) noexcept;

			template<template<class> class _T, class _U, class _ReturnType>
			static consteval std::false_type Test(...) noexcept;

		public:
			static constexpr bool value = std::is_same_v<decltype(Test<T, U, ReturnType>(nullptr)), std::true_type>;
		};
	}

	/// @brief Delegateインターフェース
	template<class _FuncType>
	class IDelegate
	{
	public:
#pragma region 関数実行クラス
		/// @brief 呼び出しクラス基底
		class ICallable
		{
		public:
			virtual FunctionReturnType<_FuncType> Invoke(const FunctionArgsType<_FuncType>&) const = 0;
			virtual constexpr bool Equal(const ICallable& rhs)const noexcept = 0;
			virtual constexpr uint32 GetTypeID()const noexcept = 0;
		};

		template<class T>
		class CallableBase : public ICallable
		{
		public:
			inline constexpr uint32 GetTypeID()const noexcept override final { return nox::util::GetUniqueTypeID<T>(); }
		};

		template<class T>
		class Callable final : public CallableBase<T>
		{
		public:
			inline constexpr explicit Callable(T&& f)noexcept :
				functor_(std::forward<T>(f)) {}

			inline FunctionReturnType<_FuncType> Invoke(const FunctionArgsType<_FuncType>& args) const override
			{
				if constexpr (std::is_void_v< FunctionReturnType<_FuncType>> == true)
				{
					std::apply(functor_, args);
				}
				else
				{
					return std::apply(functor_, args);
				}
			}

			inline constexpr bool Equal(const ICallable& rhs)const noexcept override
			{
				if (this->GetTypeID() != rhs.GetTypeID())
				{
					return false;
				}

				return true;
				return &functor_ == &static_cast<const Callable<T>*>(&rhs)->functor_;
			}

		private:
			const T functor_;
		};

		///@brief	メンバ関数用
		template<class T, class InstanceType> requires(std::is_same_v<InstanceType, std::decay_t<InstanceType>>)
		class CallableMemberFunction : public CallableBase<T>
		{

		public:
			inline explicit CallableMemberFunction(T&& f, InstanceType&& _instance):
				functor_(std::forward<T>(f)),
				instance_(std::forward<InstanceType>(_instance))
			{
				constexpr auto n = util::GetTypeName<InstanceType>();
				IDelegateRegistered<IntrusivePtr<Obj>>(instance_);
				IDelegateGetInstance<InstanceType>(instance_);
				if constexpr (std::is_void_v<decltype(IDelegateRegistered<InstanceType>(instance_))>)
				{
					IDelegateRegistered<InstanceType>(instance_);
				}
			}

			inline ~CallableMemberFunction()
			{
				nox::IDelegateUnregistered< InstanceType>(instance_);
			}

			inline FunctionReturnType<_FuncType> Invoke(const FunctionArgsType<_FuncType>& args) const override
			{
				if constexpr (std::is_void_v< FunctionReturnType<_FuncType>> == true)
				{
			//		if constexpr (nox::IsFunctionObjectValue<IDelegateGetInstance<InstanceType>, InstanceType> == true)
					{
					//	std::apply(functor_, std::tuple_cat(std::make_tuple(IDelegateGetInstance<InstanceType>{}.operator()(std::forward<InstanceType>(instance_))), args));
					}
			//		else
					{
					//	std::apply(functor_, std::tuple_cat(std::make_tuple(std::forward<InstanceType>(instance_)), args));
					}
				}
				else
				{
			//		if constexpr (nox::IsFunctionObjectValue<IDelegateGetInstance<InstanceType>, InstanceType> == true)
					{
					//	return std::apply(functor_, std::tuple_cat(std::make_tuple(IDelegateGetInstance<InstanceType>{}.operator()(std::forward<InstanceType>(instance_))), args));
					}
			//		else 
					{
					//	return std::apply(functor_, std::tuple_cat(std::make_tuple(std::forward<InstanceType>(instance_)), args));
					}
				}
			}

			inline constexpr bool Equal(const ICallable& rhs)const noexcept override
			{
				if (this->GetTypeID() != rhs.GetTypeID())
				{
					return false;
				}

				const CallableMemberFunction<T, InstanceType>* const rhs_ptr = static_cast<const CallableMemberFunction<T, InstanceType>*>(&rhs);

				if (&functor_ != &rhs_ptr->functor_)
				{
					return false;
				}

				if constexpr (HasEqualityCompareValue<InstanceType> == true)
				{

				}
				else
				{
					if (&instance_ != &rhs_ptr->instance_)
					{
						return false;
					}
				}

				return true;
			}
		private:
			InstanceType&& instance_;
			const T functor_;
		};
#pragma endregion

	public:
	
		template<class... Args>
		inline constexpr FunctionReturnType<_FuncType> Invoke(Args&&... args)const
		{
			if constexpr (std::is_void_v< FunctionReturnType<_FuncType>> == true)
			{
				callable_ptr_->Invoke(std::make_tuple(std::forward<Args>(args)...));
			}
			else
			{
				return callable_ptr_->Invoke(std::make_tuple(std::forward<Args>(args)...));
			}
		}

		inline constexpr bool IsEmpty()const noexcept { return callable_ptr_ == nullptr; }

		inline constexpr bool Equal(const IDelegate<_FuncType>& rhs)const noexcept
		{
			if (callable_ptr_ == nullptr || rhs.callable_ptr_ == nullptr)
			{
				return false;
			}

			return callable_ptr_->Equal(*(rhs.callable_ptr_));
		}
	protected:
		inline constexpr IDelegate()noexcept :
			callable_ptr_(nullptr) {}

	protected:
		ICallable* callable_ptr_;
	};

	template<class _FuncType, uint32 _BufferSize = 16>
	class Delegate : public IDelegate<_FuncType>
	{
	private:


	public:
		inline constexpr Delegate()noexcept :
			buffer_{ 0 } {}

		///	メンバ関数のバインド
		template<class T, class U> requires(
		//	std::is_same_v<FunctionClassType<T>, std::remove_pointer_t<U>> &&
		//	std::is_member_function_pointer_v<T> && std::is_pointer_v<U> &&
			sizeof(Delegate::template CallableMemberFunction<T, std::decay_t<U>>) <= (_BufferSize + sizeof(void*))
			)
		inline void Bind(T&& func, U&& instance)
		{
			if (IDelegate<_FuncType>::IsEmpty() == false)
			{
				Unbind();
			}

			Delegate::callable_ptr_ = std::construct_at(static_cast<Delegate::template CallableMemberFunction<T, std::decay_t<U>>*>(static_cast<void*>(buffer_.data())), std::forward<T>(func), std::forward<std::decay_t<U>>(instance));
		}



		template<class T> requires(sizeof(Delegate::template Callable<T>) <= (_BufferSize + sizeof(void*)))
		inline void Bind(T&& func)
		{
			if (IDelegate<_FuncType>::IsEmpty() == false)
			{
				Unbind();
			}

			
			Delegate::callable_ptr_ = std::construct_at(static_cast<Delegate::template Callable<T>*>(static_cast<void*>(buffer_.data())), std::forward<T>(func));
		}

		inline void Unbind()
		{
			if (IDelegate<_FuncType>::callable_ptr_ != nullptr)
			{
				std::destroy_at(IDelegate<_FuncType>::callable_ptr_);
				IDelegate<_FuncType>::callable_ptr_ = nullptr;
			}
			memory::ZeroMem(buffer_.data(), buffer_.size());
		}

	
	private:
		/// @brief メモリ割り当て用バッファ
		std::array<uint8, _BufferSize + sizeof(void*)>	buffer_;
	};

	template<class _FuncType>
	class IMulticastDelegate
	{
	};

	template<class _FuncType, uint32 _BufferSize = 32>
	class MulticastDelegate : public IMulticastDelegate<_FuncType>
	{
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
	struct IntrusivePtrAddReference<Obj>
	{
		inline void operator()(Obj&)noexcept
		{
			std::printf("add");
		}
	};

	template<>
	struct IntrusivePtrReleaseReference<Obj>
	{
		inline void operator()(Obj&)noexcept
		{
			std::printf("add");
		}
	};

	

	/*template <typename T, typename = decltype(foo<T>()) >
	std::true_type test(int);

	template <typename T>
	std::false_type test(...);


	template <typename T>
	using is_foo = decltype(test<T>(0));

	static_assert(is_foo<int>::value);
	static_assert(!is_foo<int*>::value);*/
	template<>
	inline void IDelegateRegistered<Obj*>(Obj*&)
	{

	}

	template<class T>
	inline void IDelegateRegistered(IntrusivePtr<T>&)
	{
	}

	

	template<class T>
	inline constexpr decltype(auto) IDelegateGetInstance(IntrusivePtr<T> v) noexcept
	{
		return v.Get();
	}

	template<>
	inline constexpr decltype(auto) IDelegateGetInstance<Obj>(Obj& v) noexcept
	{
		return v;
	}


	template<class T>
	void func() = delete;

	template<>
	void func<int>() {}

	//template <typename T, class = void>
	//struct IsCallable_IDelegateRegister : std::false_type {};

	//template <typename T>
	//struct IsCallable_IDelegateRegister < T, std::void_t<decltype(IDelegateRegistered<std::decay_t<T>>(*(T*)nullptr)) >> : std::true_type {};

}



int WINAPI::WinMain(_In_ ::HINSTANCE hInstance, _In_opt_ ::HINSTANCE /*hPrevInstance*/, _In_ ::LPSTR /*lpCmdLine*/, _In_ int nCmdShow)
{
	using namespace nox;



	Obj fObj(2);
	nox::IntrusivePtr<Obj> ptr(fObj);
	sizeof(nox::Delegate<void()>);
	sizeof(IntrusivePtrAddReference<Obj>);

	int inta = 0;
//	IDelegateRegistered<int>(inta);
	IDelegateRegistered<Obj>(fObj);
	IDelegateRegistered(ptr);

//	decltype(auto) get_instance_0 = IDelegateGetInstance<int>(inta);
//	decltype(auto) get_instance_1 = IDelegateGetInstance<Obj>(fObj);
//	decltype(auto) get_instance_2 = IDelegateGetInstance(ptr);
	
//	using aa = std::invoke_result_t< std::decay_t<decltype(&DelegateRegistered<Obj>)>, Obj&>;
	constexpr bool bbolconstat2 = nox::detail::ExistsFunction_IDelegateRegistered<Obj>::value;
	constexpr bool bbolconstat222a = nox::detail::ExistsFunction_IDelegateGetInstance<Obj>::value;
	constexpr bool bbolconstat222 = nox::detail::ExistsFunction_IDelegateGetInstanceT1<IntrusivePtr, Obj, Obj*>::value;
	constexpr bool bbolconstat23 = nox::detail::ExistsFunction_IDelegateRegistered<int>::value;
//	using tttt = std::decay_t<decltype(static_cast<void(*)(Obj&)>(DelegateRegistered))>;

//	constexpr bool bbolconstat = ExistsFunction<Obj>::value;
//	static_assert(concepts::IDelegateRegister<Obj>);


	

	int dummyBuffer[120];
	Delegate<void(), 890> func;
	func.Bind([dummyBuffer]() {});

	Delegate<void()> func1;
	func1.Bind([]() {});

	Delegate<void()> func2;
	func2.Bind(&Obj::MemberFunc, ptr);

	Delegate<void()> func3;
	//func3.Bind(&Obj::MemberFunc, &fObj);
	//func3.Invoke();

	bool isF0 = func.Equal(func1);
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