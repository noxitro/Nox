///	@file	delegate.h
///	@brief	delegate
#pragma once
#include	"../memory/memory_util.h"
#include	"../type_traits/function_signature.h"
#include	"../type_traits/type_name.h"

namespace nox
{
	/// @brief 関数を登録した時に呼ばれる
	template<class T>
	inline constexpr void IDelegateRegistered(std::add_lvalue_reference_t<T>)noexcept = delete;

	/// @brief 関数を登録解除した時に呼ばれる
	template<class T>
	inline constexpr void IDelegateUnregistered(std::add_lvalue_reference_t<T>)noexcept = delete;

	/// @brief メンバ関数を呼び出すためのインスタンスを取得する
	template<class T>
	constexpr decltype(auto) IDelegateGetInstance(std::add_lvalue_reference_t<T> v)noexcept = delete;

	namespace detail
	{
		/*	template <typename T, typename = void>
			struct IsCallable_IDelegateRegister : std::false_type {};

			template <typename T>
			struct IsCallable_IDelegateRegister<T, std::void_t<decltype(IDelegateRegistered(*static_cast<std::add_pointer_t<T>>(nullptr)))*>> : std::true_type {};*/


			/// @brief IDelegateRegisterを呼び出し可能かどうか
		template<class T>
		struct IsCallable_IDelegateRegistered
		{
		private:
			template<class _T>
			static consteval std::true_type Test(const decltype(IDelegateRegistered(*static_cast<std::add_pointer_t<_T>>(nullptr)))*) noexcept;

			template<class>
			static consteval std::false_type Test(...) noexcept;

		public:
			static constexpr bool value = std::is_same_v<decltype(Test<T>(nullptr)), std::true_type>;
		};


		template<class T>
		struct ExistsFunction_IDelegateUnregistered
		{
		private:
			template<class _T>
			static consteval std::true_type Test(const decltype(IDelegateUnregister(*static_cast<std::add_pointer_t<_T>>(nullptr)))*) noexcept;

			template<class>
			static consteval std::false_type Test(...) noexcept;

		public:
			static constexpr bool value = std::is_same_v<decltype(Test<T>(nullptr)), std::true_type>;
		};

		template<class T>
		struct IsCallable_IDelegateGetInstance
		{
		private:
			template<class _T>
			static consteval std::true_type Test(const decltype(IDelegateGetInstance(*static_cast<std::add_pointer_t<_T>>(nullptr)))*) noexcept;

			template<class>
			static consteval std::false_type Test(...) noexcept;

		public:
			static constexpr bool value = std::is_same_v<decltype(Test<T>(nullptr)), std::true_type>;
		};
	}

	/// @brief Delegateインターフェース
	template<class _FuncType>
	class IDelegate
	{
	public:
		/// @brief 関数の種類
		enum class Category : uint8
		{
			Invalid,
			Default,
			Member,
			Lambda
		};

#pragma region 関数実行クラス
		/// @brief 呼び出しクラス基底
		class ICallable
		{
		public:
			virtual constexpr FunctionReturnType<_FuncType> Invoke(const FunctionArgsType<_FuncType>&) const = 0;
			virtual constexpr bool Equal(const ICallable& rhs)const noexcept = 0;
			virtual constexpr uint32 GetTypeID()const noexcept = 0;
			virtual constexpr Category GetCategory()const noexcept = 0;
		};

		template<class T, Category category>
		class CallableBase : public ICallable
		{
		public:
			inline constexpr uint32 GetTypeID()const noexcept override final { return nox::util::GetUniqueTypeID<T>(); }
			inline constexpr Category GetCategory()const noexcept override final { return category; }
		};

		/// @brief 通常関数
		template<class T>
		class Callable final : public CallableBase<T, Category::Default>
		{
		public:
			inline constexpr explicit Callable(T&& f)noexcept :
				functor_(std::forward<T>(f)) {}

			inline constexpr FunctionReturnType<_FuncType> Invoke(const FunctionArgsType<_FuncType>& args) const override
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

				return &functor_ == &static_cast<const Callable<T>*>(&rhs)->functor_;
			}

		private:
			const T functor_;
		};

		/// @brief ラムダ式
		template<class T>
		class CallableLambda final : public CallableBase<T, Category::Lambda>
		{
		public:
			inline constexpr explicit CallableLambda(T&& f)noexcept :
				functor_(std::forward<T>(f)) {}

			inline constexpr FunctionReturnType<_FuncType> Invoke(const FunctionArgsType<_FuncType>& args) const override
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

				return &functor_ == &static_cast<const CallableLambda<T>*>(&rhs)->functor_;
			}

		private:
			const T functor_;
		};

		///@brief	メンバ関数用
		template<class T, class InstanceType> requires(std::is_same_v<InstanceType, std::decay_t<InstanceType>>)
			class CallableMemberFunction final : public CallableBase<T, Category::Member>
		{
		public:
			inline explicit CallableMemberFunction(T&& f, InstanceType&& _instance) :
				functor_(std::forward<T>(f)),
				instance_(std::forward<InstanceType>(_instance))
			{
				if constexpr (nox::detail::IsCallable_IDelegateRegistered<std::remove_pointer_t<InstanceType>>::value == true)
				{
					if constexpr (std::is_pointer_v<InstanceType> == true)
					{
						IDelegateRegistered(*instance_);
					}
					else
					{
						IDelegateRegistered(instance_);
					}

				}
			}

			inline ~CallableMemberFunction()
			{
				nox::IDelegateUnregistered< InstanceType>(instance_);
			}

			inline constexpr FunctionReturnType<_FuncType> Invoke(const FunctionArgsType<_FuncType>& args) const override
			{
				if constexpr (std::is_void_v< FunctionReturnType<_FuncType>> == true)
				{
					if constexpr (nox::detail::IsCallable_IDelegateGetInstance<InstanceType>::value == true)
					{
						std::apply(functor_, std::tuple_cat(std::make_tuple(IDelegateGetInstance(instance_)), args));
					}
					else
					{
						std::apply(functor_, std::tuple_cat(std::make_tuple(&instance_), args));
					}
				}
				else
				{
					/*if constexpr (nox::detail::IsCallable_IDelegateGetInstance<InstanceType>::value == true)
					{
						return std::apply(functor_, std::tuple_cat(std::make_tuple(nox::IDelegateGetInstance(instance_)), args));
					}
					else
					{
						return std::apply(functor_, std::tuple_cat(std::make_tuple(&instance_), args));
					}*/
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

				if (&instance_ != &rhs_ptr->instance_)
				{
					return false;
				}

				return true;
			}
		private:
			InstanceType&& instance_;
			const T functor_;
		};
#pragma endregion

	public:

		template<class... Args> requires(
			std::is_invocable_v<_FuncType, Args...>
			)
			inline constexpr FunctionReturnType<_FuncType> Invoke(Args&&... args)const
		{
			NOX_ASSERT(callable_ptr_ != nullptr, U"");

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

		inline constexpr Category GetCategory()const noexcept
		{
			if (IsEmpty() == true)
			{
				return Category::Invalid;
			}

			return callable_ptr_->GetCategory();
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



		/// @brief メンバ関数のバインド
		/// @return 
		template<class T, class U> requires(
			std::is_member_function_pointer_v<T>&&
			std::is_pointer_v<U> == false &&
			std::is_same_v<FunctionClassType<T>, std::decay_t<U>> &&
			//	std::is_member_function_pointer_v<T> && std::is_pointer_v<U> &&
			sizeof(Delegate::template CallableMemberFunction<T, std::decay_t<U>>) <= (_BufferSize + sizeof(void*))
			)
			inline constexpr void Bind(T&& func, U&& instance)
		{
			if (IDelegate<_FuncType>::IsEmpty() == false)
			{
				Unbind();
			}

			Delegate::callable_ptr_ = std::construct_at(
				static_cast<Delegate::template CallableMemberFunction<T, std::decay_t<U>>*>(static_cast<void*>(buffer_.data())),
				std::forward<T>(func),
				std::forward<std::decay_t<U>>(instance)
			);
		}

		/// @brief メンバ関数のバインド　std::unique_ptrやIntrusivePtrなど用
		template<class T, class U> requires(
			std::is_member_function_pointer_v<T>&&
			nox::detail::IsCallable_IDelegateGetInstance<U>::value == true &&
			sizeof(Delegate::template CallableMemberFunction<T, std::decay_t<U>>) <= (_BufferSize + sizeof(void*))
			)
			inline constexpr void Bind(T&& func, U&& instance)
		{

			if (IDelegate<_FuncType>::IsEmpty() == false)
			{
				Unbind();
			}

			Delegate::callable_ptr_ = std::construct_at(static_cast<Delegate::template CallableMemberFunction<T, std::decay_t<U>>*>(static_cast<void*>(buffer_.data())), std::forward<T>(func), std::forward<std::decay_t<U>>(instance));
		}

		/// @brief 通常関数のバインド
		template<class T> requires(
			IsLambdaValue<T> == false &&
			std::is_member_function_pointer_v < T> == false &&
			sizeof(Delegate::template Callable<T>) <= (_BufferSize + sizeof(void*))
			)
			inline constexpr void Bind(T&& func)
		{
			if (IDelegate<_FuncType>::IsEmpty() == false)
			{
				Unbind();
			}

			Delegate::callable_ptr_ = std::construct_at(static_cast<Delegate::template Callable<T>*>(static_cast<void*>(buffer_.data())), std::forward<T>(func));
		}

		/// @brief	ラムダ式のバインド
		template<class T> requires(
			IsLambdaValue<T> == true &&
			std::is_member_function_pointer_v < T> == false &&
			sizeof(Delegate::template CallableLambda<T>) <= (_BufferSize + sizeof(void*))
			)
			inline constexpr void Bind(T&& func)
		{
			if (IDelegate<_FuncType>::IsEmpty() == false)
			{
				Unbind();
			}

			Delegate::callable_ptr_ = std::construct_at(static_cast<Delegate::template CallableLambda<T>*>(static_cast<void*>(buffer_.data())), std::forward<T>(func));
		}

		inline constexpr void Unbind()
		{
			if (IDelegate<_FuncType>::callable_ptr_ != nullptr)
			{
				std::destroy_at(IDelegate<_FuncType>::callable_ptr_);
				IDelegate<_FuncType>::callable_ptr_ = nullptr;
			}
			memory::ZeroMem(buffer_.data(), buffer_.size());
		}

		template<class T>  requires(
			IsLambdaValue<T> == false &&
			std::is_member_function_pointer_v < T> == false &&
			sizeof(Delegate::template CallableLambda<T>) <= (_BufferSize + sizeof(void*))
			)
			inline constexpr Delegate(T&& func) : buffer_{ 0 }
		{
			Delegate::callable_ptr_ = std::construct_at(static_cast<Delegate::template Callable<T>*>(static_cast<void*>(buffer_.data())), std::forward<T>(func));
		}


		template<class T>  requires(
			IsLambdaValue<T> == true &&
			std::is_member_function_pointer_v < T> == false &&
			sizeof(Delegate::template CallableLambda<T>) <= (_BufferSize + sizeof(void*))
			)
			inline constexpr Delegate(T&& func) : buffer_{ 0 }
		{
			Delegate::callable_ptr_ = std::construct_at(static_cast<Delegate::template CallableLambda<T>*>(static_cast<void*>(buffer_.data())), std::forward<T>(func));
		}

		template<class T, class U>
		inline constexpr Delegate(T&& func, U&& instance) : buffer_{ 0 }
		{
			Bind(std::forward<T>(func), std::forward<U>(instance));
		}
	private:
		/// @brief メモリ割り当て用バッファ
		std::array<uint8, _BufferSize + sizeof(void*)>	buffer_;
	};

}