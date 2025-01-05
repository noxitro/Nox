//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	delegate_test.h
///	@brief	delegate_test
#pragma once

namespace nox
{
	/// @brief 
	struct DelegateTag 
	{
	private:
		inline consteval DelegateTag()noexcept = delete;
	};

	struct DelegateFunctionObjectTag : DelegateTag
	{
	};

	/// @brief 全ての呼び出しオブジェクトを受け入れるタグ
	struct DelegateAnyTag : DelegateFunctionObjectTag
	{
	};

	namespace detail
	{
		template<bool IsNoexcept, class _ResultType, class ArgsTuple>
		class DelegateCallableBase;

		template<bool IsNoexcept, class _ResultType, template<class...> class ArgsTuple, class... Args>
		class DelegateCallableBase<IsNoexcept, _ResultType, ArgsTuple<Args...>>
		{

		};

		template<bool IsNoexcept, class _ResultType, class ArgsTuple>
		class DelegateCallable;

		template<bool IsNoexcept, class _ResultType, template<class...> class ArgsTuple, class... Args>
		class DelegateCallable<IsNoexcept, _ResultType, ArgsTuple<Args...>> : public DelegateCallableBase<IsNoexcept, _ResultType, ArgsTuple<Args...>>
		{

		};

		template<class CallableBase, class _FunctionType>
		struct CallableDesc
		{
			nox::FunctionResultType<_FunctionType>(*const call)(const CallableBase&, const nox::FunctionArgsTupleType<_FunctionType>& args)noexcept(nox::IsFunctionNoexceptValue<_FunctionType>);
		//	_Result(* const call_ref)(CallableBase&&, Args&&... args)noexcept(IsNoexcept);
			void(* const copy)(const CallableBase&, std::span<nox::uint8>)noexcept(false);
			void(* const move)(CallableBase&&, std::span<nox::uint8>)noexcept(false);
			bool(* const equal)(const CallableBase&, const CallableBase&)noexcept;
			void(* const dispose)(CallableBase& self)noexcept(false);
			const nox::reflection::Type& callable_type;
			const nox::reflection::Type& functor_type;

			const nox::reflection::Type& instance_type;
			const nox::NontypeId& member_pointer_id;
		};
	}

	/// @brief 具体的なサイズやアラインを意識せず使用できる基底デリゲートクラス
	/// @tparam _FunctionType 
	template<class _FunctionType>
	class DelegateBase
	{
	public:
		using FunctionType = _FunctionType;
	protected:

#pragma region Callable
		class CallableBase
		{
		protected:
			struct Desc
			{
				nox::FunctionResultType<_FunctionType>(* const call)(const CallableBase&, const nox::FunctionArgsTupleType<_FunctionType>& args)noexcept(nox::IsFunctionNoexceptValue<_FunctionType>);
				nox::FunctionResultType<_FunctionType>(* const call_ref)(CallableBase&&, const nox::FunctionArgsTupleType<_FunctionType>& args)noexcept(nox::IsFunctionNoexceptValue<_FunctionType>);
				void(* const copy)(const CallableBase&, std::span<nox::uint8>)noexcept(false);
				void(* const move)(CallableBase&&, std::span<nox::uint8>)noexcept(false);
				bool(* const equal)(const CallableBase&, const CallableBase&)noexcept;
				void(* const dispose)(CallableBase& self)noexcept(false);
				const nox::reflection::Type& (* const get_callable_type)()noexcept;
				const nox::reflection::Type& functor_type;
				const nox::reflection::Type& instance_type;
				const nox::NontypeId& member_pointer_id;
			};

			static inline Desc* kDescDummy = nullptr;

		public:
			inline constexpr ~CallableBase()noexcept = default;

			template<class... Args>
			inline constexpr nox::FunctionResultType<_FunctionType> operator()(Args&&... args)const& noexcept
			{
				if constexpr (std::is_void_v<nox::FunctionResultType<_FunctionType>>)
				{
					std::invoke(desc_.call, *this, std::make_tuple(std::forward<Args>(args)...));
				}
				else
				{
					return std::invoke(desc_.call, *this, std::make_tuple(std::forward<Args>(args)...));
				}
			}

			template<class... Args>
			inline constexpr nox::FunctionResultType<_FunctionType> operator()(Args&&... args)const&& noexcept
			{
				if constexpr (std::is_void_v<nox::FunctionResultType<_FunctionType>>)
				{
					desc_.call_ref(static_cast<CallableBase&&>(*this), std::forward<Args>(args)...);
				}
				else
				{
					return desc_.call_ref(static_cast<CallableBase&&>(*this), std::forward<Args>(args)...);
				}
			}

			inline constexpr void Copy(std::span<nox::uint8> dest)const
			{
				NOX_ASSERT(dest.size() >= this->GetSelfCallableSize(), U"コピー先のサイズが不足しています");
				desc_.copy(*this, dest);
			}

			inline constexpr void Move(std::span<nox::uint8> dest)
			{
				NOX_ASSERT(dest.size() >= this->GetSelfCallableSize(), U"コピー先のサイズが不足しています");
				desc_.move(std::move(*this), dest);
			}

			inline constexpr bool operator==(const CallableBase& other)const noexcept
			{
				return desc_.equal(*this, other);
			}

			inline constexpr const nox::reflection::Type& GetCallableTypeInfo()const noexcept
			{
				return desc_.get_callable_type();
			}

			inline constexpr const nox::reflection::Type& GetFunctorTypeInfo()const noexcept
			{
				return desc_.functor_type;
			}

			inline constexpr const nox::reflection::Type& GetInstanceTypeInfo()const noexcept
			{
				return desc_.instance_type;
			}

			inline constexpr size_t GetSelfCallableSize()const noexcept
			{
				return this->GetCallableTypeInfo().GetTypeSize();
			}

			inline constexpr const nox::NontypeId& GetMemberPointerId()const noexcept
			{
				return desc_.member_pointer_id;
			}


			inline constexpr void Dispose()noexcept(false)
			{
				desc_.dispose(*this);
			}

			static inline Desc* kInvalidDesc = nullptr;
		protected:
			inline constexpr explicit CallableBase(
				const Desc& desc
			)noexcept :
				desc_(desc)
			{
			}

		private:
			const Desc& desc_;
		};

		template<class _Functor>
		class Callable : public CallableBase
		{
		public:
			template<class _F> 
				requires(
				!std::is_lvalue_reference_v<_Functor>
					)
			inline constexpr explicit Callable(_F&& functor)noexcept(false) :
				CallableBase(kDesc),
				functor_(std::forward<_F>(functor))
			{

			}

			template<class _F>
				requires(
			std::is_lvalue_reference_v<_Functor>
				)
				inline constexpr explicit Callable(_F&& functor)noexcept(false) :
				CallableBase(kDesc),
				functor_(functor)
			{

			}

			inline constexpr bool operator==(const _Functor& functor)const noexcept
			{
				if constexpr (nox::concepts::EqualityComparable<_Functor, _Functor> == true)
				{
					return this->functor_ == functor;
				}
				else
				{
					return false;
				}
			}

		private:
			inline constexpr ~Callable()noexcept = default;

			inline static constexpr nox::FunctionResultType<_FunctionType> Call(const CallableBase& self, const nox::FunctionArgsTupleType<_FunctionType>& args)noexcept(false)
			{
				const Callable<_Functor>& self_impl = static_cast<const Callable<_Functor>&>(self);

				//	メンバクラスインスタンスを加工せず、そのまま呼び出せる場合
				if constexpr (std::is_void_v<nox::FunctionResultType<_FunctionType>>)
				{
					std::apply(self_impl.functor_, args);
					//std::invoke(self_impl.functor_, std::forward<Args>(args)...);
				}
				else
				{
					return std::apply(self_impl.functor_, args);
					//return std::invoke(self_impl.functor_, std::forward<Args>(args)...);
				}
			}

			template<class... Args>
			inline static constexpr nox::FunctionResultType<_FunctionType> CallRef(CallableBase&& self, Args&&... args)noexcept
			{
				Callable<_Functor>&& self_impl = static_cast<Callable<_Functor>&&>(self);

				if constexpr (std::is_void_v<nox::FunctionResultType<_FunctionType>>)
				{
					std::apply(self_impl.functor_, std::forward<Args>(args)...);
				}
				else
				{
					return std::apply(self_impl.functor_, std::forward<Args>(args)...);
				}
			}

			inline static constexpr void Copy(const CallableBase& self, std::span<nox::uint8> dest)noexcept
			{
				const Callable<_Functor>& impl = static_cast<const Callable<_Functor>&>(self);
				std::construct_at(reinterpret_cast<Callable<_Functor>*>(dest.data()), impl.functor_);
			}

			inline static constexpr void Move(CallableBase&& self, std::span<nox::uint8> dest)noexcept
			{
				Callable<_Functor>&& impl = static_cast<Callable<_Functor>&&>(self);
				std::construct_at(reinterpret_cast<Callable<_Functor>*>(dest.data()), std::move(impl.functor_));
			}

			inline static constexpr bool EqualFunc(const CallableBase& lhs, const CallableBase& rhs)noexcept
			{
				const Callable<_Functor>& lhs_impl = static_cast<const Callable<_Functor>&>(lhs);
				const Callable<_Functor>& rhs_impl = static_cast<const Callable<_Functor>&>(rhs);

				if constexpr (nox::concepts::EqualityComparable<_Functor, _Functor> == true)
				{
					return lhs_impl.functor_ == rhs_impl.functor_;
				}
				else
				{
					return false;
				}
			}

			inline static constexpr void Dispose(CallableBase& self)noexcept(false)
			{
				static_cast<Callable<_Functor>&>(self).~Callable();
			}

			static constexpr CallableBase::Desc kDesc
			{
				.call = &Call,
				.call_ref = &CallRef,
				.copy = &Copy,
				.move = &Move,
				.equal = &EqualFunc,
				.dispose = &Dispose,
				.get_callable_type = +[]()constexpr noexcept->const nox::reflection::Type& {return nox::reflection::Typeof<Callable<_Functor>>(); },
				.functor_type = nox::reflection::GetInvalidType(),
				.instance_type = nox::reflection::GetInvalidType(),
				.member_pointer_id = nox::GetInvalidNontypeId()
			};

		private:
			_Functor functor_;


		};

		template<class _InstanceType, auto _MemberPointerValue>
		class CallableMember : public CallableBase
		{
		public:
			template<class T>
			inline constexpr explicit CallableMember(T&& instance)noexcept :
				CallableBase(kDesc),
				instance_(std::forward<T>(instance))
			{
			}

			inline constexpr bool operator==(const _InstanceType& instance)const noexcept
			{
				if constexpr (nox::concepts::EqualityComparable<_InstanceType, _InstanceType> == true)
				{
					return instance_ == instance;
				}
				else
				{
					return false;
				}
			}

		private:
			inline static constexpr nox::FunctionResultType<_FunctionType> Call(const CallableBase& self, const nox::FunctionArgsTupleType<_FunctionType>& args)noexcept
			{
				const CallableMember<_InstanceType, _MemberPointerValue>& self_impl = static_cast<const CallableMember<_InstanceType, _MemberPointerValue>&>(self);

				//	メンバクラスインスタンスを加工せず、そのまま呼び出せる場合
				if constexpr (std::is_void_v<nox::FunctionResultType<_FunctionType>>)
				{
					std::apply(_MemberPointerValue, std::tuple_cat(std::make_tuple(self_impl.instance_), args));
				}
				else
				{
					return std::apply(_MemberPointerValue, std::tuple_cat(std::make_tuple(self_impl.instance_), args));
				}
			}

			inline static constexpr nox::FunctionResultType<_FunctionType> CallRef(CallableBase&& self, const nox::FunctionArgsTupleType<_FunctionType>& args)noexcept
			{
				CallableMember<_InstanceType, _MemberPointerValue>&& self_impl = static_cast<CallableMember<_InstanceType, _MemberPointerValue>&&>(self);

				if constexpr (std::is_void_v<nox::FunctionResultType<_FunctionType>>)
				{
					std::apply(_MemberPointerValue, std::tuple_cat(std::make_tuple(self_impl.instance_), args));
				}
				else
				{
					return std::apply(_MemberPointerValue, std::tuple_cat(std::make_tuple(self_impl.instance_), args));
				}
			}

			inline static constexpr void Copy(const CallableBase& self, std::span<nox::uint8> dest)noexcept
			{
				const CallableMember<_InstanceType, _MemberPointerValue>& impl = static_cast<const CallableMember<_InstanceType, _MemberPointerValue>&>(self);
				std::construct_at(reinterpret_cast<CallableMember<_InstanceType, _MemberPointerValue>*>(dest.data()), impl.instance_);
			}

			inline static constexpr void Move(CallableBase&& self, std::span<nox::uint8> dest)noexcept
			{
				CallableMember<_InstanceType, _MemberPointerValue>&& impl = static_cast<CallableMember<_InstanceType, _MemberPointerValue>&&>(self);
				std::construct_at(reinterpret_cast<CallableMember<_InstanceType, _MemberPointerValue>*>(dest.data()), std::move(impl.instance_));
			}

			inline static constexpr bool EqualFunc(const CallableBase& lhs, const CallableBase& rhs)noexcept
			{
				const CallableMember<_InstanceType, _MemberPointerValue>& lhs_impl = static_cast<const CallableMember<_InstanceType, _MemberPointerValue>&>(lhs);
				const CallableMember<_InstanceType, _MemberPointerValue>& rhs_impl = static_cast<const CallableMember<_InstanceType, _MemberPointerValue>&>(rhs);

				if constexpr (nox::concepts::EqualityComparable<_InstanceType, _InstanceType> == true)
				{
					return lhs_impl.instance_ == rhs_impl.instance_;
				}
				else
				{
					return false;
				}
			}

			static inline constexpr void Dispose(CallableBase& self)noexcept(false)
			{
				static_cast<CallableMember<_InstanceType, _MemberPointerValue>&>(self).~CallableMember();
			}
		
			static constexpr CallableBase::Desc kDesc{
				.call = &Call,
				.call_ref = &CallRef,
				.copy = &Copy,
				.move = &Move,
				.equal = &EqualFunc,
				.dispose = &Dispose,
				.get_callable_type = +[]()constexpr noexcept->const nox::reflection::Type& {return nox::reflection::Typeof<CallableMember<_InstanceType, _MemberPointerValue>>(); },
				.functor_type = std::ref(nox::reflection::Typeof<_InstanceType>()),
				.instance_type = nox::reflection::Typeof<_InstanceType>(),
				.member_pointer_id = nox::GetNontypeId<_MemberPointerValue>()
			};
		private:
			_InstanceType instance_;
		};

		class CallableBad : public CallableBase
		{
		public:
			inline constexpr CallableBad()noexcept :
				CallableBase(kDesc)
			{
			}

		private:
			inline static constexpr nox::FunctionResultType<_FunctionType> Call(const CallableBase&, const nox::FunctionArgsTupleType<_FunctionType>& )noexcept
			{
				NOX_ASSERT(false, U"Delegate Bad Call");

				if constexpr (std::is_void_v<nox::FunctionResultType<_FunctionType>>)
				{
					return;
				}
				else
				{
					nox::FunctionResultType<_FunctionType>(*dummy)() = nullptr;
					return (*dummy)();
				}
			}

			inline static constexpr nox::FunctionResultType<_FunctionType> CallRef(CallableBase&&, const nox::FunctionArgsTupleType<_FunctionType>&)noexcept
			{
				NOX_ASSERT(false, U"Delegate Bad Call");

				if constexpr (std::is_void_v<nox::FunctionResultType<_FunctionType>>)
				{
					return;
				}
				else
				{
					nox::FunctionResultType<_FunctionType>(*dummy)() = nullptr;
					return (*dummy)();
				}
			}

			inline static constexpr void Copy(const CallableBase&, std::span<nox::uint8>)noexcept
			{
				NOX_ASSERT(false, U"Delegate Bad Call");
			}

			inline static constexpr void Move(CallableBase&&, std::span<nox::uint8>)noexcept
			{
				NOX_ASSERT(false, U"Delegate Bad Call");
			}

			inline static constexpr bool EqualFunc(const CallableBase&, const CallableBase&)noexcept
			{
				return true;
			}

			static inline constexpr void Dispose(CallableBase& self)noexcept(false)
			{
				static_cast<CallableBad*>(&self)->~CallableBad();
			}

			static constexpr CallableBase::Desc kDesc{
				.call = &Call,
				.call_ref = &CallRef,
				.copy = &Copy,
				.move = &Move,
				.equal = &EqualFunc,
				.dispose = &Dispose,
				.get_callable_type = +[]()constexpr noexcept->const nox::reflection::Type& {return nox::reflection::Typeof<CallableBad>(); },
				.functor_type = nox::reflection::Typeof<void>(),
				.instance_type = nox::reflection::GetInvalidType(),
				.member_pointer_id = nox::GetInvalidNontypeId()
			};
		};
#pragma endregion

#pragma region requires
		/// @brief 関数ポインタの制約
		template<class>
		static constexpr bool kDelegateRequiresFunctionPointer = false;

		template<class _Functor> requires(
			nox::IsStaticFunctionPointerValue<_Functor> &&
			(std::is_function_v<_FunctionType> || nox::IsStaticFunctionPointerValue<_FunctionType>) &&

			// is noexcept
			(!nox::IsFunctionNoexceptValue<_FunctionType> || nox::IsFunctionNoexceptValue<_Functor>) &&

			//	is invocable
			std::is_same_v<nox::FunctionResultType<_FunctionType>, nox::InvokeResultTypeWithTupleLike<_Functor, nox::FunctionArgsTupleType<_FunctionType>>>

			)
			static constexpr bool kDelegateRequiresFunctionPointer<_Functor> = true;

		template<class,size_t, size_t>
		static constexpr bool kDelegateRequiresFunctionPointerImpl = false;

		template<class _Functor, size_t size, size_t align> requires(
			kDelegateRequiresFunctionPointer<_Functor> &&

			// size
			(size >= sizeof(typename nox::DelegateBase<_FunctionType>::template Callable<_Functor>)) &&

			//	alignment
			(align >= alignof(typename nox::DelegateBase<_FunctionType>::template Callable<_Functor>))

			)
		static constexpr bool kDelegateRequiresFunctionPointerImpl<_Functor, size, align> = true;

		/// @brief 関数オブジェクトの制約
		template<class>
		static constexpr bool kDelegateRequiresFunctionObject = false;
		template<class _Functor> requires(
			nox::IsFunctionObjectWithTupleLikeValue<_Functor, nox::FunctionArgsTupleType<_FunctionType>> &&

			// is not base of DelegateBase
			!std::is_base_of_v<DelegateBase<_FunctionType>, std::decay_t<_Functor>>&&

			std::is_function_v<_FunctionType> &&

			// is const
			(!nox::IsFunctionConstValue<_FunctionType> || nox::IsFunctionObjectConstWithTupleLikeValue<_Functor, nox::FunctionArgsTupleType<_FunctionType>>) &&

			// is noexcept
			(!nox::IsFunctionNoexceptValue<_FunctionType> || nox::IsFunctionObjectNoexceptWithTupleLikeValue<_Functor, nox::FunctionArgsTupleType<_FunctionType>>) &&

			//	is invocable
			std::is_same_v<nox::FunctionResultType<_FunctionType>, nox::FunctionObjectResultTypeWithTupleLike<_Functor, nox::FunctionArgsTupleType<_FunctionType>>>
			)
			static constexpr bool kDelegateRequiresFunctionObject<_Functor> = true;

		template<class, size_t, size_t>
		static constexpr bool kDelegateRequiresFunctionObjectImpl = false;

		template<class _Functor, size_t size, size_t align> requires(
			kDelegateRequiresFunctionObject<_Functor> &&
			(size >= sizeof(Callable<_Functor>) && align >= alignof(Callable<_Functor>))
			)
			static constexpr bool kDelegateRequiresFunctionObjectImpl<_Functor, size, align> = true;

		
		/// @brief メンバ関数ポインタの制約
		template<auto, class>
		static constexpr bool kDelegateRequiresMemberFunctionPointer = false;
		template<auto _MemberPointerValue, class _Instance>
			requires(
			//	is member function pointer
			std::is_member_function_pointer_v<decltype(_MemberPointerValue)> &&
			(std::is_function_v<_FunctionType> || std::is_member_function_pointer_v<_FunctionType>) &&
			
			//	is const
			(!nox::IsFunctionConstValue<_FunctionType> || nox::IsFunctionConstValue<decltype(_MemberPointerValue)>) &&

			//	is noexcept
			(!nox::IsFunctionNoexceptValue<_FunctionType> || nox::IsFunctionNoexceptValue<decltype(_MemberPointerValue)>) &&

			//	is invocable
			std::is_same_v<nox::FunctionResultType<_FunctionType>, nox::InvokeResultTypeWithTupleLike<decltype(_MemberPointerValue), nox::TupleCatType<_Instance, nox::FunctionArgsTupleType<_FunctionType>>>>
			)
			static constexpr bool kDelegateRequiresMemberFunctionPointer< _MemberPointerValue, _Instance> = true;

		template<auto, class,size_t, size_t>
		static constexpr bool kDelegateRequiresMemberFunctionPointerImpl = false;

		template<auto _MemberPointerValue, class _Instance, size_t size, size_t align> requires(
			kDelegateRequiresMemberFunctionPointer<_MemberPointerValue, _Instance> &&

			// size
			(size >= sizeof(typename DelegateBase<_FunctionType>::template CallableMember<_Instance, _MemberPointerValue>)) &&

			//	alignment
			(align >= alignof(typename DelegateBase<_FunctionType>::template CallableMember<_Instance, _MemberPointerValue>))

			)
		static constexpr bool kDelegateRequiresMemberFunctionPointerImpl<_MemberPointerValue, _Instance, size, align> = true;

		/// @brief メンバオブジェクトポインタの制約
		template<auto, class>
		static constexpr bool kDelegateRequiresMemberObjectPointer = false;
		template<auto _MemberPointerValue, class _Instance>
			requires(
			//	is member object pointer
			std::is_member_object_pointer_v<decltype(_MemberPointerValue)> &&
			(std::is_function_v<_FunctionType> || std::is_member_object_pointer_v<_FunctionType>) &&

		
			//	is invocable
			std::is_same_v<nox::FunctionResultType<_FunctionType>, nox::InvokeResultTypeWithTupleLike<decltype(_MemberPointerValue), nox::TupleCatType<_Instance, nox::FunctionArgsTupleType<_FunctionType>>>>
			)
			static constexpr bool kDelegateRequiresMemberObjectPointer< _MemberPointerValue, _Instance> = true;

		template<auto, class, size_t, size_t>
		static constexpr bool kDelegateRequiresMemberObjectPointerImpl = false;

		template<auto _MemberPointerValue, class _Instance, size_t size, size_t align> requires(
			kDelegateRequiresMemberFunctionPointer<_MemberPointerValue, _Instance> &&

			// size
			(size >= sizeof(typename DelegateBase<_FunctionType>::template CallableMember<_Instance, _MemberPointerValue>)) &&

			//	alignment
			(align >= alignof(typename DelegateBase<_FunctionType>::template CallableMember<_Instance, _MemberPointerValue>))

			)
			static constexpr bool kDelegateRequiresMemberObjectPointerImpl<_MemberPointerValue, _Instance, size, align> = true;


		/*
		* requires(
			//	is member function pointer
			std::is_member_object_pointer_v<decltype(_MemberPointerValue)> &&
			(std::is_function_v<_FunctionType> || std::is_member_object_pointer_v<_FunctionType>) &&

			// size
			(_Size >= sizeof(typename DelegateBase<_FunctionType>::template CallableMember<_Instance, _MemberPointerValue>)) &&

			//	alignment
			(_Alignment >= alignof(typename DelegateBase<_FunctionType>::template CallableMember<_Instance, _MemberPointerValue>)) &&

			//	is invocable
			std::is_same_v<nox::FunctionResultType<_FunctionType>, nox::InvokeResultTypeWithTupleLike<decltype(_MemberPointerValue), nox::TupleCatType<_Instance, nox::FunctionArgsTupleType<_FunctionType>>>>
				)
		*/

#pragma endregion


	public:
		static constexpr size_t kMinSize = sizeof(CallableBase);

		template<class... Args> requires(std::is_invocable_v<_FunctionType, Args...> && !nox::IsFunctionRValueReference<_FunctionType>)
		inline constexpr nox::FunctionResultType<_FunctionType> operator()(Args&&... args)const& noexcept(nox::IsFunctionNoexceptValue<_FunctionType>)
		{
			if constexpr (std::is_void_v<nox::FunctionResultType<_FunctionType>>)
			{
				this->GetCallable()(std::forward<Args>(args)...);
			}
			else
			{
				return this->GetCallable()(std::forward<Args>(args)...);
			}
		}

		template<class... Args> requires(std::is_invocable_v<_FunctionType, Args...> && nox::IsFunctionRValueReference<_FunctionType>)
			inline constexpr nox::FunctionResultType<_FunctionType> operator()(Args&&... args)const&& noexcept(nox::IsFunctionNoexceptValue<_FunctionType>)
		{
			if constexpr (std::is_void_v<nox::FunctionResultType<_FunctionType>>)
			{
				static_cast<CallableBase&&>(this->GetCallable())(std::forward<Args>(args)...);
			}
			else
			{
				return static_cast<CallableBase&&>(this->GetCallable())(std::forward<Args>(args)...);
			}
		}

		inline constexpr void operator=(std::nullptr_t)noexcept
		{
			if (*this != nullptr)
			{
				this->GetCallable().Dispose();
				std::destroy_at(reinterpret_cast<CallableBase*>(this));
				std::construct_at(reinterpret_cast<CallableBad*>(this));
			}
		}

		[[nodiscard]] inline constexpr bool Equal(std::nullptr_t)const noexcept
		{
			return this->GetCallable().GetCallableTypeInfo() == nox::reflection::Typeof<CallableBad>();
		}

		template<class _Functor>
			requires(
				kDelegateRequiresFunctionPointer<_Functor> ||
				kDelegateRequiresFunctionObject<_Functor>
			)
		
		inline constexpr bool Equal(_Functor&& functor)const noexcept
		{
			const nox::reflection::Type& self_functor_type = this->GetCallable().GetFunctorTypeInfo();
			if (self_functor_type == nox::reflection::Typeof<_Functor>())
			{
				return static_cast<const Callable<_Functor>&>(this->GetCallable()).operator==(std::forward<_Functor>(functor));
			}

			return false;
		}

		template<auto _MemberPointerValue, class _InstanceType>
		inline constexpr bool Equal(std::pair<nox::NontypeTag<_MemberPointerValue>, _InstanceType>&& args)const noexcept
		{
			const nox::NontypeId& member_pointer_id = this->GetCallable().GetMemberPointerId();
			const nox::reflection::Type& self_instance_type = this->GetCallable().GetInstanceTypeInfo();
			
			if (member_pointer_id == nox::GetNontypeId<_MemberPointerValue>())
			{
				if (self_instance_type == nox::reflection::Typeof<_InstanceType>())
				{
					return static_cast<const CallableMember<_InstanceType, _MemberPointerValue>&>(this->GetCallable()).operator==(args.second);
				}
			}
			return false;
		}
	protected:
		inline constexpr DelegateBase()noexcept = default;
		inline constexpr ~DelegateBase()noexcept = default;

		[[nodiscard]] inline constexpr const CallableBase& GetCallable()const noexcept
		{
			return *static_cast<const CallableBase*>(static_cast<const void*>(this));
		}

		[[nodiscard]] inline constexpr CallableBase& GetCallable() noexcept
		{
			return *static_cast<CallableBase*>(static_cast< void*>(this));
		}

	private:

	};

	template<class _FunctionType, class U> requires(
		std::is_same_v<bool, decltype(std::declval< nox::DelegateBase<_FunctionType>>().Equal(std::declval<U>()))>
		)
	inline constexpr bool operator==(const nox::DelegateBase<_FunctionType>& a, U&& b)noexcept
	{
		return a.Equal(std::forward<U>(b));
	}

	template<class _FunctionType, size_t _Size = 32, size_t _Alignment = alignof(std::max_align_t)>
	requires(
		_Size >= DelegateBase<_FunctionType>::kMinSize
		)
	class Delegate : public DelegateBase<_FunctionType>
	{
	public:
		static constexpr size_t Size = _Size;
		static constexpr size_t Alignment = _Alignment;

	public:
		using DelegateBase<_FunctionType>::DelegateBase;
		using DelegateBase<_FunctionType>::operator=;

		inline constexpr Delegate(std::nullptr_t)noexcept
		{
			std::construct_at(reinterpret_cast<typename DelegateBase<_FunctionType>::CallableBad*>(this->storage_.data()));
		}

		inline constexpr Delegate(const Delegate& other)noexcept(false)
		{
			if (other == nullptr)
			{
				std::construct_at(reinterpret_cast<typename DelegateBase<_FunctionType>::CallableBad*>(this->storage_.data()));
			}
			else
			{
				other.GetCallable().Copy(this->storage_);
			}
		}

		inline constexpr Delegate(Delegate&& other)noexcept(false)
		{
			if (other == nullptr)
			{
				std::construct_at(reinterpret_cast<typename DelegateBase<_FunctionType>::CallableBad*>(this->storage_.data()));
			}
			else
			{
				other.GetCallable().Move(this->storage_);
			}
		}

		template<size_t size, size_t align>
			requires(
			_Size >= size &&
			_Alignment >= align
			)
		inline constexpr Delegate(const Delegate<_FunctionType, size, align>& other)noexcept(false)
		{
			if (other == nullptr)
			{
				std::construct_at(reinterpret_cast<typename DelegateBase<_FunctionType>::CallableBad*>(this->storage_.data()));
			}
			else
			{
				other.GetCallable().Copy(this->storage_);
			}
		}

		template<size_t size, size_t align>
			requires(
			_Size >= size &&
			_Alignment >= align
			)
			inline constexpr Delegate(Delegate<_FunctionType, size, align>&& other)noexcept(false)
		{
			if (other == nullptr)
			{
				std::construct_at(reinterpret_cast<typename DelegateBase<_FunctionType>::CallableBad*>(this->storage_.data()));
			}
			else
			{
				other.GetCallable().Move(this->storage_);
			}
		}

		inline constexpr ~Delegate()noexcept(false)
		{
			if (*this != nullptr)
			{
				this->GetCallable().Dispose();
				std::destroy_at(reinterpret_cast<typename DelegateBase<_FunctionType>::CallableBase*>(this));
			}
		}

		/// @brief 関数ポインタ
		template<class _Functor>requires(
			nox::DelegateBase<_FunctionType>::template kDelegateRequiresFunctionPointerImpl<_Functor, _Size, _Alignment>
			)
		inline constexpr Delegate(_Functor&& functor)noexcept
		{
			this->template BindImplFast<typename nox::DelegateBase<_FunctionType>::template Callable<_Functor>>(std::forward<_Functor>(functor));
		}

		/// @brief 関数オブジェクト
		template<class _Functor> requires(
			nox::DelegateBase<_FunctionType>::template kDelegateRequiresFunctionObjectImpl<_Functor, _Size, _Alignment>
			)
			inline constexpr Delegate(_Functor&& functor)noexcept
		{
			this->template BindImplFast<typename nox::DelegateBase<_FunctionType>::template Callable<_Functor>>(std::forward<_Functor>(functor));
		}

		/// @brief メンバ関数ポインタ
		template<auto _MemberPointerValue, class _Instance>	requires(
			nox::DelegateBase<_FunctionType>::template kDelegateRequiresMemberFunctionPointerImpl<_MemberPointerValue, _Instance, _Size, _Alignment>
			)
		inline constexpr Delegate(nox::NontypeTag<_MemberPointerValue>, _Instance&& instance)noexcept
		{
			this->template BindImplFast<typename nox::DelegateBase<_FunctionType>::template CallableMember<_Instance, _MemberPointerValue>>(std::forward<_Instance>(instance));
		}

		/// @brief メンバ変数ポインタ
		template<auto _MemberPointerValue, class _Instance>
			requires(
			//	is member function pointer
			std::is_member_object_pointer_v<decltype(_MemberPointerValue)> &&
			(std::is_function_v<_FunctionType> || std::is_member_object_pointer_v<_FunctionType>) &&

			// size
			(_Size >= sizeof(typename DelegateBase<_FunctionType>::template CallableMember<_Instance, _MemberPointerValue>)) &&

			//	alignment
			(_Alignment >= alignof(typename DelegateBase<_FunctionType>::template CallableMember<_Instance, _MemberPointerValue>)) &&

			//	is invocable
			std::is_same_v<nox::FunctionResultType<_FunctionType>, nox::InvokeResultTypeWithTupleLike<decltype(_MemberPointerValue), nox::TupleCatType<_Instance, nox::FunctionArgsTupleType<_FunctionType>>>>
				)
			inline constexpr Delegate(nox::NontypeTag<_MemberPointerValue>, _Instance&& instance)noexcept
		{
			this->template BindImplFast<typename nox::DelegateBase<_FunctionType>::template CallableMember<_Instance, _MemberPointerValue>>(std::forward<_Instance>(instance));
		}

		template<auto _MemberPointerValue, class _Instance>
			requires(
				std::is_constructible_v<Delegate<_FunctionType, _Size, _Alignment>, nox::NontypeTag<_MemberPointerValue>, _Instance>
			)
		inline constexpr Delegate(std::pair<nox::NontypeTag<_MemberPointerValue>, _Instance>&& args)noexcept:
			Delegate(nox::Nontype<_MemberPointerValue>, std::forward<_Instance>(args.second))
		{
			
		}

		inline constexpr Delegate& operator=(const Delegate& other)noexcept(false)
		{
			if (other == nullptr)
			{
				*this = nullptr;
			}
			else
			{
				other.GetCallable().Copy(this->storage_);
			}
			return *this;
		}

		inline constexpr Delegate& operator=(Delegate&& other)noexcept(false)
		{
			if (other == nullptr)
			{
				*this = nullptr;
			}
			else
			{
				other.GetCallable().Move(this->storage_);
			}
			return *this;
		}

		template<size_t size, size_t align>
			requires(
		_Size >= size &&
			_Alignment >= align
			)
			inline constexpr Delegate& operator=(const Delegate<_FunctionType, size, align>& other)noexcept
		{
			if (other == nullptr)
			{
				*this = nullptr;
			}
			else
			{
				other.GetCallable().Copy(this->storage_);
			}
			return *this;
		}

		template<size_t size, size_t align>
			requires(
		_Size >= size &&
			_Alignment >= align
			)
			inline constexpr Delegate& operator=(Delegate<_FunctionType, size, align>&& other)noexcept
		{
			if (other == nullptr)
			{
				*this = nullptr;
			}
			else
			{
				other.GetCallable().Move(this->storage_);
			}
			return *this;
		}

		template<class _Functor> 
			requires(
			std::is_constructible_v<Delegate<_FunctionType, _Size, _Alignment>, _Functor> &&
			!std::is_base_of_v<DelegateBase<_FunctionType>, std::decay_t<_Functor>>
			)
		inline constexpr Delegate& operator=(_Functor&& functor)noexcept
		{
			this->template BindImpl<typename nox::DelegateBase<_FunctionType>::template Callable<_Functor>>(std::forward<_Functor>(functor));
			return *this;
		}

		template<auto _MemberPointerValue, class _Instance>
		requires(
			std::is_constructible_v<Delegate<_FunctionType, _Size, _Alignment>, nox::NontypeTag<_MemberPointerValue>, _Instance>
			)
		inline constexpr Delegate& operator=(std::pair<nox::NontypeTag<_MemberPointerValue>, _Instance>&& args)noexcept
		{
			this->template BindImpl<typename nox::DelegateBase<_FunctionType>::template CallableMember<_Instance, _MemberPointerValue>>(std::forward<_Instance>(args.second));
			return *this;
		}

		inline constexpr void Swap(Delegate<_FunctionType, _Size, _Alignment>& other)noexcept
		{
			std::swap(storage_, other.storage_);
		}
	private:
		inline constexpr Delegate()noexcept
		{
		}

		template<std::derived_from<typename nox::DelegateBase<_FunctionType>::CallableBase> T, class... Args>
		inline constexpr void BindImpl(Args&&... args)noexcept
		{
			*this = nullptr;
			std::construct_at(reinterpret_cast<T*>(storage_.data()), std::forward<Args>(args)...);
		}

		template<std::derived_from<typename nox::DelegateBase<_FunctionType>::CallableBase> T, class... Args>
		inline constexpr void BindImplFast(Args&&... args)noexcept
		{
			std::construct_at(reinterpret_cast<T*>(storage_.data()), std::forward<Args>(args)...);
		}
	private:
		alignas(_Alignment) std::array<std::uint8_t, _Size> storage_;
	};

	template<class _FunctionType, size_t _Size = 32, size_t _Alignment = alignof(std::max_align_t)>
	class MoveOnlyDelegate : public nox::Delegate<_FunctionType, _Size, _Alignment>
	{
	public:
		using nox::Delegate<_FunctionType, _Size, _Alignment>::Delegate;
		using nox::Delegate<_FunctionType, _Size, _Alignment>::operator=;

		inline constexpr MoveOnlyDelegate(const MoveOnlyDelegate&)noexcept = delete;

		template<class _F>
		inline constexpr MoveOnlyDelegate(const _F&)noexcept = delete;

		inline constexpr MoveOnlyDelegate& operator=(const MoveOnlyDelegate&)noexcept = delete;

		template<class _F>
		inline constexpr MoveOnlyDelegate& operator=(const _F&)noexcept = delete;
	};

	/// @brief DelegateRef	対象の所有権を管理しない軽量ラッパー
	/// @tparam _FunctionType 呼び出しオブジェクトの型
	template<class _FunctionType>
	class DelegateRef
	{
		/// @brief		"_FunctionType"のnoexcept性
		/// @details	何故か"IsFunctionNoexceptValue"を直接使うとエラーになるため、定数定義を使用
		static constexpr bool IsNoexcept = nox::IsFunctionNoexceptValue<_FunctionType>;
		
	public:
		using FunctionType = _FunctionType;

	public:
		
		inline constexpr DelegateRef(const DelegateRef& rhs)noexcept :
			object_(rhs.object_),
			call_(rhs.call_)
		{
		}

		inline constexpr DelegateRef(DelegateRef&& rhs)noexcept :
			object_(rhs.object_),
			call_(rhs.call_)
		{
			rhs.object_ = nullptr;
			rhs.call_ = &BadCall;
		}

		inline constexpr DelegateRef(std::nullptr_t)noexcept :
			DelegateRef() {
		}

		/// @brief 関数ポインタ
		template<class _F> requires(
			// functor type
			nox::IsStaticFunctionPointerValue<_F> &&
			(std::is_function_v<_FunctionType> || nox::IsStaticFunctionPointerValue<_F>) &&

			//	is noexcept
			(!nox::IsFunctionNoexceptValue<_FunctionType> || nox::IsFunctionNoexceptValue<_F>) &&

			//	invocable
			nox::IsInvocableWithTupleLikeValue<_F, nox::FunctionArgsTupleType<_FunctionType>> 
			)
		inline constexpr DelegateRef(_F&& functor)noexcept :
			object_(const_cast<void*>(static_cast<const void*>(nox::util::AddressOfLvalueReference(functor)))),
			call_(&Call<_F>)
		{
		}

		/// @brief 関数オブジェクト
		template<class _F> 
			requires(
			// functor type
			nox::IsFunctionObjectWithTupleLikeValue<_F, nox::FunctionArgsTupleType<_FunctionType>> &&
			!std::is_same_v<std::decay_t<_F>, DelegateRef<_FunctionType>>&&
			std::is_function_v<_FunctionType> &&

			//	ignore delegate ref
			!std::is_same_v<std::decay_t<_F>, DelegateRef<_FunctionType>> &&

			//	is const
			(!nox::IsFunctionConstValue<_FunctionType> || nox::IsFunctionConstValue<_F>) &&

			//	is noexcept
			(!nox::IsFunctionNoexceptValue<_FunctionType> || nox::IsFunctionNoexceptValue<_F>) &&

			// result type
			std::is_same_v<nox::FunctionResultType<_FunctionType>, nox::FunctionObjectResultTypeWithTupleLike<_F, nox::FunctionArgsTupleType<_FunctionType>>>
			)
		inline constexpr DelegateRef(_F&& functor)noexcept :
			object_(const_cast<void*>(static_cast<const void*>(nox::util::AddressOfLvalueReference(functor)))),
			call_(&Call<_F>)
		{
		}

		/// @brief メンバ関数ポインタ
		template<auto _MemberPointerValue, class _InstanceType>
		requires(
			//	is member function pointer
			std::is_member_function_pointer_v<decltype(_MemberPointerValue)> &&
			(std::is_function_v<_FunctionType> || std::is_member_function_pointer_v<_FunctionType>) &&

			//	is const
			(!nox::IsFunctionConstValue<_FunctionType> || nox::IsFunctionConstValue<decltype(_MemberPointerValue)>) &&

			//	is noexcept
			(!nox::IsFunctionNoexceptValue<_FunctionType> || nox::IsFunctionNoexceptValue<decltype(_MemberPointerValue)>) &&

			//	is invocable
			std::is_same_v<nox::FunctionResultType<_FunctionType>, nox::InvokeResultTypeWithTupleLike<decltype(_MemberPointerValue), nox::TupleCatType<_InstanceType, nox::FunctionArgsTupleType<_FunctionType>>>>
			)
		inline constexpr DelegateRef(nox::NontypeTag<_MemberPointerValue>, _InstanceType&& instance)noexcept:
			object_(const_cast<void*>(static_cast<const void*>(nox::util::AddressOfLvalueReference(instance)))),
			call_(&MemberCall<_InstanceType, _MemberPointerValue>)
		{
		}

		/// @brief メンバ変数ポインタ
		template<auto _MemberPointerValue, class _InstanceType>
			requires(
				//	is member function pointer
				std::is_member_object_pointer_v<decltype(_MemberPointerValue)> &&
				(std::is_function_v<_FunctionType> || std::is_member_object_pointer_v<_FunctionType>) &&

				// result type
				std::is_same_v<nox::FunctionResultType<_FunctionType>, nox::ObjectPointerResultType<decltype(_MemberPointerValue)>>&&

					//	is invocable
					std::is_same_v<nox::FunctionResultType<_FunctionType>, nox::InvokeResultTypeWithTupleLike<decltype(_MemberPointerValue), nox::TupleCatType<_InstanceType, nox::FunctionArgsTupleType<_FunctionType>>>>
			)
			inline constexpr DelegateRef(nox::NontypeTag<_MemberPointerValue>, _InstanceType&& instance)noexcept :
			object_(const_cast<void*>(static_cast<const void*>(nox::util::AddressOfLvalueReference(instance)))),
			call_(&MemberCall<_InstanceType, _MemberPointerValue>)
		{
		}

		/// @brief メンバ関数ポインタ or メンバ関数ポインタ
		template<auto MemberPointerValue, class _InstanceType>
			requires(
				std::is_lvalue_reference_v<_InstanceType> || std::is_pointer_v<_InstanceType> &&
				std::is_constructible_v<DelegateRef<_FunctionType>, nox::NontypeTag<MemberPointerValue>, _InstanceType>
			)
			inline constexpr DelegateRef(std::pair<nox::NontypeTag<MemberPointerValue>, _InstanceType>&& args)noexcept :
			DelegateRef(nox::Nontype<MemberPointerValue>, std::forward<_InstanceType>(args.second))
		{
		}
	
		template<class... Args>
		inline constexpr nox::FunctionResultType<_FunctionType> operator()(Args&&... args)noexcept(nox::IsFunctionNoexceptValue<_FunctionType>)
		{
			return call_(object_, std::make_tuple(std::forward<Args>(args)...));
		}

		inline constexpr void Swap(DelegateRef& rhs)noexcept
		{
			std::swap(object_, rhs.object_);
			std::swap(call_, rhs.call_);
		}

		inline constexpr bool operator==(std::nullptr_t)const noexcept
		{
			return object_ != nullptr;
		}

		inline constexpr bool operator!=(const DelegateRef& other)const noexcept
		{
			return object_ == other.object_;
		}

		template<class _F>
		inline constexpr bool operator==(_F&& functor)const noexcept
		{
			const void*const object = static_cast<const void*>(nox::util::AddressOfLvalueReference(std::forward<_F>(functor)));
			return object_ == object;
		}

		/// @brief 関数ポインタ or 関数オブジェクト
		template<class _F>
			requires(
				std::is_constructible_v<DelegateRef<_FunctionType>, _F>
			)
		inline constexpr DelegateRef& operator=(_F&& functor)noexcept
		{
			object_ = const_cast<void*>(static_cast<const void*>(nox::util::AddressOfLvalueReference(functor)));
			call_ = &Call<_F>;

			return *this;
		}
	
		/// @brief メンバ関数ポインタ or メンバ関数オブジェクト
		template<auto MemberPointerValue, class _InstanceType> 
		requires(
			std::is_constructible_v<DelegateRef<_FunctionType>, nox::NontypeTag<MemberPointerValue>, _InstanceType>
			)
			inline constexpr DelegateRef& operator=(std::pair<nox::NontypeTag<MemberPointerValue>, _InstanceType>&& args)noexcept
		{
			object_ = const_cast<void*>(static_cast<const void*>(nox::util::AddressOfLvalueReference(args.second)));
			call_ = &MemberCall<_InstanceType, MemberPointerValue>;

			return *this;
		}

	private:
		inline constexpr DelegateRef()noexcept :
			object_(nullptr),
			call_(&BadCall)
		{
		}

	private:
		/// @brief		関数ポインタや関数オブジェクト、メンバ関数ポインタを呼び出すクラスのインスタンスを保持するポインタ
		///	@details	const性が消えているが、呼び出し時にconst性をチェックしている
		void* object_;

		/// @brief		呼び出し用関数ポインタ
		nox::FunctionResultType<_FunctionType>(*call_)(void*, nox::FunctionArgsTupleType<_FunctionType>&&)noexcept(nox::IsFunctionNoexceptValue<_FunctionType>);

		/// @brief 非メンバの呼び出し
		/// @tparam _Functor 
		/// @param functor 
		/// @param args 
		/// @return 
		template<class _Functor>
		inline static constexpr nox::FunctionResultType<_FunctionType> Call(void* functor, nox::FunctionArgsTupleType<_FunctionType>&& args)noexcept(IsNoexcept)
		{
			decltype(auto) functor_impl = *reinterpret_cast<std::conditional_t<std::is_pointer_v<std::decay_t<_Functor>>, _Functor, std::add_pointer_t<_Functor>>>(functor);

			if constexpr (std::is_void_v<nox::FunctionResultType<_FunctionType>>)
			{
				std::apply(functor_impl, std::forward<nox::FunctionArgsTupleType<_FunctionType>>(args));
			}
			else
			{
				return std::apply(functor_impl, std::forward<nox::FunctionArgsTupleType<_FunctionType>>(args));
			}
		}

		/// @brief メンバの呼び出し
		/// @tparam _InstanceType 
		/// @tparam _MemberFunctionPointer 
		/// @param instance 
		/// @param args 
		/// @return 
		template<class _InstanceType, auto _MemberFunctionPointer> requires(
			std::is_member_object_pointer_v<decltype(_MemberFunctionPointer)> == true ||
			std::is_member_function_pointer_v<decltype(_MemberFunctionPointer)> == true 
			)
		inline static constexpr nox::FunctionResultType<_FunctionType> MemberCall(void* instance, nox::FunctionArgsTupleType<_FunctionType>&& args)noexcept(IsNoexcept)
		{
			decltype(auto) instance_impl = *reinterpret_cast<std::conditional_t<std::is_pointer_v<_InstanceType>, _InstanceType, std::add_pointer_t<_InstanceType>>>(instance);
			decltype(auto) instance_addr = nox::util::TryToAddress(instance_impl);
			if constexpr (std::is_void_v<nox::FunctionResultType<_FunctionType>>)
			{
				std::apply(_MemberFunctionPointer, std::tuple_cat(std::make_tuple(instance_addr), std::forward<nox::FunctionArgsTupleType<_FunctionType>>(args)));
			}
			else
			{
				return std::apply(_MemberFunctionPointer, std::tuple_cat(std::make_tuple(instance_addr), std::forward<nox::FunctionArgsTupleType<_FunctionType>>(args)));
//				return std::apply(_MemberFunctionPointer, instance_addr, std::forward<nox::FunctionArgsTupleType<_FunctionType>>(args));
			}
		}

		/// @brief 無効な呼び出し
		inline static constexpr nox::FunctionResultType<_FunctionType> BadCall(void*, nox::FunctionArgsTupleType<_FunctionType>&&)noexcept(nox::IsFunctionNoexceptValue<_FunctionType>)
		{
			NOX_ASSERT(false, U"bad call");
			if constexpr (std::is_void_v<nox::FunctionResultType<_FunctionType>>)
			{
				return;
			}
			else
			{
				constexpr nox::FunctionResultType<_FunctionType>(*const dummy)() = nullptr;
				return (*dummy)();
			}
		}
	};

	template<class _FunctionType, size_t _Size = 32, size_t _Alignment = alignof(std::max_align_t)>
	class MulticastDelegate
	{
	public:
		using DelegateType = Delegate<_FunctionType, _Size, _Alignment>;

	public:
		inline constexpr MulticastDelegate()noexcept :
			delegate_list_{}
		{
		}

		inline constexpr MulticastDelegate(const MulticastDelegate& rhs)noexcept
		{
			delegate_list_ = rhs.delegate_list_;
		}
		inline constexpr MulticastDelegate(MulticastDelegate&& rhs)noexcept
		{
			delegate_list_ = std::move(rhs.delegate_list_);
		}
		
		inline constexpr MulticastDelegate(std::nullptr_t)noexcept:
			delegate_list_{}
		{
		}

		inline constexpr ~MulticastDelegate()noexcept
		{
		}

		inline constexpr MulticastDelegate& operator=(const MulticastDelegate& rhs)noexcept
		{
			delegate_list_ = rhs.delegate_list_;
			return *this;
		}
		inline constexpr MulticastDelegate& operator=(MulticastDelegate&& rhs)noexcept
		{
			delegate_list_ = std::move(rhs.delegate_list_);
			return *this;
		}
		inline constexpr MulticastDelegate& operator=(std::nullptr_t)noexcept
		{
			delegate_list_.clear();
			return *this;
		}

		inline constexpr MulticastDelegate& operator+= (const DelegateType& delegate)noexcept
		{
			delegate_list_.emplace_back(delegate);
			return *this;
		}

		inline constexpr MulticastDelegate& operator-= (const DelegateType & delegate)noexcept
		{
			std::erase(delegate_list_, delegate);
			return *this;
		}

		template<class _F>
		inline constexpr MulticastDelegate& operator+=(_F&& functor)noexcept
		{
			delegate_list_.emplace_back(std::forward<_F>(functor));
			return *this;
		}

		template<class _F>
		inline constexpr MulticastDelegate& operator-=(_F&& functor)noexcept
		{
			std::erase_if(delegate_list_, [&functor](const DelegateType& delegate)noexcept {
				return delegate == std::forward<_F>(functor);
				});
			return *this;
		}

	/*	template<auto _MemberPointerValue, class _F>
		inline constexpr MulticastDelegate& operator+=(std::pair<nox::NontypeTag<_MemberPointerValue>, _F>&& args)noexcept
		{
			delegate_list_.emplace_back(args);
			return *this;
		}*/

		template<class... Args>
		inline constexpr nox::FunctionResultType<_FunctionType> operator()(Args&&... args)const&noexcept(nox::IsFunctionNoexceptValue<_FunctionType>)
		{
			NOX_ASSERT(!delegate_list_.empty(), U"MulticastDelegate is empty");

			if constexpr (std::is_void_v<nox::FunctionResultType<_FunctionType>>)
			{
				for (const auto& delegate : delegate_list_)
				{
					delegate(std::forward<Args>(args)...);
				}
			}
			else
			{
				for (const auto& delegate : delegate_list_ | std::views::take(delegate_list_.size() - 1))
				{
					delegate(std::forward<Args>(args)...);
				}

				return delegate_list_.back()(std::forward<Args>(args)...);
			}
		}

		inline constexpr void Swap(MulticastDelegate& rhs)noexcept
		{
			delegate_list_.Swap(rhs.delegate_list_);
		}

		[[nodiscard]]
		inline constexpr size_t Size()const noexcept
		{
			return delegate_list_.size();
		}

		inline constexpr void Resize(size_t size)noexcept
		{
			size_t current_size = delegate_list_.size();
			delegate_list_.reserve(size);

			if (size > current_size)
			{
				std::uninitialized_fill(delegate_list_.begin() + current_size, delegate_list_.end(), DelegateType(nullptr));
			}
		}

		[[nodiscard]]
		inline constexpr bool operator==(std::nullptr_t)const noexcept
		{
			return delegate_list_.empty();
		}

	private:
		nox::Vector<DelegateType> delegate_list_;
	};
}