///	@file	type.h
///	@brief	type
#pragma once

#include	"utility.h"
#include	<string_view>

namespace nox::reflection
{
	//	前方宣言
	namespace detail
	{
		struct ReflectionTypeActivator;
		struct ReflectionTypeActivatorImpl;
	}

	/// @brief 汎用型情報
	class Type
	{
		friend struct nox::reflection::detail::ReflectionTypeActivator;

		[[nodiscard]] inline constexpr explicit Type(
			const std::uint32_t _id,
			const TypeKind _kind,
			const TypeAttributeFlag _attribute_flags,
			const size_t _size,
			const size_t _alignment,
			const std::u8string_view name,
			const Type* pointee_type,
			const Type* desugar_type
		)noexcept :
			id_(_id),
			kind_(_kind),
			attribute_flags_(_attribute_flags),
			size_(_size),
			alignment_(_alignment),
			name_(name),
			pointee_type_(pointee_type),
			desugar_type_(desugar_type)
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


	private:
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

		/// @brief		型名
		/// @details	コンパイル時に判断された名前なので、開発ビルド以外では使用禁止
		std::u8string_view name_;

		/// @brief ポインタが指す型
		const Type* pointee_type_;

		/// @brief 糖衣構文
		const Type* desugar_type_;

	};
	
	class TypeImpl : public Type
	{
	public:

	};

	namespace detail
	{
		struct ReflectionTypeActivator
		{
			/// @brief 無効タイプ
			static inline	constexpr	Type TypeofNone()noexcept { return Type(
				0U,
				TypeKind::Invalid,
				TypeAttributeFlag::None,
				0U,
				0U,
				u8"",
				nullptr,
				nullptr
			); }

			template<class T>
			static inline	constexpr	Type TypeofImpl()noexcept
			{
				return Type(
					util::GetUniqueTypeID<T>(),
					nox::reflection::GetTypeKind<T>(),
					nox::reflection::GetTypeAttributeFlags<T>(),
					IsSizeofTypeValue<T> ? sizeof(T) : 0,
					std::alignment_of_v<T>,
					util::GetTypeName<T>(),
					nullptr,
					nullptr
				);
			}
		};	

		/// @brief 無効型
		constexpr Type InvalidTypeImpl = reflection::detail::ReflectionTypeActivator::TypeofNone();

		template<class T>
		struct ReflectionTypeHolder
		{
			static constexpr Type value = reflection::detail::ReflectionTypeActivator::TypeofImpl<T>();
		};
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