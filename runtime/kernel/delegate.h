///	@file	delegate.h
///	@brief	delegate
#pragma once
#include	<array>
#include	<concepts>
#include	<cstddef>
#include	<functional>
#include	<memory>
#include	<type_traits>
#include	<utility>

#include	"type_traits/function_signature.h"

namespace nox
{
	namespace detail
	{
		template<class _FunctionType, class _ArgsTuple>
		class IDelegateBase;

		template<class _FunctionType, class... _Args>
		class IDelegateBase<_FunctionType, std::tuple<_Args...>>
		{
		public:
			using FunctionType = _FunctionType;
			using ResultType = nox::FunctionResultType<_FunctionType>;
			using InvokeFunction = ResultType(*)(void*, _Args...) noexcept(nox::IsFunctionNoexceptValue<_FunctionType>);
			struct Operations
			{
				InvokeFunction invoke;
				void(*destroy)(void*) noexcept;
				void(*move)(void*, void*) noexcept;
				void(*copy)(void*, const void*) noexcept;
			};

			[[nodiscard]] constexpr bool IsValid() const noexcept { return operations_ != nullptr; }
			[[nodiscard]] constexpr explicit operator bool() const noexcept { return IsValid(); }

		protected:
			constexpr void Bind(const Operations* operations) noexcept { operations_ = operations; }
			constexpr void Reset() noexcept { operations_ = nullptr; }
			[[nodiscard]] constexpr const Operations* OperationTable() const noexcept { return operations_; }

			ResultType Invoke(_Args... args) const noexcept(nox::IsFunctionNoexceptValue<_FunctionType>)
			{
				return operations_->invoke(const_cast<IDelegateBase*>(this), std::forward<_Args>(args)...);
			}

			~IDelegateBase() = default;

		private:
			const Operations* operations_ = nullptr;
		};
	}

	template<nox::concepts::FunctionSignatureType _FunctionType>
	class IDelegate : public detail::IDelegateBase<_FunctionType, nox::FunctionArgsTupleType<_FunctionType>>
	{
		using Base = detail::IDelegateBase<_FunctionType, nox::FunctionArgsTupleType<_FunctionType>>;

	public:
		using typename Base::FunctionType;
		using typename Base::InvokeFunction;
		using typename Base::ResultType;
		using Base::IsValid;
		using Base::operator bool;

		constexpr IDelegate() noexcept = default;

		template<class... _Args>
		ResultType operator()(_Args&&... args) noexcept(nox::IsFunctionNoexceptValue<_FunctionType>)
			requires(!nox::IsFunctionConstValue<_FunctionType> && !nox::IsFunctionVolatileValue<_FunctionType> && !nox::IsFunctionLValueReference<_FunctionType> && !nox::IsFunctionRValueReference<_FunctionType>)
		{
			return Base::Invoke(std::forward<_Args>(args)...);
		}

		template<class... _Args>
		ResultType operator()(_Args&&... args) const noexcept(nox::IsFunctionNoexceptValue<_FunctionType>)
			requires(nox::IsFunctionConstValue<_FunctionType> && !nox::IsFunctionVolatileValue<_FunctionType> && !nox::IsFunctionLValueReference<_FunctionType> && !nox::IsFunctionRValueReference<_FunctionType>)
		{
			return Base::Invoke(std::forward<_Args>(args)...);
		}

		template<class... _Args>
		ResultType operator()(_Args&&... args) const & noexcept(nox::IsFunctionNoexceptValue<_FunctionType>)
			requires(nox::IsFunctionConstValue<_FunctionType> && !nox::IsFunctionVolatileValue<_FunctionType> && nox::IsFunctionLValueReference<_FunctionType>)
		{
			return Base::Invoke(std::forward<_Args>(args)...);
		}

		template<class... _Args>
		ResultType operator()(_Args&&... args) const && noexcept(nox::IsFunctionNoexceptValue<_FunctionType>)
			requires(nox::IsFunctionConstValue<_FunctionType> && !nox::IsFunctionVolatileValue<_FunctionType> && nox::IsFunctionRValueReference<_FunctionType>)
		{
			return Base::Invoke(std::forward<_Args>(args)...);
		}

		template<class... _Args>
		ResultType operator()(_Args&&... args) & noexcept(nox::IsFunctionNoexceptValue<_FunctionType>)
			requires(nox::IsFunctionLValueReference<_FunctionType>)
		{
			return Base::Invoke(std::forward<_Args>(args)...);
		}

		template<class... _Args>
		ResultType operator()(_Args&&... args) && noexcept(nox::IsFunctionNoexceptValue<_FunctionType>)
			requires(nox::IsFunctionRValueReference<_FunctionType>)
		{
			return Base::Invoke(std::forward<_Args>(args)...);
		}

	protected:
		using Base::Bind;
		using Base::Reset;
		~IDelegate() = default;
	};

	template<class _FunctionType, std::size_t _Size = 32, std::size_t _Alignment = alignof(std::max_align_t)>
	class MoveOnlyDelegate;

	template<class _Result, class... _Args, std::size_t _Size, std::size_t _Alignment>
	class MoveOnlyDelegate<_Result(_Args...), _Size, _Alignment> : public IDelegate<_Result(_Args...)>
	{
		using Base = IDelegate<_Result(_Args...)>;
		using Operations = typename Base::Operations;

	public:
		static_assert(_Size > 0);
		static_assert(_Alignment > 0);

		static constexpr std::size_t Size = _Size;
		static constexpr std::size_t Alignment = _Alignment;

		constexpr MoveOnlyDelegate() noexcept = default;

		template<class _Functor>
			requires(!std::same_as<std::remove_cvref_t<_Functor>, MoveOnlyDelegate> && sizeof(std::decay_t<_Functor>) <= _Size && alignof(std::decay_t<_Functor>) <= _Alignment && std::is_nothrow_move_constructible_v<std::decay_t<_Functor>> && std::is_nothrow_constructible_v<std::decay_t<_Functor>, _Functor&&> && std::is_invocable_r_v<_Result, std::decay_t<_Functor>&, _Args...>)
		constexpr MoveOnlyDelegate(_Functor&& functor) noexcept
		{
			Emplace<std::decay_t<_Functor>>(std::forward<_Functor>(functor));
		}

		MoveOnlyDelegate(const MoveOnlyDelegate&) = delete;
		MoveOnlyDelegate& operator=(const MoveOnlyDelegate&) = delete;

		constexpr MoveOnlyDelegate(MoveOnlyDelegate&& other) noexcept
		{
			MoveFrom(other);
		}

		constexpr MoveOnlyDelegate& operator=(MoveOnlyDelegate&& other) noexcept
		{
			if (this != std::addressof(other))
			{
				Reset();
				MoveFrom(other);
			}

			return *this;
		}

		template<class _Functor>
			requires(!std::same_as<std::remove_cvref_t<_Functor>, MoveOnlyDelegate> && sizeof(std::decay_t<_Functor>) <= _Size && alignof(std::decay_t<_Functor>) <= _Alignment && std::is_nothrow_move_constructible_v<std::decay_t<_Functor>> && std::is_nothrow_constructible_v<std::decay_t<_Functor>, _Functor&&> && std::is_invocable_r_v<_Result, std::decay_t<_Functor>&, _Args...>)
		constexpr MoveOnlyDelegate& operator=(_Functor&& functor) noexcept
		{
			Reset();
			Emplace<std::decay_t<_Functor>>(std::forward<_Functor>(functor));
			return *this;
		}

		constexpr ~MoveOnlyDelegate() noexcept
		{
			Reset();
		}

		template<class _Functor, class... _ConstructorArgs>
			requires(sizeof(_Functor) <= _Size && alignof(_Functor) <= _Alignment && std::is_nothrow_move_constructible_v<_Functor> && std::is_invocable_r_v<_Result, _Functor&, _Args...> && std::constructible_from<_Functor, _ConstructorArgs...> && std::is_nothrow_constructible_v<_Functor, _ConstructorArgs...>)
		constexpr void Emplace(_ConstructorArgs&&... args) noexcept
		{
			Reset();
			std::construct_at(As<_Functor>(), std::forward<_ConstructorArgs>(args)...);
			this->Bind(std::addressof(GetOperations<_Functor>()));
		}

		constexpr void Reset() noexcept
		{
			if (const Operations* operations = this->OperationTable())
			{
				operations->destroy(Storage());
			}

			Base::Reset();
		}

	private:
		template<class _Functor>
		static _Result Invoke(void* object, _Args... args)
		{
			auto* delegate = static_cast<MoveOnlyDelegate*>(static_cast<Base*>(object));
			_Functor& functor = *delegate->template As<_Functor>();
			if constexpr (std::is_void_v<_Result>)
			{
				std::invoke(functor, std::forward<_Args>(args)...);
			}
			else
			{
				return std::invoke(functor, std::forward<_Args>(args)...);
			}
		}

		template<class _Functor>
		static void Destroy(void* object) noexcept
		{
			std::destroy_at(std::launder(reinterpret_cast<_Functor*>(object)));
		}

		template<class _Functor>
		static void Move(void* destination, void* source) noexcept
		{
			_Functor* source_functor = std::launder(reinterpret_cast<_Functor*>(source));
			std::construct_at(std::launder(reinterpret_cast<_Functor*>(destination)), std::move(*source_functor));
			std::destroy_at(source_functor);
		}

		template<class _Functor>
		static const Operations& GetOperations() noexcept
		{
			static constexpr Operations operations{ &Invoke<_Functor>, &Destroy<_Functor>, &Move<_Functor>, nullptr };
			return operations;
		}

		constexpr void MoveFrom(MoveOnlyDelegate& other) noexcept
		{
			if (const Operations* operations = other.OperationTable(); operations == nullptr)
			{
				return;
			}

			const Operations* operations = other.OperationTable();
			operations->move(Storage(), other.Storage());
			this->Bind(operations);
			other.Unbind();
		}

		constexpr void Unbind() noexcept
		{
			Base::Reset();
		}

		constexpr void* Storage() noexcept
		{
			return storage_.data();
		}

		template<class _Functor>
		constexpr _Functor* As() noexcept
		{
			return std::launder(reinterpret_cast<_Functor*>(Storage()));
		}

		alignas(_Alignment) std::array<std::byte, _Size> storage_;
	};

	template<class _FunctionType, std::size_t _Size = 32, std::size_t _Alignment = alignof(std::max_align_t)>
	using Delegate = MoveOnlyDelegate<_FunctionType, _Size, _Alignment>;

	template<class _FunctionType, std::size_t _Size = 32, std::size_t _Alignment = alignof(std::max_align_t)>
	using MoveOnlyFunction = MoveOnlyDelegate<_FunctionType, _Size, _Alignment>;

	template<class _FunctionType, std::size_t _Size = 32, std::size_t _Alignment = alignof(std::max_align_t)>
	class CopyableDelegate;

	template<class _Result, class... _Args, std::size_t _Size, std::size_t _Alignment>
	class CopyableDelegate<_Result(_Args...), _Size, _Alignment> : public IDelegate<_Result(_Args...)>
	{
		using Base = IDelegate<_Result(_Args...)>;
		using Operations = typename Base::Operations;

	public:
		static_assert(_Size > 0);
		static_assert(_Alignment > 0);

		static constexpr std::size_t Size = _Size;
		static constexpr std::size_t Alignment = _Alignment;

		constexpr CopyableDelegate() noexcept = default;

		template<class _Functor>
			requires(!std::same_as<std::remove_cvref_t<_Functor>, CopyableDelegate> && sizeof(std::decay_t<_Functor>) <= _Size && alignof(std::decay_t<_Functor>) <= _Alignment && std::is_nothrow_move_constructible_v<std::decay_t<_Functor>> && std::is_nothrow_copy_constructible_v<std::decay_t<_Functor>> && std::is_nothrow_constructible_v<std::decay_t<_Functor>, _Functor&&> && std::is_invocable_r_v<_Result, std::decay_t<_Functor>&, _Args...>)
		constexpr CopyableDelegate(_Functor&& functor) noexcept
		{
			Emplace<std::decay_t<_Functor>>(std::forward<_Functor>(functor));
		}

		constexpr CopyableDelegate(const CopyableDelegate& other) noexcept
		{
			CopyFrom(other);
		}

		constexpr CopyableDelegate& operator=(const CopyableDelegate& other) noexcept
		{
			if (this != std::addressof(other))
			{
				Reset();
				CopyFrom(other);
			}

			return *this;
		}

		constexpr CopyableDelegate(CopyableDelegate&& other) noexcept
		{
			MoveFrom(other);
		}

		constexpr CopyableDelegate& operator=(CopyableDelegate&& other) noexcept
		{
			if (this != std::addressof(other))
			{
				Reset();
				MoveFrom(other);
			}

			return *this;
		}

		template<class _Functor>
			requires(!std::same_as<std::remove_cvref_t<_Functor>, CopyableDelegate> && sizeof(std::decay_t<_Functor>) <= _Size && alignof(std::decay_t<_Functor>) <= _Alignment && std::is_nothrow_move_constructible_v<std::decay_t<_Functor>> && std::is_nothrow_copy_constructible_v<std::decay_t<_Functor>> && std::is_nothrow_constructible_v<std::decay_t<_Functor>, _Functor&&> && std::is_invocable_r_v<_Result, std::decay_t<_Functor>&, _Args...>)
		constexpr CopyableDelegate& operator=(_Functor&& functor) noexcept
		{
			Reset();
			Emplace<std::decay_t<_Functor>>(std::forward<_Functor>(functor));
			return *this;
		}

		constexpr ~CopyableDelegate() noexcept
		{
			Reset();
		}

		template<class _Functor, class... _ConstructorArgs>
			requires(sizeof(_Functor) <= _Size && alignof(_Functor) <= _Alignment && std::is_nothrow_move_constructible_v<_Functor> && std::is_nothrow_copy_constructible_v<_Functor> && std::is_invocable_r_v<_Result, _Functor&, _Args...> && std::constructible_from<_Functor, _ConstructorArgs...> && std::is_nothrow_constructible_v<_Functor, _ConstructorArgs...>)
		constexpr void Emplace(_ConstructorArgs&&... args) noexcept
		{
			Reset();
			std::construct_at(As<_Functor>(), std::forward<_ConstructorArgs>(args)...);
			this->Bind(std::addressof(GetOperations<_Functor>()));
		}

		constexpr void Reset() noexcept
		{
			if (const Operations* operations = this->OperationTable())
			{
				operations->destroy(Storage());
			}

			Base::Reset();
		}

	private:
		template<class _Functor>
		static _Result Invoke(void* object, _Args... args)
		{
			auto* delegate = static_cast<CopyableDelegate*>(static_cast<Base*>(object));
			_Functor& functor = *delegate->template As<_Functor>();
			if constexpr (std::is_void_v<_Result>)
			{
				std::invoke(functor, std::forward<_Args>(args)...);
			}
			else
			{
				return std::invoke(functor, std::forward<_Args>(args)...);
			}
		}

		template<class _Functor>
		static void Destroy(void* object) noexcept
		{
			std::destroy_at(std::launder(reinterpret_cast<_Functor*>(object)));
		}

		template<class _Functor>
		static void Move(void* destination, void* source) noexcept
		{
			_Functor* source_functor = std::launder(reinterpret_cast<_Functor*>(source));
			std::construct_at(std::launder(reinterpret_cast<_Functor*>(destination)), std::move(*source_functor));
			std::destroy_at(source_functor);
		}

		template<class _Functor>
		static void Copy(void* destination, const void* source) noexcept
		{
			const _Functor* source_functor = std::launder(reinterpret_cast<const _Functor*>(source));
			std::construct_at(std::launder(reinterpret_cast<_Functor*>(destination)), *source_functor);
		}

		template<class _Functor>
		static const Operations& GetOperations() noexcept
		{
			static constexpr Operations operations{ &Invoke<_Functor>, &Destroy<_Functor>, &Move<_Functor>, &Copy<_Functor> };
			return operations;
		}

		constexpr void CopyFrom(const CopyableDelegate& other) noexcept
		{
			if (const Operations* operations = other.OperationTable(); operations == nullptr)
			{
				return;
			}

			const Operations* operations = other.OperationTable();
			operations->copy(Storage(), other.Storage());
			this->Bind(operations);
		}

		constexpr void MoveFrom(CopyableDelegate& other) noexcept
		{
			if (const Operations* operations = other.OperationTable(); operations == nullptr)
			{
				return;
			}

			const Operations* operations = other.OperationTable();
			operations->move(Storage(), other.Storage());
			this->Bind(operations);
			other.Unbind();
		}

		constexpr void Unbind() noexcept
		{
			Base::Reset();
		}

		constexpr void* Storage() noexcept
		{
			return storage_.data();
		}

		constexpr const void* Storage() const noexcept
		{
			return storage_.data();
		}

		template<class _Functor>
		constexpr _Functor* As() noexcept
		{
			return std::launder(reinterpret_cast<_Functor*>(Storage()));
		}

		alignas(_Alignment) std::array<std::byte, _Size> storage_;
	};

	template<class _FunctionType, std::size_t _Size = 32, std::size_t _Alignment = alignof(std::max_align_t)>
	using CopyableFunction = CopyableDelegate<_FunctionType, _Size, _Alignment>;

	class MulticastDelegate
	{

	};

}
