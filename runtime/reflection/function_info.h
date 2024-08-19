///	@file	function.h
///	@brief	function
#pragma once
#include	"type.h"
#include	"reflection_object.h"

namespace nox::reflection
{

	/// @brief 関数の1引数情報
	class FunctionArgumentInfo
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
			type_(type),
			attribute_list_(attribute_list),
			attribute_list_length_(attribute_list_length),
			has_default_value_(hasDefaultValue)
		{}

		inline constexpr ~FunctionArgumentInfo()noexcept = default;

		/// @brief 引数名を取得
		[[nodiscard]] inline	constexpr const ReflectionStringView GetName()const noexcept { return name_; }

		/// @brief タイプ情報を取得
		[[nodiscard]] inline	constexpr const Type& GetType()const noexcept { return type_; }

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
		const reflection::Type& type_;
	
		/// @brief 引数名
		const ReflectionStringView name_;

		
//		const std::span<std::reference_wrapper<const ReflectionObject>> attribute_list_;
	};

	/// @brief 関数情報
	class FunctionInfo
	{
	protected:
		/// @brief 関数呼び出し用の引数情報
		struct InvokeArgument
		{
			constexpr InvokeArgument()noexcept = delete;

			inline	constexpr explicit InvokeArgument(
				const reflection::Type& type,
				const void* _constDataPtr,
				void* _dataPtr,
				bool is_const
			)noexcept :
				type_(type),
				const_data_ptr(_constDataPtr),
				data_ptr(_dataPtr),
				is_const_(is_const)
			{}



			/// @brief 引数の型
			const reflection::Type& type_;

		//	union
		//	{
				/// @brief 引数実態(constVer)
				const void* const_data_ptr;

				/// @brief 引数実態
				void* data_ptr;
		//	};

			/// @brief 書き換え不可(data_ptr == nullptr)
			bool is_const_;

			inline	constexpr	bool	IsConst()const noexcept { return is_const_; }

			template<class T>
			inline	static	constexpr	InvokeArgument	MakeArgument(T& value)noexcept
			{
				if constexpr (std::is_const_v<std::remove_pointer_t<std::remove_reference_t<T>>> == false)
				{
					return InvokeArgument(
						reflection::Typeof<T>(),
						static_cast<const void*>(&value),
						static_cast<void*>(&value),
						false
					);
				}
				else
				{
					return InvokeArgument(
						reflection::Typeof<T>(),
						static_cast<const void*>(&value),
						nullptr,
						true
					);
				}
			}
		};

	protected:
		inline	constexpr	explicit FunctionInfo(
			ReflectionStringView	name,
			ReflectionStringView fullname,
			ReflectionStringView	_namespace,
			const std::reference_wrapper<const ReflectionObject>* attribute_list,
			const std::uint8_t attribute_list_length,
			const std::reference_wrapper<const FunctionArgumentInfo>* function_param_list,
			const std::uint8_t function_param_list_length,
			const Type& owner_class_type,
			const reflection::Type& result_type,
			const AccessLevel access_level,
			const FunctionType	method_type,
			const FunctionAttributeFlag method_attribute_flags
			)noexcept :
			name_(name),
			fullname_(fullname),
			namespace_(_namespace),
			containing_type_(owner_class_type),
			result_type_(result_type),
			attribute_list_(attribute_list),
			attribute_list_length_(attribute_list_length),
			function_param_list_(function_param_list),
			function_param_list_length_(function_param_list_length),
			access_level_(access_level),
			method_attribute_flags_(method_attribute_flags),
			method_type_(method_type) {}

	public:
#pragma region アクセサ

		[[nodiscard]] inline	constexpr	ReflectionStringView GetName()const noexcept { return name_; }
		[[nodiscard]] inline	constexpr	ReflectionStringView GetFullName()const noexcept { return fullname_; }
		[[nodiscard]] inline	constexpr	ReflectionStringView GetNamespace()const noexcept { return namespace_; }
		[[nodiscard]] inline	constexpr	nox::reflection::AccessLevel GetAccessLevel()const noexcept { return access_level_; }

		[[nodiscard]] inline	constexpr	std::span<const std::reference_wrapper<const nox::reflection::ReflectionObject>> GetAttributeList()const noexcept { return std::span(attribute_list_, attribute_list_length_); }
		[[nodiscard]] inline	constexpr	std::uint8_t GetAttributeListLength()const noexcept { return attribute_list_length_; }
		[[nodiscard]] inline	constexpr	const nox::reflection::ReflectionObject& GetAttribute(std::uint8_t index)const noexcept { return nox::util::At(attribute_list_, attribute_list_length_, index); }

		[[nodiscard]] inline	constexpr	std::span<const std::reference_wrapper<const FunctionArgumentInfo>> GetFunctionParamList()const noexcept { return std::span(function_param_list_, function_param_list_length_); }
		[[nodiscard]] inline	constexpr	std::uint8_t GetFunctionParamLength()const noexcept { return function_param_list_length_; }
		[[nodiscard]] inline	constexpr	const FunctionArgumentInfo& GetFunctionParam(std::uint8_t index)const noexcept { return nox::util::At(function_param_list_, function_param_list_length_, index); }

		[[nodiscard]] inline	constexpr	const nox::reflection::Type& GetOwnerType()const noexcept { return containing_type_; }
		[[nodiscard]] inline	constexpr	const nox::reflection::Type& GetResultType()const noexcept { return result_type_; }
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
			std::array<InvokeArgument, sizeof...(Args)> argument_list{ InvokeArgument::MakeArgument<Args>(args)... };
			if constexpr (std::is_void_v<ResultType> == true)
			{
				TryInvokeImpl(nullptr, reflection::Typeof<ResultType>(), argument_list);
			}
			else
			{
				std::array<nox::char8, sizeof(ResultType)> buffer;
				return *static_cast<std::add_pointer_t<ResultType>>(TryInvokeImpl(buffer.data(), reflection::Typeof<ResultType>(), argument_list));
			}
		}
#pragma endregion

		template<class T> requires(std::is_void_v<std::remove_pointer_t<T>>)
		static	inline	constexpr	T	ToInvokeParam(InvokeArgument& argument)noexcept
		{
			if constexpr (std::is_const_v<std::remove_pointer_t<std::remove_reference_t<T>>> == true)
			{
				return argument.const_data_ptr;
			}
			else
			{
				return argument.data_ptr;
			}
		}


	protected:
		virtual constexpr	void* TryInvokeImpl(void* result, const reflection::Type& result_type, const std::span<InvokeArgument>& argument_list)const = 0;

		template<class Func, std::size_t... Indices>
		inline constexpr FunctionReturnType<Func> TryInvokeImpl_Private(Func func, const std::span<InvokeArgument>& argument_list, std::index_sequence<Indices...>)const
		{
			//	引数チェック
			for (std::int32_t i = 0; i < static_cast<std::int32_t>(argument_list.size()); ++i)
			{
				//	型チェック
				if (argument_list[i].type_.IsConvertible(this->GetFunctionParam(i).GetType()) == false)
				{
					NOX_ASSERT(false, U"型チェックに失敗しました");
				}
			}
			
			//	呼び出し
			if constexpr (std::is_void_v<FunctionReturnType<Func>>)
			{
				//	戻り値なし
				std::invoke(func, FunctionInfo::ToInvokeParam<std::tuple_element_t<Indices, FunctionArgsType<Func>>>(argument_list[Indices])...);
			}
			else
			{
				return std::invoke(func, FunctionInfo::ToInvokeParam<std::tuple_element_t<Indices, FunctionArgsType<Func>>>(argument_list[Indices])...);
			}
		}
	protected:
		std::uint8_t attribute_list_length_;
		std::uint8_t function_param_list_length_;

		/// @brief 関数名
		ReflectionStringView	name_;

		/// @brief フルネーム
		ReflectionStringView	fullname_;

		/// @brief 名前空間
		ReflectionStringView	namespace_;

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
		const	FunctionAttributeFlag	method_attribute_flags_;

		/// @brief 関数の種類
		const	FunctionType	method_type_;

	};

	namespace detail
	{
		template<nox::concepts::GlobalFunctionPointer... _Functions>
		class FunctionInfoImpl : public FunctionInfo
		{
		public:
			inline	constexpr	explicit	FunctionInfoImpl(
				ReflectionStringView	name,
				ReflectionStringView fullname,
				ReflectionStringView	_namespace,
				const std::reference_wrapper<const ReflectionObject>* attribute_list,
				const std::uint8_t attribute_list_length,
				const std::reference_wrapper<const FunctionArgumentInfo>* function_param_list,
				const std::uint8_t function_param_list_length,
				const Type& owner_class_type,
				const reflection::Type& result_type,
				AccessLevel access_level,
				FunctionType	method_type,
				FunctionAttributeFlag method_attribute_flags,
				_Functions... functions
			)noexcept :
				FunctionInfo(
					name,
					fullname,
					_namespace,
					attribute_list,
					attribute_list_length,
					function_param_list,
					function_param_list_length,
					owner_class_type,
					result_type,
					access_level,
					method_type,
					method_attribute_flags
				),
				functions_(std::make_tuple(functions...))
			{}


		protected:
			inline constexpr	void* TryInvokeImpl(void* result, const reflection::Type& result_type, const std::span<InvokeArgument>& argument_list)const override
			{
				//	引数数チェック
				if (static_cast<std::uint8_t>(argument_list.size()) > GetFunctionParamLength())
				{
					NOX_ASSERT(false, U"引数が一致しません");
					return result;
				}

				//	戻り値のチェック
				if (result != nullptr && result_type.IsConvertible(result_type_) == false)
				{
					NOX_ASSERT(false, U"戻り値の型が一致しません");
					return result;
				}

				const auto apply_lambda =
					[this, &result, &result_type, &argument_list]<class _F>(_F func)constexpr
				{
					if (std::tuple_size_v<FunctionArgsType<_F>> == argument_list.size())
					{
						if constexpr (std::is_void_v< FunctionReturnType<_F>> == true)
						{
							FunctionInfo::TryInvokeImpl_Private(func, argument_list, std::make_index_sequence<std::tuple_size_v<FunctionArgsType<_F>>>());
						}
						else
						{
							if (result != nullptr)
							{
								*reinterpret_cast<FunctionReturnType<_F>*>(result) =
									FunctionInfo::TryInvokeImpl_Private(func, argument_list, std::make_index_sequence<std::tuple_size_v<FunctionArgsType<_F>>>());
							}
							else
							{
								FunctionInfo::TryInvokeImpl_Private(func, argument_list, std::make_index_sequence<std::tuple_size_v<FunctionArgsType<_F>>>());
							}
						}

						return true;
					}
					return false;
				};


				bool is_success = false;
				std::apply(
					[&apply_lambda, &is_success]<class... Funcs>(Funcs... funcs)
				{
					if ((apply_lambda(funcs) || ...))
					{
						is_success = true;
						return;
					}
				},
					functions_);

				NOX_ASSERT(is_success, U"関数呼び出しに失敗しました");
				return result;
			}



		private:
			/// @brief 関数リスト
			std::tuple<_Functions...> functions_;
		};

	/*	template<class T>
		inline	consteval	FunctionArgumentInfo	CreateFunctionParameter(
			ReflectionStringView name,
			std::span<ReflectionObject> attribute_list,
			bool hasDefaultValue)noexcept
		{
			return FunctionArgumentInfo(name, attribute_ptr_table, attribute_length, reflection::Typeof<T>(), hasDefaultValue);
		}*/

		template<class RawFunction, class... _Functions>
		inline	constexpr	auto	CreateFunctionInfo(
			ReflectionStringView	name,
			ReflectionStringView	fullname,
			ReflectionStringView	_namespace,
			AccessLevel access_level,
			const std::reference_wrapper<const ReflectionObject>* attribute_list,
			const std::uint8_t attribute_list_length,
			const std::reference_wrapper<const FunctionArgumentInfo>* function_param_list,
			const std::uint8_t function_param_list_length,
			bool is_constructor,
			bool is_constexpr,
			bool is_inline,
			_Functions... functions
		)noexcept
		{
			//	c++で解決できないものは、ここで解決する
			FunctionAttributeFlag method_attribute_flags = GetFunctionAttributeFlags<RawFunction>();
			if (is_constexpr == true)
			{
				method_attribute_flags = util::BitOr(method_attribute_flags, FunctionAttributeFlag::Constexpr);
			}
			if (is_inline == true)
			{
				method_attribute_flags = util::BitOr(method_attribute_flags, FunctionAttributeFlag::Inline);
			}

			if constexpr (std::is_member_function_pointer_v<RawFunction> == true)
			{
				return FunctionInfoImpl(
					name,
					fullname,
					_namespace,
					attribute_list,
					attribute_list_length,
					function_param_list,
					function_param_list_length,
					reflection::Typeof<FunctionClassType<RawFunction>>(),
					reflection::Typeof<FunctionReturnType<RawFunction>>(),
					access_level,
					is_constructor == true ? FunctionType::Constructor : FunctionType::Default,
					method_attribute_flags,
					functions...
				);
			}
			else
			{
				return FunctionInfoImpl(
					name,
					fullname,
					_namespace,
					attribute_list,
					attribute_list_length,
					function_param_list,
					function_param_list_length,

					reflection::InvalidType,
					reflection::Typeof<FunctionReturnType<RawFunction>>(),
					access_level,
					is_constructor == true ? FunctionType::Constructor : FunctionType::Default,
					method_attribute_flags,
					functions...
				);
			}
			
		}
	}
}