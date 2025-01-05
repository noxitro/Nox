///	@file	reflection_object.h
///	@brief	reflection_object
#pragma once
#include	"attribute.h"
#include	"type.h"

namespace nox::reflection
{
	/// @brief		自動生成コード名前空間
	///	@details	直接は使用しません
	namespace gen
	{
		//	前方宣言
		template<class T>
		struct ReflectionTypeActivator;

		/// @brief リフレクション情報保持基底構造体
		struct ReflectionGeneratedHolderBase
		{

		};

		/// @brief class,union型のリフレクション用の前方宣言
		/// @tparam T 
		template<class T>  requires(std::is_class_v<T> || std::is_enum_v<T> || std::is_union_v<T>)
			struct ReflectionGeneratedHolder;

		/// @brief 名前空間に存在するリフレクション用の前方宣言
		/// @tparam ModuleId モジュール名のハッシュ値
		/// @tparam Id 名前空間文字列のハッシュ値
		template<std::uint32_t ModuleId, std::uint32_t Id>
		struct ReflectionGeneratedGlobalHolder;
	}

	///	@brief				リフレクション定義
	///	@param ClassType 	型
	///	@details			クラス内で定義することで、privateメンバもリフレクション対象になります
#define	NOX_DECLARE_REFLECTION(ClassType) \
		friend struct ::nox::reflection::gen::ReflectionTypeActivator<ClassType>;	\
	private:\
		NOX_ATTR_DECLARATION(::nox::reflection::attr::IgnoreReflection())	\
		inline constexpr void StaticAssertNoxDeclareReflection()noexcept{ \
			static_assert(std::is_same_v<ClassType, std::remove_cvref_t<decltype(*this)>>); \
		}\
		friend struct ::nox::reflection::gen::ReflectionGeneratedHolder<ClassType>
//	end define

	/// @brief		リフレクションオブジェクト定義
	///	@param ClassType 	型
	/// @details	リフレクション対象となり、型情報を取得する関数が定義されます
#define NOX_DECLARE_REFLECTION_OBJECT(ClassType)\
	private:\
		NOX_ATTR_DECLARATION(::nox::reflection::attr::IgnoreReflection())	\
		inline consteval void StaticAssertNoxDeclareReflectionObject()noexcept{ static_assert(std::is_base_of_v<::nox::reflection::ReflectionObject, ClassType>, "is not base of ReflectionObject"); }\
	public:\
		inline constexpr const ::nox::reflection::Type& GetType()const noexcept override { return ::nox::reflection::Typeof<ClassType>(); }\
		NOX_DECLARE_REFLECTION(ClassType)
//	end define
	
	/// @brief リフレクションオブジェクト
	class ReflectionObject
	{
		NOX_DECLARE_REFLECTION(ReflectionObject);
	public:
		/// @brief 型情報を取得
		inline constexpr virtual const ::nox::reflection::Type& GetType()const noexcept = 0;

	//protected:
		[[nodiscard]] inline constexpr ReflectionObject()noexcept = default;
		inline constexpr virtual ~ReflectionObject()noexcept {}

	protected:
	/*	/// @brief		関数がオーバーライドされているか
		bool	IsOverride(nox::uint64 function_id)const noexcept;


		inline bool IsOverride()const noexcept
		{
			return ReflectionObject::IsOverride(nox::GetFunctionPointerID<T>());
		}*/

	private: 
		inline constexpr ReflectionObject(const ReflectionObject&)noexcept = delete;
		inline constexpr ReflectionObject(ReflectionObject&&)noexcept = delete;

		inline constexpr void operator =(const ReflectionObject&)noexcept = delete;
		inline constexpr void operator =(ReflectionObject&&)noexcept = delete;
	};
}