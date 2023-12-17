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

///	@brief		属性付与(テキスト形式)
///	@details	enum値など型定義が不可能なものに対して使う
///				行番号で判定されるので、必ず付与対象の上行に書いてください
#define	NOX_ATTR_TEXT(...)

/// @brief		属性定義
#if NOX_REFLECTION_GENERATOR
#define	NOX_ATTR_RAW(...)\
NOX_PP_CAT_I(class NoxReflectionAttributeContainer, __COUNTER__) \
{inline constexpr void NoxReflectionAttributeContainer(const char16_t* text = #__VA_ARGS__)const noexcept = delete;}
#else
#define	NOX_ATTR_RAW(...)
#endif

	///	@brief		リフレクション定義
	///	@details	クラス内で定義することで、privateメンバもリフレクション対象になります
#define	NOX_DECLARE_REFLECTION(ClassType) \
private:\
	NOX_ATTR_TEXT(::nox::reflection::attr::IgnoreReflection);\
	inline constexpr void StaticAssertNoxDeclareReflection()noexcept{ static_assert(std::is_same_v<ClassType, std::remove_cvref_t<decltype(*this)>>); }\
	friend struct ::nox::reflection::gen::TypeInfoRefHolder<ClassType>

	/// @brief		リフレクションオブジェクト定義
	/// @details	リフレクション対象となり、型IDを取得する関数が定義されます
#define NOX_DECLARE_REFLECTION_OBJECT(ClassType)\
public:\
constexpr virtual std::uint32_t GetUniqueTypeID()const noexcept{return ::nox::util::GetUniqueTypeID<ClassType>();}\
NOX_DECLARE_REFLECTION(ClassType)


	/// @brief 属性インターフェース
	class ReflectionObject
	{
		NOX_DECLARE_REFLECTION_OBJECT(ReflectionObject);
	protected:
		[[nodiscard]] inline constexpr ReflectionObject()noexcept = default;
		inline constexpr virtual ~ReflectionObject()noexcept {}
	private:
		inline constexpr ReflectionObject(const ReflectionObject&)noexcept = delete;
		inline constexpr ReflectionObject(const ReflectionObject&&)noexcept = delete;

		inline constexpr void operator =(const ReflectionObject&)noexcept = delete;
		inline constexpr void operator =(const ReflectionObject&&)noexcept = delete;
	};

	/// @brief 属性クラスインターフェース
	struct IAttribute
	{
	protected:
		[[nodiscard]] inline constexpr IAttribute()noexcept = default;
	};
}