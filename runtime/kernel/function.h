//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	function.h
///	@brief	function
#pragma once
#include	"type_traits/concepts.h"
#include	"type_traits/function_signature.h"
#include	"basic_definition.h"
#include	"memory/memory_util.h"

#include	<memory>
#include	<functional>

//namespace nox
//{
//
//	/// @brief 関数
//	/// @tparam _FuncType 
//	/// @tparam BufferSize 
//	template <class _FuncType, size_t BufferSize = 32>
//	class Function
//	{
//	public:
//		/// @brief 関数の種類
//		enum class Category : uint8
//		{
//			/// @brief 不明
//			Invalid,
//
//			/// @brief メンバ関数
//			Member,
//
//			/// @brief グローバル関数
//			Global,
//
//			/// @brief ラムダ式
//			Lambda
//		};
//
//		inline constexpr Function()noexcept :
//			buffer_{ 0 },
//			callable_ptr_(nullptr) {}
//
//		template <concepts::MemberFunctionPointer _F, std::derived_from<FunctionClassType<_F>> _InstanceType>
//			requires(
//			std::is_same_v<FunctionArgsType<F>, FunctionArgsType<_FuncType>> == true
//			)
//		inline constexpr Function(_F&& f, _InstanceType&& instance)noexcept :
//			buffer_{ 0 },
//			callable_ptr_(nullptr) 
//		{
//			Bind(std::forward(f), std::forward(instance));
//		}
//
//		inline ~Function() { Unbind(); }
//
//		inline explicit Function(const Function& other)
//		{
//			if (other.callable_ptr_ != nullptr)
//			{
//				other.callable_ptr_->Clone(buffer_.data(), callable_ptr_);
//			}
//		}
//
//		inline explicit	Function(Function&& other)
//		{
//			if (other.callable_ptr_ != nullptr)
//			{
//				other.callable_ptr_->Clone(buffer_.data(), callable_ptr_);
//
//				other.Unbind();
//			}
//		}
//
//		/// @brief メンバ変数のバインド
//		/// @tparam F 
//		/// @tparam _InstanceType 
//		/// @param f 
//		/// @param instance 
//		template <typename F, class _InstanceType>
//			requires(
//			(sizeof(Callable<F>) <= (BufferSize + sizeof(void*)) ) == true &&
//			std::is_same_v<FunctionArgsType<F>, FunctionArgsType<_FuncType>> == true &&
//			std::is_base_of_v<FunctionClassType<F>, _InstanceType>
//			)
//			inline void Bind(F&& f, _InstanceType&& instance)
//		{
//			if (callable_ptr_ != nullptr)
//			{
//				Unbind();
//			}
//
//			const auto lambda = [f, &instance](const FunctionArgsType<F>& args)
//				noexcept(IsFunctionNoexceptValue<F>)
//				{
//					if constexpr (std::is_void_v<FunctionReturnType<F>> == true)
//					{
//						std::apply(std::forward(f), std::tuple_cat(std::make_tuple(std::forward(instance)), args));
//					}
//					else
//					{
//						return std::apply(std::forward(f), std::tuple_cat(std::make_tuple(std::forward(instance)), args));
//					}
//				};
//
//			const size_t func_bits = std::bit_cast<size_t>(std::forward(f));
//			callable_ptr_ = std::construct_at(static_cast<Callable<F>*>(buffer_.data()), lambda, func_bits, static_cast<const void*>(&instance));
//		}
//
//		/// @brief 通常関数のバインド
//		/// @tparam F 
//		/// @param f 
//		template <typename F>
//			requires(
//		(sizeof(Callable<F>) <= (BufferSize + sizeof(void*))) == true &&
//		std::is_function_v<F> &&
//		nox::IsGlobalFunctionPointerValue<F> &&
//		std::is_same_v<FunctionArgsType<F>, FunctionArgsType<_FuncType>> == true
//			)
//			inline void Bind(F&& f)
//		{
//			if (callable_ptr_ != nullptr)
//			{
//				Unbind();
//			}
//
//			const auto lambda = [f](const FunctionArgsType<F>& args)
//				noexcept(IsFunctionNoexceptValue<F>)
//				{
//					if constexpr (std::is_void_v<FunctionReturnType<F>> == true)
//					{
//						std::apply(std::forward(f), args);
//					}
//					else
//					{
//						return std::apply(std::forward(f), args);
//					}
//				};
//
//			const size_t func_bits = std::bit_cast<size_t>(std::forward(f));
//
//			callable_ptr_ = std::construct_at(static_cast<Callable<F>*>(buffer_.data()), lambda, func_bits);
//		}
//
//		/// @brief ラムダ式のバインド
//		template <typename F>
//			requires(
//		//	(sizeof(Callable<F>) <= (BufferSize + sizeof(void*))) == true &&
//			std::is_same_v<FunctionArgsType<F>, FunctionArgsType<_FuncType>> == true &&
//			nox::IsLambdaValue<F>
//			)
//			inline void Bind(F&& f)
//		{
//			if (callable_ptr_ != nullptr)
//			{
//				Unbind();
//			}
//		
//			const auto lambda = [f](const FunctionArgsType<F>& args)
//				noexcept(IsFunctionNoexceptValue<F>)
//				{
//					if constexpr (std::is_void_v<FunctionReturnType<F>> == true)
//					{
//					//	std::apply(std::forward<F>(f), args);
//					}
//					else
//					{
//						return std::apply(std::forward<F>(f), args);
//					}
//				};
//
//		//	const size_t func_bits = std::bit_cast<size_t>(f);
//
//		//	callable_ptr_ = std::construct_at(static_cast<Callable<F>*>(buffer_.data()), lambda, func_bits);
//		}
//
//		/// @brief バインド解除
//		inline void Unbind()
//		{
//			if (callable_ptr_)
//			{
//				//	std::has_virtual_destructor
//				std::destroy_at(callable_ptr_);
//				callable_ptr_ = nullptr;
//			}
//
//			memory::ZeroMem(buffer_.data(), buffer_.size());
//		}
//
//		/// @brief 関数の実行
//		/// @tparam ...Args 
//		/// @param ...args 
//		/// @return 
//		template<class... Args> requires(std::is_same_v<std::tuple<Args...>, FunctionArgsType<_FuncType>>)
//			inline	nox::FunctionReturnType<_FuncType> Invoke(Args&&... args)
//		{
//			if constexpr (std::is_void_v< FunctionReturnType<_FuncType>> == true)
//			{
//				(*callable_ptr_)(std::make_tuple(std::forward<Args>(args)...));
//			}
//			else
//			{
//				return (*callable_ptr_)(std::make_tuple(std::forward<Args>(args)...));
//			}
//		}
//
//		/// @brief 等価比較
//		template<class _F, std::derived_from<FunctionClassType<_F>> _InstanceType> requires(
//			std::is_member_function_pointer_v<_F>
//			)
//		inline constexpr bool Equal(_F&& func, const _InstanceType& instance)const noexcept
//		{
//			if (callable_ptr_ == nullptr)
//			{
//				return false;
//			}
//
//			const size_t func_bits = std::bit_cast<size_t>(std::forward(func));
//			return callable_ptr_->function_bits == func_bits && callable_ptr_->instance_ptr == static_cast<const void*>( & instance);
//		}
//
//		inline Function& operator=(const Function& other)
//		{
//			if (&other == this)
//			{
//				return *this;
//			}
//
//			Unbind();
//
//			if (other.callable_ptr_ != nullptr)
//			{
//				other.callable_ptr_->Clone(buffer_.data(), callable_ptr_);
//			}
//
//			return *this;
//		}
//
//		inline Function& operator=(Function&& other)
//		{
//			if (&other == this)
//				return *this;
//
//			Unbind();
//
//			if (other.callable_ptr_ != nullptr)
//			{
//				other.callable_ptr_->Clone(buffer_.data(), callable_ptr_);
//
//				other.Unbind();
//			}
//
//			return *this;
//		}
//
//
//
//	public:
//
//		struct ICallable
//		{
//			/// @brief 関数のビット値
//			const size_t function_bits;
//
//			/// @brief	メンバ関数のインスタンス
//			///			バインドされた関数の比較に使用
//			const void* const instance_ptr;
//
//			inline constexpr explicit ICallable(const size_t _function_bits, const void* _instance_ptr)noexcept :
//				function_bits(_function_bits),
//				instance_ptr(_instance_ptr) {}
//
//			virtual ~ICallable() = default;
//
//			/// @brief 関数実行
//			virtual FunctionReturnType<_FuncType> Invoke(const FunctionArgsType<_FuncType>&) const = 0;
//
//			virtual void Clone(not_null<uint8_t*> buffer_, ICallable*& callablePtr) const = 0;
//		};
//
//		template <typename T> //requires(std::is_invocable_v<T, const FunctionArgsType<std::decay_t<_FuncType>>&>)
//			struct Callable : public ICallable
//		{
//			/// @brief 関数実行オブジェクト
//			const T functor;
//
//			inline constexpr explicit Callable(T&& f, const size_t _function_bits, const void* _instance_ptr = nullptr)noexcept :
//				ICallable(_function_bits, _instance_ptr),
//				functor(f)
//			{}
//
//			/// @brief 関数実行
//			inline FunctionReturnType<_FuncType> Invoke(const FunctionArgsType<std::decay_t<_FuncType>>& args) const override
//			{
//				if constexpr (std::is_void_v< FunctionReturnType<_FuncType>> == true)
//				{
//				//	std::invoke(functor, args);
//				}
//				else
//				{
//					return std::invoke(functor, args);
//				}
//			}
//
//			inline void Clone(not_null<uint8_t*> buffer_, ICallable*& callablePtr) const override
//			{
////				callablePtr = std::construct_at(static_cast<Callable<T>*>(buffer_.get()), static_cast<const ICallable*>(this)->function_bits, static_cast<const ICallable*>(this)->instance_ptr);
//			//	callablePtr = std::construct_at(static_cast<Callable<T>*>(buffer_.get()), 0);
//				//	callablePtr = new (buffer_.get()) Callable<T>(functor, static_cast<const ICallable*>(this)->function_bits, static_cast<const ICallable*>(this)->instance_ptr);
//			}
//		};
//
//	private:
//		std::array<uint8, BufferSize + sizeof(void*)> buffer_;
//		ICallable* callable_ptr_;
//
//	};
//}