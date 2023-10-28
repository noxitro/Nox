///	@file	function.h
///	@brief	function
#pragma once
#include	"../type_traits/concepts.h"
#include	"../type_traits/function_signature.h"
#include	"basic_definition.h"
#include	"../memory/memory_util.h"

namespace nox
{

	///// @brief 関数
	///// @tparam _FuncType 
	///// @tparam BufferSize 
	//template <class _FuncType, std::size_t BufferSize = 32>
	//class Function
	//{
	//public:
	//	/// @brief 関数の種類
	//	enum class Category : u8
	//	{
	//		/// @brief 不明
	//		Invalid,

	//		/// @brief メンバ関数
	//		Member,

	//		/// @brief グローバル関数
	//		Global,

	//		/// @brief ラムダ式
	//		Lambda
	//	};

	//	inline constexpr Function()noexcept :
	//		buffer_{ 0 },
	//		callable_ptr_(nullptr) {}

	//	template <concepts::MemberFunctionPointer _F, std::derived_from<FunctionClassType<_F>> _InstanceType>
	//		requires(
	//		std::is_same_v<FunctionArgsType<F>, FunctionArgsType<_FuncType>> == true
	//		)
	//	inline constexpr Function(_F f, _InstanceType& instance)noexcept :
	//		buffer_{ 0 },
	//		callable_ptr_(nullptr) 
	//	{
	//			Bind(f, instance);
	//	}

	//	inline ~Function() { Unbind(); }

	//	inline explicit Function(const Function& other)
	//	{
	//		if (other.callable_ptr_ != nullptr)
	//		{
	//			other.callable_ptr_->Clone(buffer_.data(), callable_ptr_);
	//		}
	//	}

	//	inline explicit	Function(Function&& other)
	//	{
	//		if (other.callable_ptr_ != nullptr)
	//		{
	//			other.callable_ptr_->Clone(buffer_.data(), callable_ptr_);

	//			other.Unbind();
	//		}
	//	}

	//	/// @brief メンバ変数のバインド
	//	/// @tparam F 
	//	/// @tparam _InstanceType 
	//	/// @param f 
	//	/// @param instance 
	//	template <typename F, class _InstanceType>
	//		requires(
	//	std::is_same_v<FunctionArgsType<F>, FunctionArgsType<_FuncType>> == true &&
	//		std::is_base_of_v<FunctionClassType<F>, _InstanceType>
	//		)
	//		inline void Bind(F f, _InstanceType& instance)
	//	{
	//		if (callable_ptr_ != nullptr)
	//		{
	//			Unbind();
	//		}

	//		auto lambda = [f, &instance](const FunctionArgsType<F>& args)
	//			noexcept(IsFunctionNoexceptValue<F>)
	//			{
	//				if constexpr (std::is_void_v<FunctionReturnType<F>> == true)
	//				{
	//					std::apply(f, std::tuple_cat(std::make_tuple(instance), args));
	//				}
	//				else
	//				{
	//					return std::apply(f, std::tuple_cat(std::make_tuple(instance), args));
	//				}
	//			};

	//		static_assert(sizeof(decltype(Callable(lambda, typeid(f), static_cast<const void*>(&instance)))) <= (BufferSize + sizeof(void*)));

	//		callable_ptr_ = new (buffer_.data()) Callable(lambda, typeid(f), static_cast<const void*>(&instance));
	//	}

	//	/// @brief 通常関数のバインド
	//	/// @tparam F 
	//	/// @param f 
	//	template <typename F>
	//		requires(
	//	std::is_function_v<F> &&
	//	nox::IsGlobalFunctionPointerValue<F> &&
	//	std::is_same_v<FunctionArgsType<F>, FunctionArgsType<_FuncType>> == true
	//		)
	//		inline void Bind(F f)
	//	{
	//		if (callable_ptr_ != nullptr)
	//		{
	//			Unbind();
	//		}

	//		auto lambda = [f](const FunctionArgsType<F>& args)
	//			noexcept(IsFunctionNoexceptValue<F>)
	//			{
	//				if constexpr (std::is_void_v<FunctionReturnType<F>> == true)
	//				{
	//					std::apply(f, args);
	//				}
	//				else
	//				{
	//					return std::apply(f, args);
	//				}
	//			};

	//		static_assert(sizeof(decltype(Callable(lambda, typeid(f)))) <= (BufferSize + sizeof(void*)));

	//		callable_ptr_ = new (buffer_.data()) Callable(lambda, typeid(f));
	//	}

	//	template <typename F>
	//		requires(
	//		std::is_same_v<FunctionArgsType<F>, FunctionArgsType<_FuncType>> == true &&
	//		nox::IsLambdaValue<F>
	//		)
	//		inline void Bind(F f)
	//	{
	//		if (callable_ptr_ != nullptr)
	//		{
	//			Unbind();
	//		}

	//		auto lambda = [f](const FunctionArgsType<F>& args)
	//			noexcept(IsFunctionNoexceptValue<F>)
	//			{
	//				if constexpr (std::is_void_v<FunctionReturnType<F>> == true)
	//				{
	//					std::apply(f, args);
	//				}
	//				else
	//				{
	//					return std::apply(f, args);
	//				}
	//			};

	//		static_assert(sizeof(decltype(Callable(lambda, typeid(f)))) <= (BufferSize + sizeof(void*)));

	//		callable_ptr_ = new (buffer_.data()) Callable(lambda, typeid(f));
	//	}

	//	inline void Unbind()
	//	{
	//		if (callable_ptr_)
	//		{
	//			callable_ptr_->~ICallable();
	//			callable_ptr_ = nullptr;
	//		}

	//		memory::ZeroMem(buffer_.data(), buffer_.size());
	//	}

	//	template<class... Args> requires(std::is_same_v<std::tuple<Args...>, FunctionArgsType<_FuncType>>)
	//		inline	nox::FunctionReturnType<_FuncType> Invoke(const Args&... args)
	//	{
	//		if constexpr (std::is_void_v< FunctionReturnType<_FuncType>> == true)
	//		{
	//			(*callable_ptr_)(std::make_tuple(args...));
	//		}
	//		else
	//		{
	//			return (*callable_ptr_)(std::make_tuple(args...));
	//		}
	//	}

	//	template<class _F, std::derived_from<FunctionClassType<_F>> _InstanceType> requires(
	//		std::is_member_function_pointer_v<_F>
	//		)
	//	inline constexpr bool Equal(_F func, _InstanceType& instance)const noexcept
	//	{
	//		if (callable_ptr_ == nullptr)
	//		{
	//			return false;
	//		}
	//		return callable_ptr_->typeinfo == typeid(func) && callable_ptr_->instance_ptr == static_cast<const void*>( & instance);
	//	}

	//	inline Function& operator=(const Function& other)
	//	{
	//		if (&other == this)
	//		{
	//			return *this;
	//		}

	//		Unbind();

	//		if (other.callable_ptr_ != nullptr)
	//		{
	//			other.callable_ptr_->Clone(buffer_.data(), callable_ptr_);
	//		}

	//		return *this;
	//	}

	//	inline Function& operator=(Function&& other)
	//	{
	//		if (&other == this)
	//			return *this;

	//		Unbind();

	//		if (other.callable_ptr_ != nullptr)
	//		{
	//			other.callable_ptr_->Clone(buffer_.data(), callable_ptr_);

	//			other.Unbind();
	//		}

	//		return *this;
	//	}
	//public:

	//	struct ICallable
	//	{
	//		const std::type_info& typeinfo;
	//		const void* const instance_ptr;

	//		inline constexpr explicit ICallable(const std::type_info& _typeinfo, const void* _instance_ptr)noexcept :
	//			typeinfo(_typeinfo),
	//			instance_ptr(_instance_ptr) {}

	//		virtual ~ICallable() = default;
	//		virtual FunctionReturnType<_FuncType> operator()(const FunctionArgsType<_FuncType>&) const = 0;
	//		virtual void Clone(not_null<uint8_t*> buffer_, ICallable*& callablePtr) const = 0;
	//	};

	//	template <typename T> requires(std::is_invocable_v<T, const FunctionArgsType<std::decay_t<_FuncType>>&>)
	//		struct Callable : public ICallable
	//	{
	//		const T functor;
	//		

	//		inline constexpr explicit Callable(T f, const std::type_info& _typeinfo, const void* _instance_ptr = nullptr)noexcept :
	//			ICallable(_typeinfo, _instance_ptr),
	//			functor(f)
	//		{}

	//		inline FunctionReturnType<_FuncType> operator()(const FunctionArgsType<std::decay_t<_FuncType>>& args) const override
	//		{
	//			if constexpr (std::is_void_v< FunctionReturnType<_FuncType>> == true)
	//			{
	//				std::invoke(functor, args);
	//			}
	//			else
	//			{
	//				return std::invoke(functor, args);
	//			}
	//		}

	//		inline void Clone(not_null<uint8_t*> buffer_, ICallable*& callablePtr) const override
	//		{
	//			
	//			callablePtr = new (buffer_.get()) Callable(functor, static_cast<const ICallable*>(this)->typeinfo, static_cast<const ICallable*>(this)->instance_ptr);
	//		}
	//	};

	//private:
	//	std::array<u8, BufferSize + sizeof(void*)> buffer_;
	//	ICallable* callable_ptr_;

	//};
}