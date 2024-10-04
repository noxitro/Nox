///	@file	type.h
///	@brief	type
#pragma once
#include	<string_view>
#include	<span>
#include	<typeinfo>
#include	"utility.h"
#include	"attribute.h"
namespace nox::reflection
{
	//	前方宣言
	class Type;

	namespace detail
	{
		/// @brief Type生成構造体
		struct ReflectionTypeActivator;

		/// @brief ポインタ、参照が指す型を取得
		template<class T>
		inline constexpr const Type& GetPointeeType()noexcept;
		/// @brief 関数の戻り値の型を取得
		template<class T>
		inline constexpr const Type& GetResultType()noexcept;
		/// @brief 配列型から次元を除去した型を取得
		template<class T>
		inline constexpr const Type& GetRemoveExtentType()noexcept;
		/// @brief 配列型から全ての次元を除去した型を取得
		template<class T>
		inline constexpr const Type& GetRemoveAllExtentType()noexcept;
		/// @brief 基底型を取得
		template<class T>
		inline constexpr const Type& GetUnderlyingType()noexcept;
		/// @brief const修飾した型を取得
		template<class T>
		inline constexpr const Type& GetAddConstType()noexcept;
		/// @brief const を取り除いた型を取得
		template<class T>
		inline constexpr const Type& GetRemoveConstType()noexcept;
		/// @brief volatile修飾した型を取得
		template<class T>
		inline constexpr const Type& GetAddVolatileType()noexcept;
		/// @brief volatile を取り除いた型を取得
		template<class T>
		inline constexpr const Type& GetRemoveVolatileType()noexcept;
		/// @brief 左辺参照型を取得
		template<class T>
		inline constexpr const Type& GetAddLValueReferenceType()noexcept;
		/// @brief 右辺参照型を取得
		template<class T>
		inline constexpr const Type& GetAddRValueReferenceType()noexcept;
		/// @brief 全ての修飾子を取り除いた型を取得
		template<class T>
		inline constexpr const Type& GetRemoveAllModifiersType()noexcept;

		template<nox::concepts::FunctionSignatureType T>
		inline constexpr std::array<std::reference_wrapper<const nox::reflection::Type>, nox::FunctionArgsLength<T>> GetArgumentTypeList()noexcept;

		template<class T>
		inline constexpr const Type& GetOwnerType()noexcept;
	}

	/// @brief 汎用型情報
	class Type
	{
		friend struct nox::reflection::detail::ReflectionTypeActivator;

	protected:
		[[nodiscard]] inline constexpr explicit Type(
			const std::uint32_t _id,					//	0
			const nox::TypeId& _typeid,					//	1
			const TypeKind _kind,						//	2		
			const TypeQualifierFlag _attribute_flags,	//	3
			const std::uint32_t _size,					//	4
			const std::uint32_t _alignment,				//	5
			const std::uint16_t array_rank,				//	6
			const std::uint32_t array_extent,			//	7
			const std::string_view name,				//	8	
			const Type& pointee_type,					//	9	
			const Type& result_type,					//	10
			const Type& remove_element_type,			//	11
			const Type& remove_all_element_type,		//	12
			const Type& underlying_type,				//	13
			const Type& add_const_type,					//	14
			const Type& remove_const_type,				//	15
			const Type& add_volatile_type,				//	16
			const Type& remove_volatile_type,			//	17
			const Type& add_lvalue_reference_type,		//	18
			const Type& add_rvalue_reference_type,		//	19
			const Type& remove_all_modifiers_type,		//	20
			const Type& owner_type,						//	21
			const std::uint8_t argument_length			//	22
		)noexcept :
			id_(_id),
			typeid_(_typeid),
			kind_(_kind),
			attribute_flags_(_attribute_flags),
			size_(_size),
			alignment_(_alignment),
			array_rank_(array_rank),
			array_extent_(array_extent),
			name_(name),
			pointee_type_(pointee_type),
			result_type_(result_type),
			remove_element_type_(remove_element_type),
			remove_all_element_type_(remove_all_element_type),
			underlying_type_(underlying_type),
			add_const_type_(add_const_type),
			remove_const_type_(remove_const_type),
			add_volatile_type_(add_volatile_type),
			remove_volatile_type_(remove_volatile_type),
			add_lvalue_reference_type_(add_lvalue_reference_type),
			add_rvalue_reference_type_(add_rvalue_reference_type),
			remove_all_modifiers_type_(remove_all_modifiers_type),
			owner_type_(owner_type),
			argument_length_(argument_length)
		{}
	public:

		
		/// @brief toへ変換可能かどうか
		inline constexpr bool	IsConvertible(const Type& to)const noexcept
		{
			if (*this == to)
			{
				return true;
			}

			//	修飾子を取り除いた型が一致するか
			if (GetRemoveConstType() == to)
			{
				return true;
			}

			//	変換先が参照型の場合
			if (to.IsReference() == true)
			{
				const Type& toPointeeType = to.GetPointeeType();
				if (*this == toPointeeType || GetRemoveConstType() == toPointeeType)
				{
					return true;
				}
			}

			//	参照型の場合、参照を取り除いた型でチェックする
			if (IsReference() == true)
			{
				if (pointee_type_ == to || pointee_type_ == to.GetRemoveConstType())
				{
					return true;
				}

				//	変換先が参照型の場合
				if (to.IsReference() == true)
				{
					const Type& toPointeeType = to.GetPointeeType();
					if (pointee_type_ == toPointeeType || pointee_type_ == toPointeeType.GetRemoveConstType())
					{
						return true;
					}
				}
			}
			//	ポインタ型の場合
			else
			if (IsPointer() == true && to.IsPointer() == true)
			{
				if (pointee_type_ == to.pointee_type_.GetRemoveConstType())
				{
					return true;
				}
			}

			return false;
		}

		/// @brief 変換可能かどうか
		template<class T>
		inline constexpr bool	IsConvertible()const noexcept;

#pragma region アクセサ
		/// @brief 型IDを取得
		[[nodiscard]] inline	constexpr	std::uint32_t GetTypeID()const noexcept { return id_; }

		/// @brief 型の名前を取得
		[[nodiscard]] inline	constexpr std::string_view GetTypeName()const noexcept { return name_; }

		/// @brief 型のサイズを取得
		[[nodiscard]] inline	constexpr std::uint32_t GetTypeSize()const noexcept { return size_; }

		/// @brief アライメントを取得
		[[nodiscard]] inline	constexpr std::uint32_t GetAlignmentOf()const noexcept { return alignment_; }

		/// @brief タイプ識別を取得
		[[nodiscard]] inline	constexpr TypeKind	GetTypeKind()const noexcept { return kind_; }

		/// @brief タイプ属性を取得
		[[nodiscard]] inline	constexpr TypeQualifierFlag GetTypeAttributeFlags()const noexcept { return attribute_flags_; }

		/// @brief 配列の次元数を取得
		[[nodiscard]] inline constexpr std::uint16_t GetArrayRank()const noexcept { return array_rank_; }

		/// @brief 配列の要素数を取得
		[[nodiscard]] inline constexpr std::uint32_t GetArrayExtent() const noexcept { return array_extent_; }

		/// @brief タイプ属性を保持しているかチェック
		/// @param flag タイプ属性
		/// @return 保持しているかどうか
		[[nodiscard]] inline	constexpr bool	IsTypeAttributeFlag(const TypeQualifierFlag flag)const noexcept { return util::IsBitAnd(attribute_flags_, flag); }

		[[nodiscard]] inline constexpr const Type& GetPointeeType()const noexcept { return pointee_type_; }
		/// @brief 配列型から次元を除去した型を取得
		[[nodiscard]] inline constexpr const Type& GetRemoveExtentType()const noexcept { return remove_element_type_; }

		/// @brief 配列型から全ての次元を除去した型を取得
		[[nodiscard]] inline constexpr const Type& GetRemoveAllExtentType()const noexcept { return remove_all_element_type_; }

		/// @brief 基底型を取得
		[[nodiscard]] inline constexpr const Type& GetUnderlyingType()const noexcept { return underlying_type_; }

		/// @brief const修飾した型を取得
		[[nodiscard]] inline constexpr const Type& GetAddConstType()const noexcept { return add_const_type_; }

		/// @brief const を取り除いた型を取得
		[[nodiscard]] inline constexpr const Type& GetRemoveConstType()const noexcept { return remove_const_type_; }

		/// @brief volatile修飾した型を取得
		[[nodiscard]] inline constexpr const Type& GetAddVolatileType()const noexcept { return add_volatile_type_; }

		/// @brief volatile を取り除いた型を取得
		[[nodiscard]] inline constexpr const Type& GetRemoveVolatileType()const noexcept { return remove_volatile_type_; }

		/// @brief 左辺参照型を取得
		[[nodiscard]] inline constexpr const Type& GetAddLValueReferenceType()const noexcept { return add_lvalue_reference_type_; }

		/// @brief 右辺参照型を取得
		[[nodiscard]] inline constexpr const Type& GetAddRValueReferenceType()const noexcept { return add_rvalue_reference_type_; }

		/// @brief 全ての修飾子を取り除いた型を取得
		[[nodiscard]] inline constexpr const Type& GetRemoveAllModifiersType()const noexcept { return remove_all_modifiers_type_; }

		/// @brief オーナー型を取得
		[[nodiscard]] inline constexpr const Type& GetOwnerType()const noexcept { return owner_type_; }

		/// @brief 指定次元の配列の要素数を取得
		/// @param index 次元数
		/// @return 要素数
		[[nodiscard]] inline constexpr std::uint32_t GetArrayExtent(std::uint32_t index) const noexcept
		{
			if (index >= array_rank_)
			{
				return 0;
			}

			const Type* type = &remove_element_type_;
#pragma warning(push)
#pragma warning(disable: 4296)
			for (; index >= 0; --index, type = &type->remove_element_type_)
			{
				if (type->IsBoundedArray() == false)
				{
					return 0;
				}
			}
#pragma warning(pop)

			return type->array_extent_;
		}

		/// @brief 関数の戻り値の型を取得
		[[nodiscard]] inline constexpr const Type& GetResultType()const noexcept { return result_type_; }

		[[nodiscard]] inline constexpr std::uint8_t GetArgumentLength()const noexcept { return argument_length_; }
		[[nodiscard]] inline constexpr const Type& GetArgumentType(const std::uint32_t index)const noexcept
		{
			if (index < argument_length_)
			{
				return GetArgumentTypeList()[index];
			}
			return GetInvalidType();
		}

		[[nodiscard]] inline constexpr bool IsValid()const noexcept { return id_ != 0; }

		//	リフレクション実装から取得する
		[[nodiscard]] const class UserDefinedCompoundTypeInfo* GetUserDefinedCompoundTypeInfo()const noexcept;
		[[nodiscard]] const class EnumInfo* GetEnumInfo()const noexcept;
#pragma region virtual
		/// @brief 関数の引数型情報リストを取得
		[[nodiscard]] inline constexpr virtual std::span<const std::reference_wrapper<const nox::reflection::Type>> GetArgumentTypeList()const noexcept { return {}; }

		/// @brief new演算子でオブジェクトを生成
		/// @details	修飾子が付与された型の場合、修飾子を除いた型で生成する
		/// @return 成功した場合はオブジェクトのポインタ、失敗した場合はnullptr
		[[nodiscard]] inline constexpr virtual void* CreateObject()const noexcept { return nullptr; }

		
		/// @brief 配置new演算子でオブジェクトを生成
		/// @param buffer 配置先のバッファ
		/// @details	修飾子が付与された型の場合、修飾子を除いた型で生成する
		/// @return 成功した場合はオブジェクトのポインタ、失敗した場合はnullptr
		[[nodiscard]] inline constexpr void* CreateObject(std::span<std::uint8_t> buffer)const noexcept 
		{ 
			if (buffer.size() >= size_)
			{
				return CreateObject(buffer.data());
			}
			else
			{
				return nullptr;
			}
		}
#pragma endregion


#pragma endregion

#pragma region 型の特性
		[[nodiscard]] inline	constexpr	bool IsClass()const noexcept { return kind_ == TypeKind::Class; }
		[[nodiscard]] inline	constexpr	bool IsUnion()const noexcept { return kind_ == TypeKind::Union; }
		[[nodiscard]] inline	constexpr	bool IsFloating()const noexcept { return kind_ == TypeKind::Float || kind_ == TypeKind::Double; }
		[[nodiscard]] inline	constexpr	bool IsEnum()const noexcept { return kind_ == TypeKind::Enum || kind_ == TypeKind::ScopedEnum; }
		[[nodiscard]] inline	constexpr	bool IsScopedEnum()const noexcept { return kind_ == TypeKind::ScopedEnum; }
		[[nodiscard]] inline	constexpr	bool	IsChar8()const noexcept { return kind_ == TypeKind::Char8; }
		[[nodiscard]] inline	constexpr	bool	IsArray()const noexcept { return IsBoundedArray() || IsUnboundedArray(); }
		[[nodiscard]] inline	constexpr	bool	IsBoundedArray()const noexcept { return kind_ == TypeKind::Array; }
		[[nodiscard]] inline	constexpr	bool	IsUnboundedArray()const noexcept { return kind_ == TypeKind::UnboundedArray; }

#pragma endregion

#pragma region Qualifier
		[[nodiscard]]	inline	constexpr	bool	IsConstQualified()const noexcept { return nox::util::IsBitAnd(attribute_flags_, TypeQualifierFlag::Const); }
		[[nodiscard]] inline	constexpr	bool	IsPointer()const noexcept { return kind_ == TypeKind::Pointer; }
		[[nodiscard]]	inline	constexpr	bool	IsReference()const noexcept { return IsLValueReference() || IsRValueReference(); }
		[[nodiscard]] inline	constexpr	bool	IsLValueReference()const noexcept { return kind_ == TypeKind::LvalueReference; }
		[[nodiscard]] inline	constexpr	bool	IsRValueReference()const noexcept { return kind_ == TypeKind::RvalueReference; }
#pragma endregion

#pragma region operator
	//	[[nodiscard]] inline constexpr bool operator =(const Type&)noexcept = delete;

		[[nodiscard]] inline constexpr bool operator ==(const Type& type)const noexcept { return &typeid_ == &type.typeid_; }

	//	[[nodiscard]] inline constexpr bool operator !=(const Type& type)const noexcept { return !Equal(type); }
#pragma endregion

		protected:
			/// @brief 配置new演算子でオブジェクトを生成
			/// @param buffer 
			/// @return 
			[[nodiscard]] inline constexpr virtual void* CreateObject(void* /*buffer*/)const noexcept { return nullptr; }

		private:
			static inline constexpr const Type& GetInvalidType()noexcept;

	protected:
		/// @brief 関数の引数の数
		const std::uint8_t argument_length_;
	
		/// @brief 型の識別
		const TypeKind kind_;

		/// @brief 型属性
		const TypeQualifierFlag attribute_flags_;

		/// @brief 配列の次元数
		const std::uint16_t array_rank_;

		/// @brief 型ID
		const std::uint32_t id_;

		const std::uint32_t array_extent_;

		/// @brief 型のサイズ
		const std::uint32_t size_;

		/// @brief アライメントオフ
		const std::uint32_t alignment_;

		const nox::TypeId& typeid_;

		/// @brief		型名
		/// @details	コンパイル時に判断された名前なので、開発ビルド以外では使用禁止
		const std::string_view name_;

		const Type& pointee_type_;
		const Type& result_type_;
		const Type& remove_element_type_;
		const Type& remove_all_element_type_;
		const Type& underlying_type_;
		const Type& add_const_type_;
		const Type& remove_const_type_;
		const Type& add_volatile_type_;
		const Type& remove_volatile_type_;
		const Type& add_lvalue_reference_type_;
		const Type& add_rvalue_reference_type_;
		const Type& remove_all_modifiers_type_;
		const Type& owner_type_;

		//const Type* const* const argument_type_table_;
	
	};

	namespace detail
	{
		/// @brief コンパイル時無効型
		class CompileTimeInvalidType final: public Type
		{
			friend struct nox::reflection::detail::ReflectionTypeActivator;
		private:
			inline constexpr CompileTimeInvalidType()noexcept :
				Type(
					0,
					nox::GetInvalidTypeId(),
					TypeKind::Invalid,
					TypeQualifierFlag::None,
					0,
					0,
					0,
					0,
					"",
					*this,
					*this,
					*this,
					*this,
					*this,
					*this,
					*this,
					*this,
					*this,
					*this,
					*this,
					*this,
					*this,
					0
				)
			{}
		};

		template<class T>
		class CompileTimeTypeImpl : public Type
		{
			friend struct nox::reflection::detail::ReflectionTypeActivator;

			inline static consteval std::uint32_t SafeSizeof()noexcept
			{
				if constexpr (nox::IsSizeofTypeValue<T>)
				{
					return sizeof(T);
				}
				else
				{
					return 0;
				}
			}

			inline static consteval std::uint32_t SafeAlignmentOf()noexcept
			{
				if constexpr (nox::IsSizeofTypeValue<T>)
				{
					return std::alignment_of_v<T>;
				}
				else
				{
					return 0;
				}
			}
		protected:
			inline constexpr explicit CompileTimeTypeImpl(
				const std::uint8_t argument_length
			)noexcept :
				Type(
					nox::util::GetUniqueTypeID<T>(),						//	0
					nox::GetTypeId<T>(),										//	1	
					nox::reflection::GetTypeKind<T>(),						//	2	
					nox::reflection::GetTypeAttributeFlags<T>(),			//	3
					SafeSizeof(),											//	4
					SafeAlignmentOf(),										//	5
					std::rank_v<T>,											//	6
					std::extent_v<T>,										//	7
					nox::util::GetTypeName<T>(),							//	8
					nox::reflection::detail::GetPointeeType<T>(),			//	9
					nox::reflection::detail::GetResultType<T>(),			//	10
					nox::reflection::detail::GetRemoveExtentType<T>(),		//	11
					nox::reflection::detail::GetRemoveAllExtentType<T>(),	//	12
					nox::reflection::detail::GetUnderlyingType<T>(),		//	13
					nox::reflection::detail::GetAddConstType<T>(),			//	14
					nox::reflection::detail::GetRemoveConstType<T>(),		//	15
					nox::reflection::detail::GetAddVolatileType<T>(),		//	16
					nox::reflection::detail::GetRemoveVolatileType<T>(),	//	17
					nox::reflection::detail::GetAddLValueReferenceType<T>(),	//	18
					nox::reflection::detail::GetAddRValueReferenceType<T>(),	//	19
					nox::reflection::detail::GetRemoveAllModifiersType<T>(),	//	20
					nox::reflection::detail::GetOwnerType<T>(),			//	21
					argument_length											//	22
				) 
			{}										

			inline constexpr CompileTimeTypeImpl()noexcept :
				CompileTimeTypeImpl(
					0
				)
			{}
		public:
#pragma region override
			[[nodiscard]] inline constexpr void* CreateObject()const noexcept override final
			{
				if constexpr (std::is_constructible_v<T> == true)
				{
					return new std::remove_cvref_t<T>();
				}
				else 
				{
					return nullptr;
				}
			}

			[[nodiscard]] inline constexpr void* CreateObject(void* buffer)const noexcept override final
			{
				if constexpr (std::is_constructible_v<T> == true)
				{
					return static_cast<void*>(std::construct_at(static_cast<std::remove_cvref_t<T>*>(buffer)));
				}
				else
				{
					return nullptr;
				}
			}
#pragma endregion

		private:
		};

		template<nox::concepts::FunctionSignatureType T>
		class CompileTimeTypeFunction : public CompileTimeTypeImpl<T>
		{
			friend struct nox::reflection::detail::ReflectionTypeActivator;
		private:
			inline constexpr CompileTimeTypeFunction()noexcept:
				CompileTimeTypeImpl<T>(nox::FunctionArgsLength<T>),
				argument_type_table_(nox::reflection::detail::GetArgumentTypeList<T>())
			{}
		public:
			inline constexpr std::span<const std::reference_wrapper<const nox::reflection::Type>> GetArgumentTypeList()const noexcept override {
				return std::span(argument_type_table_.data(), argument_type_table_.size());
			}
		private:
			const std::array<std::reference_wrapper<const nox::reflection::Type>, nox::FunctionArgsLength<T>> argument_type_table_;
		};
	}

	namespace detail
	{
		/// @brief		型生成構造体
		/// @details	型情報を生成するための構造体
		struct ReflectionTypeActivator
		{
			/// @brief 無効タイプ
			static inline	constexpr	nox::reflection::detail::CompileTimeInvalidType CreateReflectionInvalidType()noexcept
			{ 
				return nox::reflection::detail::CompileTimeInvalidType(); 
			}

			template<class T>
			static inline	constexpr	nox::reflection::detail::CompileTimeTypeImpl<T> CreateReflectionType()noexcept
			{
				return nox::reflection::detail::CompileTimeTypeImpl<T>();
			}

			template<nox::concepts::FunctionSignatureType T>
			static inline	constexpr	nox::reflection::detail::CompileTimeTypeFunction<T> CreateTypeFunction()noexcept
			{
//				constexpr std::array<const Type*, nox::FunctionArgsLength<T>> argument_type_table = nox::reflection::detail::GetArgumentTypeList<T>();
				return nox::reflection::detail::CompileTimeTypeFunction<T>();
			}
		};
	}

	/// @brief 無効型
	constexpr nox::reflection::detail::CompileTimeInvalidType InvalidType = reflection::detail::ReflectionTypeActivator::CreateReflectionInvalidType();

	namespace detail
	{
		template<class T>
		struct ReflectionTypeHolder;

		/// @brief 型情報保持構造体
		template<class T>
		struct ReflectionTypeHolder
		{
			static constexpr nox::reflection::detail::CompileTimeTypeImpl<T> value = reflection::detail::ReflectionTypeActivator::CreateReflectionType<T>();
		};

		template<nox::concepts::FunctionSignatureType T>
		struct ReflectionTypeHolder<T>
		{
			static constexpr nox::reflection::detail::CompileTimeTypeFunction<T> value = reflection::detail::ReflectionTypeActivator::CreateTypeFunction<T>();
		};
	}

	/// @brief 型情報を取得
	/// @tparam T 型
	/// @return 型情報
	template<class T>
	[[nodiscard]] inline	constexpr const Type& Typeof()noexcept
	{
		return reflection::detail::ReflectionTypeHolder<T>::value;
	}
#pragma region 関数群
#pragma endregion
}

#pragma region Type実装部
	template<class T>
	inline constexpr bool	nox::reflection::Type::IsConvertible()const noexcept
	{
		return nox::reflection::Type::IsConvertible(nox::reflection::Typeof<T>());
	}

	inline constexpr const nox::reflection::Type& nox::reflection::Type::GetInvalidType()noexcept
	{
		return nox::reflection::InvalidType;
	}

	template<class T>
	inline constexpr const nox::reflection::Type& nox::reflection::detail::GetPointeeType()noexcept
	{
		if constexpr (std::is_pointer_v<T>)
		{
			return nox::reflection::detail::ReflectionTypeHolder<std::remove_pointer_t<T>>::value;
		}
		else if constexpr (std::is_reference_v<T>)
		{
			return nox::reflection::detail::ReflectionTypeHolder<std::remove_reference_t<T>>::value;
		}
		else
		{
			return nox::reflection::InvalidType;
		}
	}

	template<class T>
	inline constexpr const nox::reflection::Type& nox::reflection::detail::GetResultType()noexcept
	{
		if constexpr (nox::concepts::FunctionSignatureType<T> == true)
		{
			return nox::reflection::detail::ReflectionTypeHolder<nox::FunctionReturnType<T>>::value;
		}
		else
		{
			return nox::reflection::InvalidType;
		}
	}

	template<class T>
	inline constexpr const nox::reflection::Type& nox::reflection::detail::GetRemoveExtentType()noexcept
	{
		if constexpr (std::is_array_v<T> == true)
		{
			return nox::reflection::detail::ReflectionTypeHolder<std::remove_extent_t<T>>::value;
		}
		else
		{
			return nox::reflection::InvalidType;
		}
	}

	template<class T>
	inline constexpr const nox::reflection::Type& nox::reflection::detail::GetRemoveAllExtentType()noexcept
	{
		if constexpr (std::is_unbounded_array_v<T> == true)
		{
			return reflection::detail::ReflectionTypeHolder<std::remove_all_extents_t<T>>::value;
		}
		else
		{
			return nox::reflection::InvalidType;
		}
	}

	template<class T>
	inline constexpr const nox::reflection::Type& nox::reflection::detail::GetUnderlyingType()noexcept
	{
		if constexpr (std::is_enum_v<T> == true)
		{
			return nox::reflection::detail::ReflectionTypeHolder<std::underlying_type_t<T>>::value;
		}
		else
		{
			return nox::reflection::InvalidType;
		}
	}

	template<class T>
	inline constexpr const nox::reflection::Type& nox::reflection::detail::GetAddConstType()noexcept
	{
		if constexpr (std::is_const_v<T> == false)
		{
			return nox::reflection::detail::ReflectionTypeHolder<std::add_const_t<T>>::value;
		}
		else
		{
			return nox::reflection::InvalidType;
		}
	}

	template<class T>
	inline constexpr const nox::reflection::Type& nox::reflection::detail::GetRemoveConstType()noexcept
	{
		if constexpr (std::is_const_v<T> == true)
		{
			return nox::reflection::detail::ReflectionTypeHolder<std::remove_const_t<T>>::value;
		}
		else
		{
			return nox::reflection::InvalidType;
		}
	}

	template<class T>
	inline constexpr const nox::reflection::Type& nox::reflection::detail::GetAddVolatileType()noexcept
	{
		if constexpr (std::is_volatile_v<T> == false)
		{
			return nox::reflection::detail::ReflectionTypeHolder<std::add_volatile_t<T>>::value;
		}
		else
		{
			return nox::reflection::InvalidType;
		}
	}

	template<class T>
	inline constexpr const nox::reflection::Type& nox::reflection::detail::GetRemoveVolatileType()noexcept
	{
		if constexpr (std::is_volatile_v<T> == true)
		{
			return nox::reflection::detail::ReflectionTypeHolder<std::remove_volatile_t<T>>::value;
		}
		else
		{
			return nox::reflection::InvalidType;
		}
	}

	template<class T>
	inline constexpr const nox::reflection::Type& nox::reflection::detail::GetAddLValueReferenceType()noexcept
	{
		if constexpr (std::is_lvalue_reference_v<T> == false)
		{
			return nox::reflection::detail::ReflectionTypeHolder<std::add_lvalue_reference_t<T>>::value;
		}
		else
		{
			return nox::reflection::InvalidType;
		}
	}

	template<class T>
	inline constexpr const nox::reflection::Type& nox::reflection::detail::GetAddRValueReferenceType()noexcept
	{
		if constexpr (std::is_rvalue_reference_v<T> == false)
		{
			return nox::reflection::detail::ReflectionTypeHolder<std::add_rvalue_reference_t<T>>::value;
		}
		else
		{
			return nox::reflection::InvalidType;
		}
	}

	template<class T>
	inline constexpr const nox::reflection::Type& nox::reflection::detail::GetRemoveAllModifiersType()noexcept
	{
		if constexpr (std::is_const_v<T> || std::is_volatile_v<T> || std::is_reference_v<T>)
		{
			return nox::reflection::detail::ReflectionTypeHolder<std::remove_cvref_t<T>>::value;
		}
		else
		{
			return nox::reflection::InvalidType;
		}
	}

	namespace nox::reflection::detail
	{
		template<class TupleType, std::uint32_t... Indices>
		inline constexpr std::array<std::reference_wrapper<const nox::reflection::Type>, sizeof...(Indices)> GetArgumentTypeListImpl(std::integer_sequence<std::uint32_t, Indices...>)noexcept
		{
			return { (nox::reflection::detail::ReflectionTypeHolder<std::tuple_element_t<Indices, TupleType>>::value)... };
		}
	}

	template<nox::concepts::FunctionSignatureType T>
	inline constexpr std::array<std::reference_wrapper<const nox::reflection::Type>, nox::FunctionArgsLength<T>> nox::reflection::detail::GetArgumentTypeList()noexcept
	{
		return nox::reflection::detail::GetArgumentTypeListImpl<nox::FunctionArgsType<T>>(std::make_integer_sequence<std::uint32_t, FunctionArgsLength<T>>{});
	}

	template<class T>
	inline constexpr const nox::reflection::Type& nox::reflection::detail::GetOwnerType()noexcept
	{
		if constexpr (nox::concepts::FunctionSignatureType<T> == true && std::is_member_function_pointer_v<T> == true)
		{
			return nox::reflection::detail::ReflectionTypeHolder<nox::FunctionClassType<T>>::value;
		}
		else if constexpr (nox::concepts::FieldSignatureType<T> == true && std::is_member_object_pointer_v<T> == true)
		{
			return nox::reflection::detail::ReflectionTypeHolder<nox::FieldClassType<T>>::value;
		}
		else
		{
			return nox::reflection::InvalidType;
		}
	}
#pragma endregion