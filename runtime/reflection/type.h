///	@file	type.h
///	@brief	type
#pragma once

#include	"utility.h"
#include	<string_view>

namespace nox::reflection
{
	//	前方宣言
	class Type;

	namespace detail
	{
		/// @brief Type生成構造体
		struct ReflectionTypeActivator;

		struct TypeExtraResultInfo
		{
			//	8byte
			union
			{
				/**
				 * @brief ダミー
				*/
				std::uintptr_t ret0 = 0;

				/**
				 * @brief 配列の次元数
				*/
				std::uint32_t array_rank;

				/**
				 * @brief 配列サイズ
				*/
				std::uint32_t array_extent;

				/**
				 * @brief インスタンス
				*/
				void* instance_ptr;

				/**
				 * @brief 基底型のタイプ種別
				*/
				TypeKind underlying_type_kind;

				/**
				 * @brief 要素タイプ
				*/
				const class reflection::Type* extra_type_ptr;
			};
		};
		static_assert(sizeof(TypeExtraResultInfo) <= sizeof(std::uintptr_t));

		enum class TypeExtraAccessInfoType : u8
		{
			/**
			 * @brief 配列の次元数
			*/
			ArrayRank,

			/**
			 * @brief 配列の要素数
			*/
			ArrayExtent,

			/**
			 * @brief インスタンス作成
			*/
			CreateInstance,

			/**
			 * @brief 基底型
			*/
			UnderlyingTypeKind,

			/// @brief ポインタの指す型
			PointeeType,

			DesugarType,

			/**
			 * @brief その他の型情報
			*/
			ExtraType,
		};

		/// @brief 動的情報呼び出しの引数パラメータ
		struct TypeExtraArgsInfo
		{
			/// @brief 呼び出し内容
			TypeExtraAccessInfoType request_type;

			//	8byte
			union
			{
				/// @brief ダミー
				const std::uint64_t arg0 = 0;

				/// @brief 配列の次元数
				const std::uint32_t array_rank_index;
			};
		};

		/// @brief 動的情報を取得するための関数型
		using TypeExtraInfoFuncType = bool(*)(TypeExtraResultInfo& outInfo, const TypeExtraArgsInfo&)noexcept;

		/// @brief 動的情報を取得する関数の宣言
		/// @tparam T 型
		/// @param out_info 戻り値
		/// @param args 引数パラメータ
		/// @return 成功したか
		template<class T>
		bool GetTypeExtraInfo(TypeExtraResultInfo& out_info, const TypeExtraArgsInfo& args)noexcept;
	}

	/// @brief 汎用型情報
	class Type final
	{
		friend struct nox::reflection::detail::ReflectionTypeActivator;

	private:
		[[nodiscard]] inline constexpr explicit Type(
			const std::uint32_t _id,
			const TypeKind _kind,
			const TypeAttributeFlag _attribute_flags,
			const size_t _size,
			const size_t _alignment,
			not_null<reflection::detail::TypeExtraInfoFuncType> extra_func,
			const std::u8string_view name
		)noexcept :
			id_(_id),
			kind_(_kind),
			attribute_flags_(_attribute_flags),
			size_(_size),
			alignment_(_alignment),
			extra_func_(extra_func.get()),
			name_(name)
		{}
	public:

	//	[[nodiscard]] inline constexpr Type(const Type&)noexcept = delete;
	//	[[nodiscard]] inline constexpr Type(const Type&&)noexcept = delete;

		/// @brief 等価比較
		inline	constexpr	bool	Equal(const Type& type)const noexcept { return id_ == type.id_; }

		/// @brief 変換可能かどうか
		/// @param to 
		/// @return 
		bool	IsConvertible(const Type& to)const noexcept;

		inline constexpr const Type& GetPointeeType()const noexcept 
		{
			reflection::detail::TypeExtraResultInfo result;
			std::invoke(extra_func_, result, reflection::detail::TypeExtraArgsInfo{ .request_type = reflection::detail::TypeExtraAccessInfoType::PointeeType });
			return util::Deref(result.extra_type_ptr);
		}
		inline constexpr const Type& GetDesugarType()const noexcept
		{
			reflection::detail::TypeExtraResultInfo result;
			std::invoke(extra_func_, result, reflection::detail::TypeExtraArgsInfo{ .request_type = reflection::detail::TypeExtraAccessInfoType::DesugarType });
			return util::Deref(result.extra_type_ptr);
		}

#pragma region アクセサ
		/// @brief 型IDを取得
		[[nodiscard]] inline	constexpr	std::uint32_t GetTypeID()const noexcept { return id_; }

		/// @brief 型の名前を取得
		[[nodiscard]] inline	constexpr std::u8string_view GetTypeName()const noexcept { return name_; }

		/// @brief 型のサイズを取得
		[[nodiscard]] inline	constexpr std::size_t GetTypeSize()const noexcept { return size_; }

		/// @brief アライメントを取得
		[[nodiscard]] inline	constexpr std::size_t GetAlignmentOf()const noexcept { return alignment_; }

		/// @brief タイプ識別を取得
		[[nodiscard]] inline	constexpr TypeKind	GetTypeKind()const noexcept { return kind_; }

		/// @brief タイプ属性を取得
		[[nodiscard]] inline	constexpr TypeAttributeFlag GetTypeAttributeFlags()const noexcept { return attribute_flags_; }

		/// @brief タイプ属性を保持しているかチェック
		/// @param flag タイプ属性
		/// @return 保持しているかどうか
		[[nodiscard]] inline	constexpr bool	IsTypeAttributeFlag(const TypeAttributeFlag flag)const noexcept { return util::IsBitAnd(attribute_flags_, flag); }
#pragma endregion

#pragma region 型の特性
		[[nodiscard]] inline	constexpr	bool IsClass()const noexcept { return kind_ == TypeKind::Class; }
		[[nodiscard]] inline	constexpr	bool IsUnion()const noexcept { return kind_ == TypeKind::Union; }
		[[nodiscard]] inline	constexpr	bool IsFloating()const noexcept { return kind_ == TypeKind::F32 || kind_ == TypeKind::F64; }
		[[nodiscard]] inline	constexpr	bool IsEnum()const noexcept { return kind_ == TypeKind::Enum || kind_ == TypeKind::ScopedEnum; }
		[[nodiscard]] inline	constexpr	bool IsScopedEnum()const noexcept { return kind_ == TypeKind::ScopedEnum; }
		[[nodiscard]] inline	constexpr	bool	IsChar8()const noexcept { return kind_ == TypeKind::Char8; }

#pragma endregion

#pragma region Qualifier
		[[nodiscard]]	inline	constexpr	bool	IsConstQualified()const noexcept { return util::IsBitAnd(attribute_flags_, TypeAttributeFlag::Const); }
#pragma endregion

#pragma region operator
	//	[[nodiscard]] inline constexpr bool operator =(const Type&)noexcept = delete;

		[[nodiscard]] inline constexpr bool operator ==(const Type& type)const noexcept { return Equal(type); }

		[[nodiscard]] inline constexpr bool operator !=(const Type& type)const noexcept { return !Equal(type); }
#pragma endregion


	protected:
		/// @brief 型ID
		std::uint32_t id_;

		/// @brief 型の識別
		TypeKind kind_;

		/// @brief 型属性
		TypeAttributeFlag attribute_flags_;

		/// @brief 型のサイズ
		size_t size_;

		/// @brief アライメントオフ
		size_t alignment_;

		/// @brief 動的情報取得関数
		detail::TypeExtraInfoFuncType extra_func_;

		/// @brief		型名
		/// @details	コンパイル時に判断された名前なので、開発ビルド以外では使用禁止
		std::u8string_view name_;
	};
	
	namespace detail
	{
		struct ReflectionTypeActivator
		{
			/// @brief 無効タイプ
			static inline	constexpr	Type CreateReflectionInvalidType()noexcept 
			{ 
				return Type(
				0U,
				TypeKind::Invalid,
				TypeAttributeFlag::None,
				0U,
				0U,
					+[](detail::TypeExtraResultInfo&, const detail::TypeExtraArgsInfo&)noexcept {
						return false;
					},
				u8""
			); }

			template<class T>
			static inline	constexpr	Type CreateReflectionType()noexcept
			{
				return Type(
					util::GetUniqueTypeID<T>(),
					nox::reflection::GetTypeKind<T>(),
					nox::reflection::GetTypeAttributeFlags<T>(),
					IsSizeofTypeValue<T> ? sizeof(T) : 0,
					std::alignment_of_v<T>,
					+[](detail::TypeExtraResultInfo& out_info, const detail::TypeExtraArgsInfo& args)noexcept {
						//memo:	複雑すぎてコンパイルエラーになるので、実行時処理に回避
						return reflection::detail::GetTypeExtraInfo<T>(out_info, args);
					},
					util::GetTypeName<T>()
				);
			}
		};	

		/// @brief 無効型
		constexpr Type InvalidTypeImpl{ reflection::detail::ReflectionTypeActivator::CreateReflectionInvalidType() };

		/// @brief 型情報保持構造体
		/// @tparam T 型
		template<class T>
		struct ReflectionTypeHolder
		{
			static constexpr Type value{ reflection::detail::ReflectionTypeActivator::CreateReflectionType<T>() };
		};

		template<class T>
		bool GetTypeExtraInfo(TypeExtraResultInfo& out_info, const TypeExtraArgsInfo& args)noexcept
		{
			switch (args.request_type)
			{
			case TypeExtraAccessInfoType::ArrayRank:
				if constexpr (std::is_array_v<T> == false)
				{
					out_info.array_rank = 0U;
					return true;
				}
				else
				{
					out_info.array_rank = static_cast<u32>(std::rank_v<T>);
					return true;
				}
				break;
			case TypeExtraAccessInfoType::ArrayExtent:
				if constexpr (std::is_array_v<T> == false)
				{
					out_info.array_extent = 0U;
					return true;
				}
				else
				{
					out_info.array_extent = nox::util::GetArrayExtent<T>(args.array_rank_index);
					return true;
				}

			case TypeExtraAccessInfoType::CreateInstance:
				if constexpr (std::is_class_v<T> == true)
				{
					//	out_info.instance_ptr = TypeExtraCreateInstance<std::remove_const_t<T>>::Create();
					return true;
				}
				else
				{
					out_info.instance_ptr = nullptr;
				}

			case TypeExtraAccessInfoType::UnderlyingTypeKind:
				if constexpr (std::is_enum_v<T> == true)
				{
					out_info.underlying_type_kind = GetTypeKind<std::underlying_type_t<T>>();
					return true;
				}
				return false;
			case TypeExtraAccessInfoType::PointeeType:
				if constexpr (std::is_pointer_v<T> == true)
				{
					out_info.extra_type_ptr = &ReflectionTypeHolder<std::remove_pointer_t<T>>::value;
				}
				else
				{
					out_info.extra_type_ptr = &reflection::detail::InvalidTypeImpl;
				}
				return true;

			case TypeExtraAccessInfoType::DesugarType:
				if constexpr (std::is_const_v<T> == true)
				{
					out_info.extra_type_ptr = &ReflectionTypeHolder<std::remove_const_t<T>>::value;
				}
				else
				{
					out_info.extra_type_ptr = &reflection::detail::InvalidTypeImpl;
				}
				return true;
			}

			return false;
		}
	}

	/// @brief 無効型
	constexpr const Type& InvalidType = reflection::detail::InvalidTypeImpl;

	/// @brief 型情報を取得
	/// @tparam T 型
	/// @return 型情報
	template<class T>
	[[nodiscard]] inline	consteval const Type& Typeof()noexcept
	{
		return reflection::detail::ReflectionTypeHolder<T>::value;
	}

#pragma region 関数群
	/// @brief 変換可能か
	/// @param a 
	/// @param b 
	/// @return 
	bool	IsConvertible(const Type& a, const Type& b);
#pragma endregion

}