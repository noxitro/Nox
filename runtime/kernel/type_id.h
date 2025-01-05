//	Copyright (C) 2024 NOX ENGINE All Rights Rserved.

///	@file	type_id.h
///	@brief	type_id
#pragma once
#include	<type_traits>
#include	"basic_type.h"

namespace nox
{
	/// @brief 非型パラメータのID
	class NontypeId
	{
	protected:
		constexpr NontypeId() noexcept = default;
		inline constexpr NontypeId(const NontypeId&)noexcept = delete;
		inline constexpr NontypeId(NontypeId&&)noexcept = delete;
	};

	[[nodiscard]]
	inline constexpr bool operator==(const nox::NontypeId& a, const nox::NontypeId& b)noexcept
	{
		return &a == &b;
	}

	/// @brief 関数ポインタIDクラス
	struct FunctionPointerId
	{
	protected:
		constexpr FunctionPointerId() noexcept = default;
		inline constexpr FunctionPointerId(const FunctionPointerId&)noexcept = delete;
		inline constexpr FunctionPointerId(FunctionPointerId&&)noexcept = delete;
	public:
		
	};

	[[nodiscard]]
	inline constexpr bool operator==(const nox::FunctionPointerId& a, const nox::FunctionPointerId& b)noexcept
	{
		return &a == &b;
	}

	/// @brief オブジェクトポインタIDクラス
	struct ObjectPointerId
	{
	protected:
		constexpr ObjectPointerId() noexcept = default;
		inline constexpr ObjectPointerId(const ObjectPointerId&)noexcept = delete;
		inline constexpr ObjectPointerId(ObjectPointerId&&)noexcept = delete;

		inline constexpr ObjectPointerId& operator=(const ObjectPointerId&)noexcept = delete;
		inline constexpr ObjectPointerId& operator=(ObjectPointerId&&)noexcept = delete;
	public:
		inline constexpr bool operator==(const ObjectPointerId& other)const noexcept
		{
			return this == &other;
		}
	};

	namespace detail
	{
		struct InvalidNontypeId : nox::NontypeId
		{
			inline constexpr InvalidNontypeId()noexcept = default;
		};

		constexpr nox::detail::InvalidNontypeId kInvalidNontypeId{};


		template<auto>
		struct NontypeIdHolder
		{
			struct NontypeIdImpl : nox::NontypeId
			{
				inline constexpr NontypeIdImpl()noexcept = default;
			};

			static constexpr NontypeIdImpl value{};
		};


		template<auto>
		struct FunctionPointerIdImpl : FunctionPointerId
		{
		};
		template<auto Func>
			requires(
			std::is_function_v<std::remove_pointer_t<decltype(Func)>> ||
			std::is_member_function_pointer_v<decltype(Func)>)
		struct FunctionPointerIdHolder
		{
		private:
		

		public:
			static constexpr FunctionPointerIdImpl<Func> value;
			inline constexpr FunctionPointerIdHolder()noexcept = delete;
			inline constexpr ~FunctionPointerIdHolder()noexcept = delete;
		};

		template<auto>
		struct ObjectPointerIdImpl : ObjectPointerId
		{
		};

		template<auto ObjectPointer>
			requires(
		std::is_pointer_v<decltype(ObjectPointer)>
			)
		struct ObjectPointerIdHolder
		{
		private:
			
		public:
			static constexpr ObjectPointerIdImpl<ObjectPointer> value;
			inline constexpr ObjectPointerIdHolder()noexcept = delete;
			inline constexpr ~ObjectPointerIdHolder()noexcept = delete;
	
		};

		struct ObjectPointerIdInvalid : ObjectPointerId
		{
		};

		constexpr ObjectPointerIdInvalid ObjectPointerIdInvalidValue;
	}

	[[nodiscard]] inline constexpr const nox::NontypeId& GetInvalidNontypeId()noexcept
	{
		return nox::detail::kInvalidNontypeId;
	}

	template<auto Value>
	[[nodiscard]] inline constexpr const nox::NontypeId& GetNontypeId()noexcept
	{
		return nox::detail::NontypeIdHolder<Value>::value;
	}

	template<auto Func>	requires(
		std::is_function_v<std::remove_pointer_t<decltype(Func)>> ||
		std::is_member_function_pointer_v<decltype(Func)>)
	[[nodiscard]] inline constexpr const nox::FunctionPointerId& GetFunctionPointerId()noexcept
	{
		return nox::detail::FunctionPointerIdHolder<Func>::value;
	}


	[[nodiscard]] inline constexpr const nox::ObjectPointerId& GetInvalidObjectPointerId()noexcept
	{
		return nox::detail::ObjectPointerIdInvalidValue;
	}

	template<auto ObjectPointer> requires(
		std::is_pointer_v<decltype(ObjectPointer)>
		)
	[[nodiscard]] inline constexpr const nox::ObjectPointerId& GetObjectPointerId()noexcept
	{
		return nox::detail::ObjectPointerIdHolder<ObjectPointer>::value;
	}
}