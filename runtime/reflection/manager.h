///	@file	manager.h
///	@brief	manager
#pragma once
#include	"definition.h"
#include	"type.h"

namespace nox::reflection
{
	class Reflection : public nox::ISingleton<Reflection>
	{
		friend class nox::ISingleton<Reflection>;
	private:
		/// @brief クラスデータ
		struct ClassNode
		{
			std::reference_wrapper<const class UserDefinedCompoundTypeInfo> class_info;
			ClassNode* next_ptr;
			const ClassNode* prev_ptr;
			ClassNode* child_ptr;

			inline constexpr ClassNode(const class UserDefinedCompoundTypeInfo& _class_info)noexcept:
				class_info(_class_info),
				next_ptr(nullptr),
				prev_ptr(nullptr),
				child_ptr(nullptr)
			{

			}

			inline constexpr ClassNode(const ClassNode& rhs)noexcept:
				class_info(rhs.class_info),
				next_ptr(rhs.next_ptr),
				prev_ptr(rhs.prev_ptr),
				child_ptr(rhs.child_ptr)
			{
			}

			inline ClassNode(const ClassNode&&)noexcept = delete;

			inline ClassNode& operator =(const ClassNode& rhs)noexcept
			{
				class_info = rhs.class_info;
				next_ptr = rhs.next_ptr;
				prev_ptr = rhs.prev_ptr;
				child_ptr = rhs.child_ptr;
				return *this;
			}

			inline ClassNode& operator =(ClassNode&& rts)noexcept = delete;
		};

		/// @brief 翻訳単位(project)の情報
		struct Artifact
		{
			std::int32_t counter_;

			struct
			{
				nox::UnorderedMap<std::uint32_t, ClassNode> class_node_map;
				nox::UnorderedMap<std::uint32_t, std::reference_wrapper<const class UserDefinedCompoundTypeInfo>> union_map;
				nox::UnorderedMap<std::uint64_t, std::reference_wrapper<const class VariableInfo>> variable_map;
				nox::UnorderedMap<std::uint64_t, std::reference_wrapper<const class FunctionInfo>> function_map;
				nox::UnorderedMap<std::uint32_t, std::reference_wrapper<const class EnumInfo>> enum_map;
			}chunk_with_type_id;

			struct
			{
				nox::UnorderedMap<std::uint32_t, std::reference_wrapper<const ClassNode>> class_node_map;
				nox::UnorderedMap<std::uint32_t, std::reference_wrapper<const class UserDefinedCompoundTypeInfo>> union_map;
				nox::UnorderedMap<std::uint32_t, std::reference_wrapper<const class VariableInfo>> variable_map;
				nox::UnorderedMap<std::uint32_t, std::reference_wrapper<const class FunctionInfo>> function_map;
				nox::UnorderedMap<std::uint32_t, std::reference_wrapper<const class EnumInfo>> enum_map;

			}chunk_with_name_hash;

			inline Artifact() :counter_(0)
			{}

			inline Artifact(const Artifact&)noexcept
			{
				NOX_ASSERT(false, U"failed");
			}

			inline Artifact(Artifact&& rhs)noexcept 
			{
				counter_ = rhs.counter_;

				chunk_with_type_id.class_node_map = std::move(rhs.chunk_with_type_id.class_node_map);
				chunk_with_type_id.union_map = std::move(rhs.chunk_with_type_id.union_map);
				chunk_with_type_id.variable_map = std::move(rhs.chunk_with_type_id.variable_map);
				chunk_with_type_id.function_map = std::move(rhs.chunk_with_type_id.function_map);
				chunk_with_type_id.enum_map = std::move(rhs.chunk_with_type_id.enum_map);

				chunk_with_name_hash.class_node_map = std::move(rhs.chunk_with_name_hash.class_node_map);
				chunk_with_name_hash.union_map = std::move(rhs.chunk_with_name_hash.union_map);
				chunk_with_name_hash.variable_map = std::move(rhs.chunk_with_name_hash.variable_map);
				chunk_with_name_hash.function_map = std::move(rhs.chunk_with_name_hash.function_map);
				chunk_with_name_hash.enum_map = std::move(rhs.chunk_with_name_hash.enum_map);
			}
		};
	public:
		void	Finalize();
		
		const class UserDefinedCompoundTypeInfo* FindClassInfo(const nox::reflection::Type& type)const noexcept;
		const class UserDefinedCompoundTypeInfo* FindClassInfo(std::uint32_t namehash)const noexcept;
		const class UserDefinedCompoundTypeInfo* FindClassInfo(ReflectionStringView fullName)const noexcept { return FindClassInfo(nox::util::Crc32(fullName)); }

		template<nox::concepts::UserDefinedCompoundType T>
		inline const class UserDefinedCompoundTypeInfo* FindClassInfo()const noexcept {
			return FindClassInfo(nox::reflection::Typeof<T>());
		}

		const class EnumInfo* FindEnumInfo(const nox::reflection::Type& type)const noexcept;
		template<nox::concepts::Enum T>
		inline const class EnumInfo* FindEnumInfo()const noexcept
		{
			return FindEnumInfo(nox::reflection::Typeof<T>());
		}
		const class EnumInfo* FindEnumInfo(const std::uint32_t artiifact_name_hash, const nox::reflection::Type& type)const noexcept;
		template<nox::concepts::Enum T>
		inline const class EnumInfo* FindEnumInfo(const std::uint32_t artiifact_name_hash)const noexcept
		{
			return FindEnumInfo(artiifact_name_hash, nox::reflection::Typeof<T>());
		}

		inline const class FunctionInfo* FindFunctionInfo(const std::uint64_t id)const noexcept;

		template<nox::concepts::EveryFunctionType T>
		inline const class FunctionInfo* FindFunctionInfo()const noexcept
		{
			return FindFunctionInfo(nox::util::GetFunctionPointerID<T>());
		}


		bool IsBaseOf(const nox::reflection::UserDefinedCompoundTypeInfo& base, const nox::reflection::UserDefinedCompoundTypeInfo& derived);

		inline bool IsBaseOf(const nox::reflection::Type& baseType, const nox::reflection::Type& derivedType)
		{
			if (baseType.IsClass() == false || derivedType.IsClass() == false)
			{
				return false;
			}

			const class UserDefinedCompoundTypeInfo*const base = FindClassInfo(baseType);
			const class UserDefinedCompoundTypeInfo*const derived = FindClassInfo(derivedType);

			if (base == nullptr || derived == nullptr)
			{
				return false;
			}

			return IsBaseOf(*base, *derived);
		}

		template<nox::concepts::Class Base, nox::concepts::Class Derived>
		inline bool IsBaseOf()
		{
			return IsBaseOf(nox::reflection::Typeof<Base>(), nox::reflection::Typeof<Derived>());
		}

#pragma region 登録関数
		void Register(const std::uint32_t artifact_name_hash, const class UserDefinedCompoundTypeInfo& data);
		void Unregister(const std::uint32_t artifact_name_hash, const class UserDefinedCompoundTypeInfo& data);

		void Register(const std::uint32_t artifact_name_hash, const class EnumInfo& data);
		void Unregister(const std::uint32_t artifact_name_hash, const class EnumInfo& data);

		void Register(const std::uint32_t artifact_name_hash, const class VariableInfo& data);
		void Unregister(const std::uint32_t artifact_name_hash, const class VariableInfo& data);

		void Register(const std::uint32_t artifact_name_hash, const class FunctionInfo& data);
		void Unregister(const std::uint32_t artifact_name_hash, const class FunctionInfo& data);
#pragma endregion
	private:
		inline Reflection() {}

		inline	Artifact& GetCreateArtifact(std::uint32_t name_hash);

		inline	void RegisterTypeIdMap(const nox::reflection::Type& type);
		inline	void UnregisterTypeIdMap(const nox::reflection::Type& type);
	private:
		/// @brief 全ての翻訳単位の情報を格納するマップ
		nox::UnorderedMap<std::uint32_t, Artifact> artifact_map_;

		/// @brief 全ての型情報を格納するマップ
		nox::UnorderedMap<std::uint32_t, std::reference_wrapper<const nox::reflection::Type>> all_type_id_map_;
	};
	
	/// @brief as cast
	/// @tparam _Base 
	/// @tparam _Derived 
	/// @param value 
	/// @return 
	template<class _Derived, class _Base>
		requires(
	(
		std::is_const_v<std::remove_pointer_t<std::decay_t<_Base>>> == std::is_const_v<std::remove_pointer_t<std::decay_t<_Derived>>> ||
		std::is_const_v<std::remove_pointer_t<std::decay_t<_Base>>> == false
		) &&
	std::is_same_v<std::remove_pointer_t<std::decay_t<_Base>>, std::remove_pointer_t<std::decay_t<_Derived>>> == false &&
	std::is_base_of_v<std::remove_pointer_t<std::decay_t<_Base>>, std::remove_pointer_t<_Derived>>)
	inline _Derived AsCast(_Base&& value)noexcept
	{
		if (nox::reflection::Reflection::Instance().IsBaseOf<_Base, _Derived>() == false)
		{
			return nullptr;
		}

		return static_cast<_Derived>(value);
	}
}