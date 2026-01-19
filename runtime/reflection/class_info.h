///	@file	class_info.h
///	@brief	クラス情報
#pragma once

namespace nox::reflection
{
	//	前方宣言
	class FunctionInfo;
	class VariableInfo;
	class EnumInfo;
	class ReflectionObject;
	//struct IAttribute;

	class TypeInfo
	{

	};

	class PrimitiveTypeInfo : public TypeInfo
	{

	};

	/// @brief 継承元クラスの型、アクセスレベル、属性リストを保持する BaseSpecifier クラス。
	class BaseSpecifierInfo
	{
	public:
		inline constexpr explicit BaseSpecifierInfo(
			const nox::reflection::Type& type,
			const nox::reflection::AccessLevel accessLevel,
			const std::reference_wrapper<const nox::reflection::ReflectionObject>* attribute_list,
			const std::uint8_t attribute_length
		)noexcept:
			accessLevel_(accessLevel),
			attribute_length_(attribute_length),
			type_(type),
			attribute_list_(attribute_list)
		{}

		inline constexpr const nox::reflection::Type& GetType()const noexcept { return type_; }
		inline constexpr nox::reflection::AccessLevel GetAccessLevel()const noexcept { return accessLevel_; }
		[[nodiscard]] inline	constexpr	std::uint8_t	GetAttributeListLength()const noexcept { return attribute_length_; }
		[[nodiscard]] inline	constexpr	const std::span<const std::reference_wrapper<const reflection::ReflectionObject>> GetAttributeList()const noexcept { return std::span(attribute_list_, attribute_length_); }
		[[nodiscard]] inline	constexpr	const nox::reflection::ReflectionObject& GetAttribute(const std::uint8_t index)const noexcept { return nox::util::At(attribute_list_, attribute_length_, index); }
	private:
		const nox::reflection::AccessLevel accessLevel_;
		const std::uint8_t attribute_length_;

		const nox::reflection::Type& type_;
		const std::reference_wrapper<const nox::reflection::ReflectionObject>* attribute_list_;
	};

	/// @brief 型エイリアスに関する情報を保持するクラスです。
	class TypeAliasInfo
	{
	private:
		const nox::reflection::Type& type_;
		const ReflectionStringView name_;
		const ReflectionStringView fullname_;
		const ReflectionStringView namespace_;
	};

	/// @brief ユーザー定義の複合型情報
	/// @details クラス、構造体、共用体が該当します
	class ClassInfo : public TypeInfo
	{
	public:
		inline constexpr explicit ClassInfo(
			const nox::reflection::Type& type,
			ReflectionStringView name,
			ReflectionStringView fullname,
			ReflectionStringView _namespace,
			const nox::reflection::Type& external_class_type,
			const std::reference_wrapper<const nox::reflection::Type>* base_type_list,
			const std::uint8_t base_type_length,
			const std::reference_wrapper<const nox::reflection::ReflectionObject>* attribute_list,
			const std::uint8_t attribute_length,
			const std::reference_wrapper<const nox::reflection::VariableInfo>* variable_list,
			const std::uint8_t variable_length,
			const std::reference_wrapper<const nox::reflection::FunctionInfo>* function_list,
			const std::uint8_t function_length,
			const std::reference_wrapper<const nox::reflection::EnumInfo>* enum_list,
			const std::uint8_t enum_length,
			const std::reference_wrapper<const nox::reflection::Type>* internal_type_list,
			const std::uint8_t internal_class_length
		)noexcept:
			type_(type),
			name_(name),
			fullname_(fullname),
			namespace_(_namespace),
			external_class_type_(external_class_type),
			base_type_list_(base_type_list),
			base_type_length_(base_type_length),
			attribute_list_(attribute_list),
			attribute_length_(attribute_length),
			variable_list_(variable_list),
			variable_length_(variable_length),
			function_list_(function_list),
			function_length_(function_length),
			internal_type_list_(internal_type_list),
			internal_type_length_(internal_class_length),
			enum_list_(enum_list),
			enum_length_(enum_length)
		{

		}

#pragma region アクセサ
		[[nodiscard]] inline	constexpr	const nox::reflection::Type& GetType()const noexcept { return type_; }
		[[nodiscard]] inline	constexpr	bool	IsValid()const noexcept { return type_.IsValid(); }

		[[nodiscard]] inline	constexpr	ReflectionStringView GetName()const noexcept { return name_; }
		[[nodiscard]] inline	constexpr	ReflectionStringView GetFullName()const noexcept { return fullname_; }
		[[nodiscard]] inline	constexpr	ReflectionStringView GetNamespace()const noexcept { return namespace_; }

		[[nodiscard]] inline	constexpr	const nox::reflection::Type& GetExternalType()const noexcept { return external_class_type_; }
		[[nodiscard]] inline	const nox::reflection::ClassInfo& GetExternalUserDefinedCompoundTypeInfo()const noexcept { return nox::util::Deref(external_class_type_.GetUserDefinedCompoundTypeInfo()); }


		[[nodiscard]] inline	constexpr	std::uint8_t	GetAttributeListLength()const noexcept { return attribute_length_; }
		[[nodiscard]] inline	constexpr	const std::span<const std::reference_wrapper<const reflection::ReflectionObject>> GetAttributeList()const noexcept { return std::span(attribute_list_, attribute_length_); }
		[[nodiscard]] inline	constexpr	const nox::reflection::ReflectionObject& GetAttribute(const std::uint8_t index)const noexcept { return nox::util::At(attribute_list_, attribute_length_, index); }

		template<class T> requires
			(
				std::is_base_of_v<nox::reflection::ReflectionObject, T> && 
				std::is_base_of_v<nox::reflection::IAttribute, T> 
				)
		[[nodiscard]] inline	constexpr	const T* GetAttribute()const noexcept
		{
			for (std::int32_t i = 0; i < attribute_length_; ++i)
			{
				if (attribute_list_[i].get().GetType() == nox::reflection::Typeof<T>())
				{
					return static_cast<const T*>(&attribute_list_[i].get());
				}
			}
			return nullptr;
		}

		[[nodiscard]] inline constexpr std::uint8_t GetBaseTypeLength()const noexcept { return base_type_length_; }
		[[nodiscard]] inline constexpr const std::span<const std::reference_wrapper<const reflection::Type>> GetBaseTypeList()const noexcept { return std::span(base_type_list_, base_type_length_); }
		[[nodiscard]] inline constexpr std::uint8_t GetVariableLength()const noexcept { return variable_length_; }
		[[nodiscard]] inline constexpr const std::span<const std::reference_wrapper<const nox::reflection::VariableInfo>> GetVariableList()const noexcept { return std::span(variable_list_, variable_length_); }
		[[nodiscard]] inline constexpr std::uint8_t GetFunctionLength()const noexcept { return function_length_; }
		[[nodiscard]] inline constexpr const std::span<const std::reference_wrapper<const nox::reflection::FunctionInfo>> GetFunctionList()const noexcept { return std::span(function_list_, function_length_); }
		[[nodiscard]] inline constexpr std::uint8_t GetInternalTypeLength()const noexcept { return internal_type_length_; }
		[[nodiscard]] inline constexpr const std::span<const std::reference_wrapper<const nox::reflection::Type>> GetInternalClassList()const noexcept { return std::span(internal_type_list_, internal_type_length_); }
		[[nodiscard]] inline constexpr std::uint8_t GetEnumLength()const noexcept { return enum_length_; }
		[[nodiscard]] inline constexpr	std::span<const std::reference_wrapper<const nox::reflection::EnumInfo>> GetEnumInfoList()const noexcept { return std::span(enum_list_, enum_length_); }
		[[nodiscard]] inline constexpr const nox::reflection::EnumInfo& GetEnumInfo(std::uint8_t index)const noexcept { return nox::util::At(enum_list_, enum_length_, index); }

		[[nodiscard]] bool	IsBaseOf(const nox::reflection::ClassInfo& derived)const noexcept;
		[[nodiscard]] bool	IsBaseOf(const nox::reflection::Type& derived)const noexcept;
		template<class T>
		inline bool IsBaseOf()const noexcept
		{
			return this->IsBaseOf(nox::reflection::Typeof<T>());
		}

		[[nodiscard]] bool	IsSubclassOf(const nox::reflection::Type& base)const noexcept;

		const nox::reflection::FunctionInfo* GetCopyConstructor()const noexcept;
		const nox::reflection::FunctionInfo* GetMoveConstructor()const noexcept;

		const nox::reflection::FunctionInfo* GetConstructor(std::span<const std::reference_wrapper<const nox::reflection::Type>> args)const noexcept;
		inline const nox::reflection::FunctionInfo* GetConstructor(std::initializer_list<const std::reference_wrapper<const nox::reflection::Type>> args)const noexcept
		{
			return GetConstructor(std::span(args.begin(), args.end()));
		}

		template<class... Args>
		inline const nox::reflection::FunctionInfo* GetConstructor()const noexcept
		{
			return GetConstructor(nox::reflection::Typeof<Args>()...);
		}
#pragma endregion

	private:
		/*inline constexpr ClassInfo(const ClassInfo&)noexcept = delete;
		inline constexpr ClassInfo(const ClassInfo&&)noexcept = delete;
		inline constexpr void operator =(const ClassInfo&)noexcept = delete;
		inline constexpr void operator =(const ClassInfo&&)noexcept = delete;

		inline constexpr void operator ==(const ClassInfo&)noexcept = delete;
		inline constexpr void operator ==(const ClassInfo&&)noexcept = delete;
		inline constexpr void operator !=(const ClassInfo&)noexcept = delete;
		inline constexpr void operator !=(const ClassInfo&&)noexcept = delete;*/
	private:
		/// @brief 継承型テーブルの長さ
		std::uint8_t base_type_length_;

		/// @brief 属性テーブルの長さ
		std::uint8_t attribute_length_;

		/// @brief 変数テーブルの長さ
		std::uint8_t variable_length_;

		/// @brief 関数テーブルの長さ
		std::uint8_t function_length_;

		/// @brief 内部クラスの長さ
		std::uint8_t internal_type_length_;

		/// @brief 列挙体の数
		std::uint8_t enum_length_;

		/// @brief 自身のタイプ
		const Type& type_;

		/// @brief 自身が所属するクラス
		const nox::reflection::Type&	external_class_type_;

		/// @brief 継承型テーブル
		const std::reference_wrapper<const nox::reflection::Type>* base_type_list_;

		/// @brief 属性テーブル
		const std::reference_wrapper<const nox::reflection::ReflectionObject>* attribute_list_;

		/// @brief 変数情報ポインタテーブル
		const std::reference_wrapper<const class nox::reflection::VariableInfo>* variable_list_;

		/// @brief 関数情報ポインタテーブル
		const std::reference_wrapper<const class nox::reflection::FunctionInfo>* function_list_;

		/// @brief 内部複合型テーブル
		const std::reference_wrapper<const nox::reflection::Type>* internal_type_list_;

		/// @brief 内部列挙体テーブル
		const std::reference_wrapper<const class nox::reflection::EnumInfo>* enum_list_;

		/// @brief 名前
		ReflectionStringView name_;

		/// @brief フルネーム
		ReflectionStringView fullname_;

		/// @brief 名前空間
		ReflectionStringView namespace_;
	};

	namespace detail
	{
		class InvalidTypeInfo : public nox::reflection::ClassInfo
		{
		public:
			inline constexpr InvalidTypeInfo()noexcept :
				nox::reflection::ClassInfo(
					nox::reflection::GetInvalidType(),
					u8"",
					u8"",
					u8"",
					nox::reflection::GetInvalidType(),

					nullptr,
					0,
					nullptr,
					0,
					nullptr,
					0,
					nullptr,
					0,
					nullptr,
					0,
					nullptr,
					0
				)
			{}
		};

//		constexpr InvalidTypeInfo InvalidUserDefinedCompoundTypeInfo{};

	/*	template<class T> requires(std::is_class_v<T> || std::is_union_v<T>)
		inline constexpr nox::reflection::ClassInfo CreateUserDefinedCompoundTypeInfo(
			nox::reflection::ReflectionStringView name,
			nox::reflection::ReflectionStringView fullname,
			nox::reflection::ReflectionStringView _namespace,
			const nox::reflection::ClassInfo& external_class_type,
			const std::reference_wrapper<const nox::reflection::ClassInfo>* base_type_list,
			std::uint8_t base_type_length,
			const std::reference_wrapper<const nox::reflection::ReflectionObject>* attribute_list,
			std::uint8_t attribute_length,
			const std::reference_wrapper<const nox::reflection::VariableInfo>* variable_list,
			std::uint8_t variable_length,
			const std::reference_wrapper<const nox::reflection::FunctionInfo>* function_list,
			std::uint8_t function_length,
			const std::reference_wrapper<const nox::reflection::ClassInfo>* internal_type_list,
			std::uint8_t internal_type_length,
			const std::reference_wrapper<const nox::reflection::EnumInfo>* enum_list,
			std::uint8_t enum_length


		)noexcept
		{
			return nox::reflection::ClassInfo(
				nox::reflection::Typeof<T>(),
				name,
				fullname,
				_namespace,
				external_class_type,
				base_type_list,
				base_type_length,
				attribute_list,
				attribute_length,
				variable_list,
				variable_length,
				function_list,
				function_length,
				internal_type_list,
				internal_type_length,
				enum_list,
				enum_length
			);
		}*/
	}
}