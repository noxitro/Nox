///	@file	delegate.h
///	@brief	delegate
#pragma once
#include	"algorithm.h"
#include	"assertion.h"
#include	"memory/memory_util.h"
#include	"type_traits/function_signature.h"
#include	"type_traits/type_name.h"
#include	"reflection_type.h"

#if false

namespace nox
{
	namespace detail
	{

	}

	template<class _F>
	class IDelegate
	{
	protected:
		struct ICallable
		{
			inline constexpr virtual ~ICallable()noexcept;

			inline constexpr virtual nox::FunctionResultType<_F> operator()(const nox::FunctionArgsTupleType<_F>&)const noexcept(nox::IsFunctionNoexceptValue<_F>) = 0;
		};

		template<class T>
		struct Callable
		{
			inline constexpr nox::FunctionResultType<_F> operator()(const nox::FunctionArgsTupleType<_F>& args)const noexcept(nox::IsFunctionNoexceptValue<_F>) override
			{
				if constexpr (std::is_void_v<nox::FunctionResultType<_F>>)
				{
					std::apply(functor_, args);
				}
				else
				{
					return std::apply(functor_, args);
				}
			}
			T functor_;
		};

	public:
		inline constexpr IDelegate()noexcept = default;

		template<class... Args>
		inline constexpr decltype(auto) operator()(Args&&... args)const
		{
			return (*GetCallable())(std::forward_as_tuple(std::forward<Args>(args)...));
		}

	private:
		ICallable* callable_;
	};

	template<class _FuncType>
	class Delegate
	{

	};

	class MulticastDelegate
	{

	};
}
#endif // false
