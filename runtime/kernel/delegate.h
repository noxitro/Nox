///	@file	delegate.h
///	@brief	delegate
#pragma once
#include	"algorithm.h"
#include	"assertion.h"
#include	"memory/memory_util.h"
#include	"type_traits/function_signature.h"
#include	"type_traits/type_name.h"

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

			/// @brief 関数
			Default,

			/// @brief メンバ関数
			Member,

			/// @brief 関数オブジェクト
			FunctionObject,

			/// @brief ラムダ式
			Lambda
		};

	protected:
#pragma region 関数実行クラス
		/// @brief 呼び出しクラス基底
		class ICallable
		{
		public:
			virtual ~ICallable() {}
			virtual constexpr FunctionReturnType<_FuncType> Invoke(const FunctionArgsType<_FuncType>&)const = 0;
			inline constexpr bool Equal(const ICallable& rhs)const noexcept
			{
				if (GetTypeID() != rhs.GetTypeID())
				{
					return false;
				}

				if (GetCategory() != rhs.GetCategory())
				{
					return false;
				}

				return DoEqual(rhs);
			}

			virtual constexpr uint32 GetFunctorTypeID()const noexcept = 0;
			virtual constexpr uint32 GetTypeID()const noexcept = 0;
			virtual constexpr Category GetCategory()const noexcept = 0;
			virtual constexpr ICallable* Clone(uint8& buffer) const noexcept= 0;
		protected:
			virtual constexpr bool DoEqual(const ICallable& rhs)const noexcept = 0;
		};

		/// @brief 基底
		template<class T, Category category>
		class CallableBase : public ICallable
		{
		public:
			inline constexpr uint32 GetFunctorTypeID()const noexcept { return util::GetUniqueTypeID<T>(); }
			inline constexpr Category GetCategory()const noexcept override final { return category; }
		};



		/// @brief 通常関数
		template<class T>
		class CallableDefault final : public CallableBase<T, Category::Default>
		{
		public:
			inline constexpr explicit CallableDefault(T&& f)noexcept :
				functor_(std::forward<T>(f)) {}

			inline constexpr uint32 GetTypeID()const noexcept override final { 
				auto name = util::GetTypeName<T>();
				return nox::util::GetUniqueTypeID<std::decay_t<std::remove_pointer_t<decltype(this)>>>();
			}

			inline constexpr FunctionReturnType<_FuncType> Invoke(const FunctionArgsType<_FuncType>& args)const override
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

			inline constexpr bool DoEqual(const ICallable& rhs)const noexcept override
			{
				if (this->GetTypeID() != rhs.GetTypeID())
				{
					return false;
				}

				return functor_ == static_cast<const CallableDefault<T>*>(&rhs)->functor_;
			}

			inline constexpr ICallable* Clone(uint8& buffer)const noexcept override
			{
				return std::construct_at(static_cast<CallableDefault<T>*>(static_cast<void*>(&buffer)), std::forward<T>(this->functor_));
			}
		private:
			T&& functor_;
		};

		template<class T>
		class CallableFunctionObject final: public CallableBase<T, Category::FunctionObject>
		{
		public:
			inline constexpr explicit CallableFunctionObject(T&& f)noexcept :
				functor_(std::forward<T>(f)) {}

			inline uint32 GetTypeID()const noexcept override final { 
				return nox::util::GetUniqueTypeID<std::decay_t<std::remove_pointer_t<decltype(this)>>>(); 
			}

			inline constexpr FunctionReturnType<_FuncType> Invoke(const FunctionArgsType<_FuncType>& args)const override
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

			inline constexpr bool DoEqual(const ICallable& rhs)const noexcept override
			{
				if (this->GetTypeID() != rhs.GetTypeID())
				{
					return false;
				}

				return &functor_ == &static_cast<const CallableFunctionObject<T>*>(&rhs)->functor_;
			}

			inline constexpr ICallable* Clone(uint8& buffer)const noexcept override
			{
				return std::construct_at(static_cast<CallableFunctionObject<T>*>(static_cast<void*>(&buffer)), std::forward<T>(this->functor_));
			}
		private:
			T&& functor_;
		};

		/// @brief 等価比較用オブジェクト
		/// @tparam T 
		/// @tparam InstanceType 
		template<class T, class InstanceType> //requires(std::is_same_v<InstanceType, std::decay_t<InstanceType>>)
			class CallableMemberFunctionEqualObject : public CallableBase<T, Category::Member>
		{
		public:
			inline explicit CallableMemberFunctionEqualObject(T&& f, InstanceType&& _instance) :
				functor_(std::forward<T>(f)),
				instance_(std::forward<InstanceType>(_instance))
			{
			}

			inline constexpr uint32 GetTypeID()const noexcept override final { return nox::util::GetUniqueTypeID<CallableMemberFunctionEqualObject<T, InstanceType>>(); }

			inline constexpr ICallable* Clone(uint8&)const noexcept override
			{
				NOX_ASSERT(false, U"等価比較用オブジェクトのため複製はできません");
				return nullptr;
			}
			
			inline constexpr FunctionReturnType<_FuncType> Invoke(const FunctionArgsType<_FuncType>& args)const override
			{
				if constexpr (std::is_void_v< FunctionReturnType<_FuncType>> == true)
				{
					if constexpr (nox::detail::IsCallable_IDelegateGetInstance<InstanceType>::value == true)
					{
						std::apply(functor_, std::tuple_cat(std::make_tuple(IDelegateGetInstance(instance_)), args));
					}
					else
					{
						std::apply(functor_, std::tuple_cat(std::make_tuple(instance_), args));
					}
				}
				else
				{
					if constexpr (nox::detail::IsCallable_IDelegateGetInstance<InstanceType>::value == true)
					{
						return std::apply(functor_, std::tuple_cat(std::make_tuple(IDelegateGetInstance(instance_)), args));
					}
					else
					{
						return std::apply(functor_, std::tuple_cat(std::make_tuple(instance_), args));
					}
				}
			}

			inline constexpr bool DoEqual(const ICallable& rhs)const noexcept override
			{
				const CallableMemberFunctionEqualObject<T, InstanceType>* const rhs_ptr = static_cast<const CallableMemberFunctionEqualObject<T, InstanceType>*>(&rhs);

				if (functor_ != rhs_ptr->functor_)
				{
					return false;
				}

				if (instance_ != rhs_ptr->instance_)
				{
					return false;
				}

				return true;
			}

		protected:
			InstanceType&& instance_;
			T&& functor_;
		};

		///@brief	メンバ関数用
		template<class T, class InstanceType> //requires(std::is_same_v<InstanceType, std::decay_t<InstanceType>>)
			class CallableMemberFunction final : public CallableMemberFunctionEqualObject<T, InstanceType>
		{
		public:
			inline explicit CallableMemberFunction(T&& f, InstanceType&& _instance) :
				CallableMemberFunctionEqualObject<T, InstanceType>(std::forward<T>(f), std::forward<InstanceType>(_instance))
			{
				if constexpr (nox::detail::IsCallable_IDelegateRegistered<std::remove_pointer_t<InstanceType>>::value == true)
				{
					if constexpr (std::is_pointer_v<InstanceType> == true)
					{
						IDelegateRegistered(*_instance);
					}
					else
					{
						IDelegateRegistered(_instance);
					}

				}
			}

			inline ~CallableMemberFunction()
			{
				if constexpr (nox::detail::ExistsFunction_IDelegateUnregistered<std::remove_pointer_t<InstanceType>>::value == true)
				{
					IDelegateUnregistered(CallableMemberFunctionEqualObject<T, InstanceType>::instance_);
//					nox::IDelegateUnregistered< InstanceType>(CallableMemberFunctionEqualObject<T, InstanceType>::instance_);
				}
			}

			inline constexpr ICallable* Clone(uint8& buffer)const noexcept override
			{
				return std::construct_at(static_cast<CallableMemberFunction<T, InstanceType>*>(static_cast<void*>(&buffer)), std::forward<T>(this->functor_), std::forward<InstanceType>(this->instance_));
			}
		};
#pragma endregion

	public:

		template<class... Args> requires(
			std::is_invocable_v<_FuncType, Args...>
			)
			inline constexpr FunctionReturnType<_FuncType> Invoke(Args&&... args)const
		{
			NOX_ASSERT(this->callable_ptr_ != nullptr, U"");

			if constexpr (std::is_void_v< FunctionReturnType<_FuncType>> == true)
			{
				this->callable_ptr_->Invoke(std::make_tuple(std::forward<Args>(args)...));
			}
			else
			{
				return this->callable_ptr_->Invoke(std::make_tuple(std::forward<Args>(args)...));
			}
		}

		inline constexpr bool IsEmpty()const noexcept { return this->callable_ptr_ == nullptr; }

		template<class T, class U> 
			//requires(
			//std::is_member_function_pointer_v<T> &&
			//sizeof(CallableMemberFunction<T, std::decay_t<U>>) <= kRealBufferSize
			////std::is_same_v<FunctionClassType<T>, std::decay_t<U>> 
			//)
		inline constexpr bool Equal(T&& functor, U&& instance)const noexcept
		{
			if (IsEmpty() == true)
			{
				return false;
			}

			if (this->callable_ptr_->GetTypeID() != util::GetUniqueTypeID<CallableMemberFunctionEqualObject<T, U>>())
			{
				return false;
			}

			return this->callable_ptr_->Equal(CallableMemberFunctionEqualObject<T, U>(std::forward<T>(functor), std::forward<U>(instance)));
		}

		template<class T, class U> requires(
			std::is_member_function_pointer_v<T>&&
	//		sizeof(CallableMemberFunction<T, std::decay_t<U>>) <= kRealBufferSize &&
		//	std::is_pointer_v<U> == false &&
			nox::detail::IsCallable_IDelegateGetInstance<U>::value == true 
			)
			inline constexpr bool Equal(T&& functor, U&& instance)const noexcept
		{
			if (IsEmpty() == true)
			{
				return false;
			}

			if (callable_ptr_->GetTypeID() != util::GetUniqueTypeID<CallableMemberFunctionEqualObject<T, U>>())
			{
				return false;
			}

			return callable_ptr_->Equal(CallableMemberFunctionEqualObject<T, U>());
		}

		template<class T> requires(			nox::IsGlobalFunctionPointerValue < T> == true 	)
			inline constexpr bool Equal(T&& functor)const noexcept
		{
			if (IsEmpty() == true)
			{
				return false;
			}

			if (callable_ptr_->GetTypeID() != util::GetUniqueTypeID<CallableDefault<T>>())
			{
				return false;
			}

			return callable_ptr_->Equal(CallableDefault<T>(std::forward<T>(functor)));
		}

		template<class T> requires(	nox::IsFunctionObjectValue<std::decay_t<T>> == true)
			inline constexpr bool Equal(T&& functor)const noexcept
		{
			if (IsEmpty() == true)
			{
				return false;
			}

			auto name = util::GetTypeName< CallableFunctionObject<std::decay_t<T>>>();
			if (callable_ptr_->GetTypeID() != util::GetUniqueTypeID<CallableFunctionObject<std::decay_t<T>>>())
			{
				return false;
			}

			return callable_ptr_->Equal(CallableFunctionObject<T>(std::forward<T>(functor)));
		}

		inline constexpr bool Equal(const IDelegate<_FuncType>& rhs)const noexcept
		{
			if (IsEmpty() == true || rhs.IsEmpty() == true)
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

		inline constexpr IDelegate(const IDelegate& other)noexcept = delete;
		inline constexpr IDelegate(IDelegate& other)noexcept = delete;

	protected:
		ICallable* callable_ptr_;
	};

	template<class _FuncType, uint32 _BufferSize = 24>
	class Delegate : public IDelegate<_FuncType>
	{
	private:
		
		template<class T>
		using CallableDefaultType = typename IDelegate<_FuncType>::template CallableDefault<T>;

		template<class T>
		using CallableFunctionObjectType = typename IDelegate<_FuncType>::template CallableFunctionObject<T>;

		template<class T, class InstanceType>
		using CallableMemberFunctionType = typename IDelegate<_FuncType>::template CallableMemberFunction<T, InstanceType>;

	public:
		/// @brief 実際のメモリ確保サイズ([_BufferSize]+仮想テーブルサイズ)
		static constexpr uint32 kRealBufferSize = _BufferSize + sizeof(void*);

		/// @brief IDelegate<_FuncType>::Callable<T>のサイズ
		template<class T>
		static constexpr bool CallableDefaultTypeSize = sizeof(CallableDefaultType<T>);
		template<class T>
		static constexpr bool CallableFunctionObjectTypeSize = sizeof(CallableFunctionObjectType<T>);
		template<class T, class InstanceType>
		static constexpr bool CallableMemberFunctionTypeSize = sizeof(CallableMemberFunctionType<T, InstanceType>);

	public:
		inline constexpr Delegate()noexcept :
			buffer_{ 0 } {}

		inline constexpr Delegate(const Delegate& rhs)noexcept :
			buffer_(rhs.buffer_)
		{
			if (rhs.callable_ptr_ != nullptr)
			{
				this->callable_ptr_ = rhs.callable_ptr_->Clone(*buffer_.data());
			}
		}

		inline constexpr Delegate(Delegate&& rhs)noexcept :
			buffer_(rhs.buffer_)
		{
			if (rhs.callable_ptr_ != nullptr)
			{
				this->callable_ptr_ = rhs.callable_ptr_->Clone(*buffer_.data());
				rhs.callable_ptr_ = nullptr;
			}
			rhs.buffer_ = { 0 };
		}

		template<class T>
		inline constexpr Delegate(Delegate<T>&& rhs)noexcept:
			buffer_(rhs.buffer_)
		{
			if (rhs.callable_ptr_ != nullptr)
			{
				this->callable_ptr_ = rhs.callable_ptr_->Clone(*buffer_.data());
				rhs.callable_ptr_ = nullptr;
			}
			rhs.buffer_ = { 0 };
		}

		inline constexpr Delegate& operator=(const Delegate& rhs)
		{
			buffer_ = rhs.buffer_;
			if (rhs.callable_ptr_ != nullptr)
			{
				this->callable_ptr_ = rhs.callable_ptr_->Clone(*buffer_.data());
			}
			return *this;
		}

		inline constexpr Delegate& operator=(Delegate&& rhs)
		{
			buffer_ = rhs.buffer_;
			if (rhs.callable_ptr_ != nullptr)
			{
				this->callable_ptr_ = rhs.callable_ptr_->Clone(*buffer_.data());
				rhs.callable_ptr_ = nullptr;
			}
			rhs.buffer_ = { 0 };
		}

		inline constexpr bool operator==(const Delegate& rhs)const noexcept
		{
			return this->Equal(rhs);
		}

		/*inline constexpr Delegate& operator=(const Delegate& other)noexcept
		{
			*this = Delegate(other);
			return *this;
		}*/

		inline ~Delegate()
		{
			Unbind();
		}

		/// @brief メンバ関数のバインド
		template<class T, class U> requires(
			std::is_member_function_pointer_v<T> &&
			(std::is_same_v< nox::FunctionClassType<T>, std::decay_t<U>> || std::is_same_v<nox::FunctionClassType<T>, std::remove_pointer_t<std::decay_t<U>>>) &&
			sizeof(CallableMemberFunctionType<T, std::decay_t<U>>) <= kRealBufferSize
			)
			inline constexpr void Bind(T&& func, U&& instance)
		{
			NOX_ASSERT(util::IsNullPointer(std::forward<U>(instance)) == false, U"instanceがnullptrです");

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
			sizeof(CallableMemberFunctionType<T, std::decay_t<U>>) <= kRealBufferSize
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
			nox::IsFunctionObjectValue<T> == false &&
			nox::IsGlobalFunctionPointerValue < T> == true &&
			sizeof(CallableDefaultType<T>) <= kRealBufferSize
			)
			inline constexpr void Bind(T&& func)
		{
			if (IDelegate<_FuncType>::IsEmpty() == false)
			{
				Unbind();
			}

//			Delegate::callable_ptr_ = std::construct_at(static_cast<Delegate::template CallableDefault<T>*>(static_cast<void*>(buffer_.data())), std::forward<T>(func));
			Delegate::callable_ptr_ = std::construct_at(static_cast<CallableDefaultType<T>*>(static_cast<void*>(buffer_.data())), std::forward<T>(func));
		}

		/// @brief 関数オブジェクトのバインド
		template<class T> requires(
			nox::IsFunctionObjectValue<std::decay_t<T>> == true &&
			sizeof(CallableFunctionObjectType<T>) <= kRealBufferSize
			)
			inline constexpr void Bind(T&& func)
		{
			if (IDelegate<_FuncType>::IsEmpty() == false)
			{
				Unbind();
			}

			Delegate::callable_ptr_ = std::construct_at(static_cast<CallableFunctionObjectType<T>*>(static_cast<void*>(buffer_.data())), std::forward<T>(func));
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

		/// @brief 通常関数
		template<class T>  requires(
			nox::IsGlobalFunctionPointerValue < T> == true &&
			sizeof(CallableDefaultType<T>) <= kRealBufferSize
			)
			inline constexpr explicit Delegate(T&& func) : buffer_{ 0 }
		{
			Delegate::callable_ptr_ = std::construct_at(static_cast<CallableDefaultType<T>*>(static_cast<void*>(buffer_.data())), std::forward<T>(func));
		}

		/// @brief 関数オブジェクト
		template<class T>  requires(
			nox::IsFunctionObjectValue<std::decay_t<T>> == true &&
			sizeof(CallableFunctionObjectType<T>) <= kRealBufferSize
			)
			inline constexpr Delegate(T&& func) : buffer_{ 0 }
		{
			Delegate::callable_ptr_ = std::construct_at(static_cast<CallableFunctionObjectType<T>*>(static_cast<void*>(buffer_.data())), std::forward<T>(func));
		}


		/// @brief メンバ関数のバインド
		template<class T, class U> requires(
			std::is_member_function_pointer_v<T>&&
		//	std::is_pointer_v<U> == false &&
			sizeof(CallableMemberFunctionType<T, std::decay_t<U>>) <= kRealBufferSize
			)
			inline constexpr explicit Delegate(T&& func, U&& instance) : buffer_{ 0 }
		{
			Delegate::callable_ptr_ = std::construct_at(
				static_cast<CallableMemberFunctionType<T, std::decay_t<U>>*>(static_cast<void*>(buffer_.data())),
				std::forward<T>(func),
				std::forward<std::decay_t<U>>(instance)
			);
		}

		/// @brief メンバ関数のバインド　std::unique_ptrやIntrusivePtrなど用
		template<class T, class U> requires(
			std::is_member_function_pointer_v<T>&&
			std::is_pointer_v<U> == false &&
			nox::detail::IsCallable_IDelegateGetInstance<U>::value == true &&
			sizeof(CallableMemberFunctionType<T, std::decay_t<U>>) <= kRealBufferSize
			)
			inline constexpr explicit Delegate(T&& func, U&& instance) : buffer_{ 0 }
		{
			Delegate::callable_ptr_ = std::construct_at(static_cast<CallableMemberFunctionType<T, std::decay_t<U>>*>(static_cast<void*>(buffer_.data())), std::forward<T>(func), std::forward<std::decay_t<U>>(instance));
		}
	private:
		/// @brief メモリ割り当て用バッファ
		std::array<uint8, _BufferSize + sizeof(void*)>	buffer_;
	};

}