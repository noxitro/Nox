///	@file	reflection_object.h
///	@brief	reflection_object
#pragma once

namespace nox::reflection
{
	/// @brief		自動生成コード名前空間
	///	@details	直接は使用しません
	namespace gen
	{
		/**
		 * @brief リフレクション情報保持基底構造体
		*/
		struct TypeInfoRefHolderBase
		{

		};

		/**
		 * @brief リフレクション用の前方宣言
		 * @tparam T 情報を持つ型
		*/
		template<class T>  requires(std::is_class_v<T> || std::is_enum_v<T> || std::is_union_v<T>)
			struct TypeInfoRefHolder;

		/**
		 * @brief 別名定義用のリフレクション用の前方宣言
		 * @tparam T 情報を持つ型
		*/
		template<class T, std::uint8_t Index>  requires(std::is_class_v<T> || std::is_enum_v<T> || std::is_union_v<T>)
			struct TypeInfoRefTypedefHolder;

		/**
		 * @brief 名前空間に存在するリフレクション用の前方宣言
		 * @tparam Tag 名前空間文字列のハッシュ値
		*/
		template<std::uint32_t ID>
		struct GlobalRefHolder;
	}

	///	@brief		リフレクション定義
	///	@details	クラス内で定義することで、privateメンバもリフレクション対象になります
#define	NOX_REFLECTION_DECLARE(ClassType) \
private:\
	inline constexpr void __CheckStaticAssert()noexcept{ static_assert(std::is_same_v<ClassType, std::remove_cvref_t<decltype(*this)>>); }\
	friend struct ::nox::reflection::gen::TypeInfoRefHolder<ClassType>

	/// @brief		リフレクションオブジェクト定義
	/// @details	リフレクション対象となり、型IDを取得する関数が定義されます
#define NOX_DECLARE_REFLECTION_OBJECT(ClassType)\
public:\
constexpr virtual std::uint32_t GetUniqueTypeID()const noexcept{return ::nox::util::GetUniqueTypeID<ClassType>();}\
NOX_REFLECTION_DECLARE(ClassType)


	/// @brief 属性インターフェース
	class ReflectionObject
	{
		NOX_DECLARE_REFLECTION_OBJECT(ReflectionObject);
	protected:
		[[nodiscard]] inline constexpr ReflectionObject()noexcept = default;

	private:
		inline constexpr ReflectionObject(const ReflectionObject&)noexcept = delete;
		inline constexpr ReflectionObject(const ReflectionObject&&)noexcept = delete;

		inline constexpr void operator =(const ReflectionObject&)noexcept = delete;
		inline constexpr void operator =(const ReflectionObject&&)noexcept = delete;
	};

	struct IAttribute
	{
	protected:
		[[nodiscard]] inline constexpr IAttribute()noexcept = default;
	};
}