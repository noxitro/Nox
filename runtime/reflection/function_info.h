///	@file	function.h
///	@brief	function
#pragma once

namespace nox::reflection
{
	//	前方宣言
	class ClassInfo;
	class ReflectionObject;

	/// @brief 関数の1引数情報
	class NOX_ATTR(nox::reflection::attr::IgnoreReflection()) FunctionArgumentInfo
	{
	public:
		/**
		 * @brief 引数付きコンストラクタ
		 * @param name 引数名
		 * @param type 型情報
		*/
		[[nodiscard]] inline constexpr explicit FunctionArgumentInfo(
			const ReflectionStringView name,
			const std::reference_wrapper<const ReflectionObject>* attribute_list,
			const std::uint8_t attribute_list_length,
			const nox::reflection::Type& type,
			const bool hasDefaultValue = false
		)noexcept :
			name_(name),
			underlying_type_(type),
			attribute_list_(attribute_list),
			attribute_list_length_(attribute_list_length),
			has_default_value_(hasDefaultValue)
		{}

		inline constexpr FunctionArgumentInfo(const FunctionArgumentInfo&)noexcept = delete;
		inline constexpr FunctionArgumentInfo(const FunctionArgumentInfo&&)noexcept = delete;
		inline constexpr ~FunctionArgumentInfo()noexcept = default;

		/// @brief 引数名を取得
		[[nodiscard]] inline	constexpr const ReflectionStringView GetName()const noexcept { return name_; }

		/// @brief タイプ情報を取得
		[[nodiscard]] inline	constexpr const Type& GetType()const noexcept { return underlying_type_; }
		[[nodiscard]] inline	constexpr const Type& GetUnderlyingType()const noexcept { return underlying_type_; }

		/// @brief デフォルト値を持っているか
		[[nodiscard]] inline	constexpr bool HasDefaultValue()const noexcept { return has_default_value_; }


		[[nodiscard]] inline constexpr	std::uint8_t GetAttributeLength()const noexcept { return attribute_list_length_; }
		[[nodiscard]] inline constexpr	const ReflectionObject& GetAttribute(std::uint8_t index)const noexcept { return util::At(attribute_list_, attribute_list_length_, index); }
		[[nodiscard]] inline constexpr	auto GetAttributeList()const noexcept { return std::span(attribute_list_, attribute_list_length_); }
	private:
		/// @brief デフォルト値を持っているか
		const bool has_default_value_;

		/// @brief 属性リストの長さ
		std::uint8_t attribute_list_length_;

		/// @brief 属性リスト
		const std::reference_wrapper<const ReflectionObject>* attribute_list_;
		
		/// @brief タイプ情報
		const reflection::Type& underlying_type_;
	
		/// @brief 引数名
		const ReflectionStringView name_;

		
//		const std::span<std::reference_wrapper<const ReflectionObject>> attribute_list_;
	};

	namespace detail
	{
		template<class>
		class FunctionInfoImpl;
	}

	/// @brief 関数情報
	class FunctionInfo
	{
	public:
		inline	constexpr	explicit FunctionInfo(
			ReflectionStringView	name,
			ReflectionStringView	fullname,
			ReflectionStringView	_namespace,
			const std::reference_wrapper<const ReflectionObject>* attribute_list,
			const std::uint8_t attribute_list_length,
			const nox::FunctionPointerId& function_id,
			const std::reference_wrapper<const FunctionArgumentInfo>* function_param_list,
			const std::uint8_t function_param_list_length,
			const Type& owner_class_type,
			const reflection::Type& result_type,
			const AccessLevel access_level,
			const FunctionAttributeFlag method_attribute_flags
			)noexcept :
			name_(name),
			fullname_(fullname),
			namespace_(_namespace),
			containing_type_(owner_class_type),
			result_type_(result_type),
			attribute_list_(attribute_list),
			attribute_list_length_(attribute_list_length),
			function_id_(function_id),
			function_param_list_(function_param_list),
			function_param_list_length_(function_param_list_length),
			access_level_(access_level),
			function_attribute_flags_(method_attribute_flags)
		{}

	public:
#pragma region アクセサ

		[[nodiscard]] inline	constexpr	ReflectionStringView GetName()const noexcept { return name_; }
		[[nodiscard]] inline	constexpr	ReflectionStringView GetFullName()const noexcept { return fullname_; }
		[[nodiscard]] inline	constexpr	ReflectionStringView GetNamespace()const noexcept { return namespace_; }
		[[nodiscard]] inline	constexpr	nox::reflection::AccessLevel GetAccessLevel()const noexcept { return access_level_; }
		[[nodiscard]] inline	constexpr	const nox::FunctionPointerId& GetFunctionId()const noexcept { return function_id_; }

		[[nodiscard]] inline	constexpr	std::span<const std::reference_wrapper<const nox::reflection::ReflectionObject>> GetAttributeList()const noexcept { return std::span(attribute_list_, attribute_list_length_); }
		[[nodiscard]] inline	constexpr	std::uint8_t GetAttributeListLength()const noexcept { return attribute_list_length_; }
		[[nodiscard]] inline	constexpr	const nox::reflection::ReflectionObject& GetAttribute(std::uint8_t index)const noexcept { return nox::util::At(attribute_list_, attribute_list_length_, index); }

		[[nodiscard]] inline	constexpr	std::span<const std::reference_wrapper<const FunctionArgumentInfo>> GetFunctionParamList()const noexcept { return std::span(function_param_list_, function_param_list_length_); }
		[[nodiscard]] inline	constexpr	std::uint8_t GetFunctionParamLength()const noexcept { return function_param_list_length_; }
		[[nodiscard]] inline	constexpr	std::uint8_t GetNonDefaultParamLength()const noexcept
		{
			std::uint8_t count = 0;
			for (std::uint8_t i = 0; i < function_param_list_length_; ++i)
			{
				if (function_param_list_[i].get().HasDefaultValue() == false)
				{
					++count;
				}
			}
			return count;
		}

		[[nodiscard]] inline	constexpr	const FunctionArgumentInfo& GetFunctionParam(std::uint8_t index)const noexcept { return nox::util::At(function_param_list_, function_param_list_length_, index); }

		[[nodiscard]] inline	constexpr	const nox::reflection::Type& GetOwnerType()const noexcept { return containing_type_; }
		[[nodiscard]] inline	constexpr	const nox::reflection::Type& GetResultType()const noexcept { return result_type_; }

		[[nodiscard]] inline	constexpr	bool	IsStatic()const noexcept { return nox::util::IsBitAnd(function_attribute_flags_, FunctionAttributeFlag::Static); }
		[[nodiscard]] inline	constexpr	bool	IsNoReturn()const noexcept { return result_type_ == nox::reflection::GetInvalidType(); }

		[[nodiscard]] inline constexpr bool IsConstructor()const noexcept {
			return
				IsDefaultConstructor() ||
				IsCopyConstructor() ||
				IsMoveConstructor();
		}

		[[nodiscard]] inline constexpr bool IsDefaultConstructor()const noexcept { return nox::util::IsBitAnd(function_attribute_flags_, FunctionAttributeFlag::DefaultConstructor); }
		[[nodiscard]] inline constexpr bool IsCopyConstructor()const noexcept { return nox::util::IsBitAnd(function_attribute_flags_, FunctionAttributeFlag::CopyConstructor); }
		[[nodiscard]] inline constexpr bool IsMoveConstructor()const noexcept { return nox::util::IsBitAnd(function_attribute_flags_, FunctionAttributeFlag::MoveConstructor); }

#pragma endregion

#pragma region 関数実行
		/// @brief 関数呼び出し
		/// @tparam ResultType 
		/// @tparam ...Args 
		/// @param ...args 
		/// @return 
		template<class ResultType = void, class... Args>
		inline	constexpr	ResultType	Invoke(Args&&... args)const
		{
			std::optional<ResultType> result = TryInvoke<ResultType>(std::forward<Args>(args)...);
			NOX_ASSERT(result.has_value(), U"関数呼び出しに失敗しました");

			if constexpr (std::is_void_v<ResultType>)
			{
				return;
			}
			else
			{
				return *result;
			}
		}

		template<class R = void, class... Args> 
		inline	constexpr	std::optional<std::conditional_t<std::is_void_v<R>, std::monostate, R>>  TryInvoke(Args&&... args)const
		{
			if constexpr (std::is_void_v<R>)
			{
				return TryInvokeImpl<R>(std::forward<Args>(args)...);
			}
			else
			{
				if (result_type_ == nox::reflection::Typeof<R>())
				{
					return TryInvokeImpl<R>(std::forward<Args>(args)...);
				}
				else if (result_type_ == nox::reflection::Typeof<std::add_const_t<R>>())
				{
					return TryInvokeImpl<std::add_const_t<R>>(std::forward<Args>(args)...);
				}
				else if (result_type_ == nox::reflection::Typeof<std::remove_const_t<R>>())
				{
					return TryInvokeImpl<std::remove_const_t<R>>(std::forward<Args>(args)...);
				}
				else if (result_type_ == nox::reflection::Typeof<std::add_lvalue_reference_t<R>>())
				{
					return TryInvokeImpl<std::add_lvalue_reference_t<R>>(std::forward<Args>(args)...);
				}
				else if (result_type_ == nox::reflection::Typeof<std::add_lvalue_reference_t<const R>>())
				{
					return TryInvokeImpl<std::add_lvalue_reference_t<const R>>(std::forward<Args>(args)...);
				}
				else
				{
					return std::nullopt;
				}
			}
		}

#pragma endregion

	protected:
		inline constexpr virtual std::optional<std::monostate> InvokeImplNoReturn(std::span<void*> args)const = 0;

	private:
		template<class R, class... Args>
		inline	constexpr	std::optional<std::conditional_t<std::is_void_v<R>, std::monostate, R>>	TryInvokeImpl(Args&&... args)const
		{
			const std::uint8_t need_param_length = GetNonDefaultParamLength() + (IsStatic() ? 0 : 1);
			if (sizeof...(Args) > need_param_length)
			{
				return std::nullopt;
			}

			constexpr std::array<std::reference_wrapper<const nox::reflection::Type>, sizeof...(Args)> invoke_type_list = { reflection::Typeof<Args>()... };
			if (IsStatic()==false)
			{
				const nox::reflection::Type& type = invoke_type_list[0];
				if (type.IsConvertible(containing_type_) == false)
				{
					return std::nullopt;
				}
			}

			{
				const std::uint8_t offset = IsStatic() ? 0 : 1;
				for (std::uint8_t i = offset; i < sizeof...(Args) + offset; ++i)
				{
					const nox::reflection::Type& type = invoke_type_list[i];
					if (type.IsConvertible(this->GetFunctionParam(i- offset).GetType()) == false)
					{
						return std::nullopt;
					}
				}
			}

			std::array<void*, sizeof...(Args)> invoke_args = { const_cast<void*>(static_cast<const void*>(&args))... };
			if constexpr (std::is_void_v<R>)
			{
				return InvokeImplNoReturn(invoke_args);
			}
			else
			{
				return static_cast<const nox::reflection::detail::FunctionInfoImpl<R>&>(*this).InvokeImpl(invoke_args);
			}
		}

	protected:
		/// @brief 属性の数
		std::uint8_t attribute_list_length_;

		/// @brief 引数の数
		std::uint8_t function_param_list_length_;

		/// @brief 関数名
		ReflectionStringView	name_;

		/// @brief フルネーム
		ReflectionStringView	fullname_;

		/// @brief 名前空間
		ReflectionStringView	namespace_;

		/// @brief 関数のID
		const nox::FunctionPointerId& function_id_;

		/// @brief クラス情報
		const reflection::Type& containing_type_;

		/// @brief 戻り値の型
		const reflection::Type& result_type_;

		/// @brief 属性リスト
		const std::reference_wrapper<const ReflectionObject>* attribute_list_;

		/// @brief 引数テーブル
		const std::reference_wrapper<const FunctionArgumentInfo>* function_param_list_;

		/// @brief アクセスレベル
		const AccessLevel access_level_;

		/// @brief 関数の属性
		const	FunctionAttributeFlag	function_attribute_flags_;
	};

	namespace detail
	{
		template<class ResultType>
		class FunctionInfoImpl final : public FunctionInfo
		{
		public:
			inline constexpr FunctionInfoImpl(const FunctionInfoImpl&)noexcept = delete;
			inline constexpr FunctionInfoImpl(FunctionInfoImpl&&)noexcept = delete;

			inline	constexpr	explicit	FunctionInfoImpl(
				ReflectionStringView	name,
				ReflectionStringView fullname,
				ReflectionStringView	_namespace,
				const std::reference_wrapper<const ReflectionObject>* attribute_list,
				const std::uint8_t attribute_list_length,
				const nox::FunctionPointerId& function_id,
				const std::reference_wrapper<const FunctionArgumentInfo>* function_param_list,
				const std::uint8_t function_param_list_length,
				const Type& owner_class_type,
				const reflection::Type& result_type,
				AccessLevel access_level,
				FunctionAttributeFlag method_attribute_flags,
				const std::tuple<ResultType(*)(void**), std::uint8_t>* function_holder_table,
				std::uint8_t function_holder_table_length
			)noexcept :
				FunctionInfo(
					name,
					fullname,
					_namespace,
					attribute_list,
					attribute_list_length,
					function_id,
					function_param_list,
					function_param_list_length,
					owner_class_type,
					result_type,
					access_level,
					method_attribute_flags
				),
				function_holder_table_(function_holder_table),
				function_holder_table_length_(function_holder_table_length)
			{}

		public:
			inline constexpr std::optional<std::conditional_t<std::is_void_v<ResultType>, std::monostate, ResultType>> InvokeImpl(std::span<void*> args)const
			{
				return this->InvokeImpl<ResultType>(args);
			}

		protected:
			inline constexpr std::optional<std::monostate> InvokeImplNoReturn(std::span<void*> args)const override
			{
				return this->InvokeImpl<void>(args);
			}

		private:
			template<class R>
			inline constexpr std::optional<std::conditional_t<std::is_void_v<R>, std::monostate, R>> InvokeImpl(std::span<void*> args)const
			{
			//	const std::uint8_t raw_arg_length = static_cast<std::uint8_t>(args.size()) - (IsStatic() ? 0 : 1);
				const std::optional<ResultType(*)(void**)> function_pointer_result = FindFunctionPointer(static_cast<std::uint8_t>(args.size()));
				if (function_pointer_result.has_value() == false)
				{
					return std::nullopt;
				}

				if constexpr (std::is_void_v<R>)
				{
					std::invoke(*function_pointer_result, args.data());
					return std::monostate{};
				}
				else
				{
					return std::invoke(*function_pointer_result, args.data());
				}
			}

			inline constexpr std::optional<ResultType(*)(void**)> FindFunctionPointer(std::uint8_t arg_length)const noexcept
			{
				for (std::uint8_t i = 0; i < function_holder_table_length_; ++i)
				{
					if (std::get<1>(function_holder_table_[i]) == arg_length)
					{
						return std::get<0>(function_holder_table_[i]);
					}
				}
				return std::nullopt;
			}
		private:
			/// @brief 関数リスト
			const std::tuple<ResultType(*)(void**), std::uint8_t>* function_holder_table_;
			const std::uint8_t function_holder_table_length_;
		};

		template<class RawFunction>
		inline	constexpr	nox::reflection::detail::FunctionInfoImpl<nox::FunctionResultType<RawFunction>>	CreateFunctionInfo(
			const nox::FunctionPointerId& function_id,
			ReflectionStringView	name,
			ReflectionStringView	fullname,
			ReflectionStringView	_namespace,
			AccessLevel access_level,
			const std::reference_wrapper<const ReflectionObject>* attribute_list,
			const std::uint8_t attribute_list_length,
			const std::reference_wrapper<const FunctionArgumentInfo>* function_param_list,
			const std::uint8_t function_param_list_length,
			const FunctionAttributeFlag extraAttributeFlags,
			const std::tuple<nox::FunctionResultType<RawFunction>(*)(void**), std::uint8_t>* function_holder_table,
			const std::uint8_t function_holder_table_length
		)noexcept
		{
			//	c++で解決できないものは、ここで解決する
			const FunctionAttributeFlag method_attribute_flags = nox::util::BitOr(nox::reflection::GetFunctionAttributeFlags<RawFunction>(), extraAttributeFlags);
			
			if constexpr (std::is_member_function_pointer_v<RawFunction> == true)
			{
				return nox::reflection::detail::FunctionInfoImpl<nox::FunctionResultType<RawFunction>>(
					name,
					fullname,
					_namespace,
					attribute_list,
					attribute_list_length,
					function_id,
					function_param_list,
					function_param_list_length,
					nox::reflection::Typeof<nox::FunctionClassType<RawFunction>>(),
					nox::reflection::Typeof<nox::FunctionResultType<RawFunction>>(),
					access_level,
					method_attribute_flags,
					function_holder_table,
					function_holder_table_length
				);
			}
			else
			{
				return nox::reflection::detail::FunctionInfoImpl<nox::FunctionResultType<RawFunction>>(
					name,
					fullname,
					_namespace,
					attribute_list,
					attribute_list_length,
					function_id,
					function_param_list,
					function_param_list_length,
					nox::reflection::GetInvalidType(),
					nox::reflection::Typeof<nox::FunctionResultType<RawFunction>>(),
					access_level,
					method_attribute_flags,
					function_holder_table,
					function_holder_table_length
				);
			}
			
		}
	}
}