// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	entity_access.h
/// @brief	System / EntityLogic の関数シグネチャからアクセス権限を導出する型レベル基盤。
/// @details 宣言(引数リスト)と実際に触れる範囲が型で一致するため、UpdaterGraphの依存解析の入力と
///          実アクセス権限が乖離しない。宣言外アクセスはコンパイルエラーになる。
#pragma once
#include	"component_type.h"
#include	"entity.h"
#include	"service.h"

namespace nox
{
	class World;

	/// @brief 引数1つが表すアクセスの種別。
	enum class EntityParameterKind : nox::uint8
	{
		/// @brief 引数として使用できない型
		Invalid,
		/// @brief nox::EntityId (値渡し)
		Entity,
		/// @brief const ComponentData& (読み取り専用)
		ComponentRead,
		/// @brief ComponentData& (読み書き)
		ComponentWrite,
		/// @brief const Service* (読み取り専用)
		ServiceRead,
		/// @brief Service* (読み書き)
		ServiceWrite,
	};

	/// @brief ComponentDataとして引数に取れる型。
	template<class T>
	concept ComponentDataParameter = nox::IsComponentDataType<std::remove_cv_t<T>>();

	/// @brief Serviceとして引数に取れる型。
	template<class T>
	concept ServiceParameter = std::derived_from<std::remove_cv_t<T>, nox::Service>;

	/// @brief 引数1つの分類。特殊化に該当しない型はInvalidのまま残り、検証でコンパイルエラーになる。
	template<class T>
	struct EntityParameterTraits
	{
		static constexpr nox::EntityParameterKind k_kind = nox::EntityParameterKind::Invalid;
		using RawType = void;
	};

	template<>
	struct EntityParameterTraits<nox::EntityId>
	{
		static constexpr nox::EntityParameterKind k_kind = nox::EntityParameterKind::Entity;
		using RawType = nox::EntityId;
	};

	template<nox::ComponentDataParameter T>
	struct EntityParameterTraits<T&>
	{
		static constexpr nox::EntityParameterKind k_kind = nox::EntityParameterKind::ComponentWrite;
		using RawType = std::remove_cv_t<T>;
	};

	template<nox::ComponentDataParameter T>
	struct EntityParameterTraits<const T&>
	{
		static constexpr nox::EntityParameterKind k_kind = nox::EntityParameterKind::ComponentRead;
		using RawType = std::remove_cv_t<T>;
	};

	template<nox::ServiceParameter T>
	struct EntityParameterTraits<T*>
	{
		static constexpr nox::EntityParameterKind k_kind = nox::EntityParameterKind::ServiceWrite;
		using RawType = std::remove_cv_t<T>;
	};

	template<nox::ServiceParameter T>
	struct EntityParameterTraits<const T*>
	{
		static constexpr nox::EntityParameterKind k_kind = nox::EntityParameterKind::ServiceRead;
		using RawType = std::remove_cv_t<T>;
	};

	namespace detail
	{
		[[nodiscard]] inline constexpr bool IsComponentParameterKind(const nox::EntityParameterKind kind)noexcept
		{
			return kind == nox::EntityParameterKind::ComponentRead || kind == nox::EntityParameterKind::ComponentWrite;
		}

		[[nodiscard]] inline constexpr bool IsServiceParameterKind(const nox::EntityParameterKind kind)noexcept
		{
			return kind == nox::EntityParameterKind::ServiceRead || kind == nox::EntityParameterKind::ServiceWrite;
		}
	}

	/// @brief System / EntityLogic のメソッド引数リストから導出したアクセス宣言。
	/// @details 引数の並びがそのまま宣言なので、宣言用の別記述が要らない。
	template<class... Parameters>
	struct EntitySignature final
	{
	private:
		template<class Parameter>
		using Traits = nox::EntityParameterTraits<Parameter>;

		template<class Parameter>
		static void AccumulateComponentMask(nox::ComponentMask& mask, const bool write_only)noexcept
		{
			if constexpr (nox::detail::IsComponentParameterKind(Traits<Parameter>::k_kind))
			{
				if (write_only == false || Traits<Parameter>::k_kind == nox::EntityParameterKind::ComponentWrite)
				{
					mask.Set(nox::ComponentTypeIndexOf<typename Traits<Parameter>::RawType>());
				}
			}
		}

		/// @brief Parameterが表すComponentDataがRestの中に現れないか。
		template<class Parameter, class... Rest>
		[[nodiscard]] static consteval bool IsComponentAbsentFrom()noexcept
		{
			if constexpr (nox::detail::IsComponentParameterKind(Traits<Parameter>::k_kind) == false)
			{
				return true;
			}
			else
			{
				return ((nox::detail::IsComponentParameterKind(Traits<Rest>::k_kind) == false ||
					std::same_as<typename Traits<Parameter>::RawType, typename Traits<Rest>::RawType> == false) && ... && true);
			}
		}

		template<size_t Index, size_t... RestOffsets>
		[[nodiscard]] static consteval bool IsUniqueAtImpl(std::index_sequence<RestOffsets...>)noexcept
		{
			using ParameterTuple = std::tuple<Parameters...>;
			return IsComponentAbsentFrom<
				std::tuple_element_t<Index, ParameterTuple>,
				std::tuple_element_t<Index + 1u + RestOffsets, ParameterTuple>...>();
		}

		template<size_t Index>
		[[nodiscard]] static consteval bool IsUniqueAt()noexcept
		{
			return IsUniqueAtImpl<Index>(std::make_index_sequence<sizeof...(Parameters) - Index - 1u>{});
		}

		template<size_t... Indices>
		[[nodiscard]] static consteval bool HasUniqueComponentsImpl(std::index_sequence<Indices...>)noexcept
		{
			return (IsUniqueAt<Indices>() && ... && true);
		}

		[[nodiscard]] static consteval bool HasUniqueComponents()noexcept
		{
			if constexpr (sizeof...(Parameters) == 0u)
			{
				return true;
			}
			else
			{
				return HasUniqueComponentsImpl(std::make_index_sequence<sizeof...(Parameters)>{});
			}
		}

	public:
		static constexpr nox::uint32 k_parameter_count = sizeof...(Parameters);

		/// @brief 全ての引数が分類可能か。
		static constexpr bool k_all_parameters_valid =
			((Traits<Parameters>::k_kind != nox::EntityParameterKind::Invalid) && ... && true);

		static constexpr nox::uint32 k_entity_parameter_count =
			((Traits<Parameters>::k_kind == nox::EntityParameterKind::Entity ? 1u : 0u) + ... + 0u);

		static constexpr nox::uint32 k_component_parameter_count =
			((nox::detail::IsComponentParameterKind(Traits<Parameters>::k_kind) ? 1u : 0u) + ... + 0u);

		static constexpr nox::uint32 k_service_parameter_count =
			((nox::detail::IsServiceParameterKind(Traits<Parameters>::k_kind) ? 1u : 0u) + ... + 0u);

		/// @brief EntityIdは省略可能だが、書く場合は必ず先頭1つ。
		static constexpr bool k_entity_parameter_is_leading =
			(k_entity_parameter_count == 0u) ||
			(k_entity_parameter_count == 1u &&
				Traits<std::tuple_element_t<0, std::tuple<Parameters..., void>>>::k_kind == nox::EntityParameterKind::Entity);

		/// @brief 指定ComponentDataへの読み取り権限(constでも可)を持つか。
		template<class T>
		static constexpr bool HasReadAccess =
			((nox::detail::IsComponentParameterKind(Traits<Parameters>::k_kind) &&
				std::same_as<typename Traits<Parameters>::RawType, std::remove_cvref_t<T>>) || ... || false);

		/// @brief 指定ComponentDataへの書き込み権限を持つか。
		template<class T>
		static constexpr bool HasWriteAccess =
			((Traits<Parameters>::k_kind == nox::EntityParameterKind::ComponentWrite &&
				std::same_as<typename Traits<Parameters>::RawType, std::remove_cvref_t<T>>) || ... || false);

		/// @brief 自分が触る全ComponentDataについて、OtherSignatureが書き込みを持たないか。
		/// @details 片方向の判定。同時実行可否は nox::CanRunConcurrently で両方向を見る。
		template<class OtherSignature>
		[[nodiscard]] static consteval bool IsUnwrittenBy()noexcept
		{
			return ((nox::detail::IsComponentParameterKind(Traits<Parameters>::k_kind) == false ||
				OtherSignature::template HasWriteAccess<typename Traits<Parameters>::RawType> == false) && ... && true);
		}

		/// @brief 同じComponentDataを2回宣言していないか。
		static constexpr bool k_has_unique_components = HasUniqueComponents();

		static constexpr bool k_is_valid =
			k_all_parameters_valid &&
			k_entity_parameter_count <= 1u &&
			k_entity_parameter_is_leading &&
			k_has_unique_components;

		/// @brief 読み書きするComponentDataのマスク。Queryの必須条件を兼ねる。
		/// @details ComponentTypeIndexは実行時に払い出されるため、構築時に一度だけ呼ぶ。
		[[nodiscard]] static nox::ComponentMask GetReadWriteMask()noexcept
		{
			nox::ComponentMask mask{};
			(AccumulateComponentMask<Parameters>(mask, false), ...);
			return mask;
		}

		/// @brief 書き込みするComponentDataのマスク。
		[[nodiscard]] static nox::ComponentMask GetWriteMask()noexcept
		{
			nox::ComponentMask mask{};
			(AccumulateComponentMask<Parameters>(mask, true), ...);
			return mask;
		}

	};

	/// @brief 2つのシグネチャが同一フェーズ内で並列実行できるか。
	/// @details 同一ComponentDataにRWが絡めば直列化、全てROなら並列。UpdaterGraphの依存ルールそのもの。
	template<class SignatureA, class SignatureB>
	inline constexpr bool CanRunConcurrently =
		SignatureA::template IsUnwrittenBy<SignatureB>() && SignatureB::template IsUnwrittenBy<SignatureA>();

	/// @brief メンバ関数ポインタから引数リストを取り出す。
	template<class T>
	struct EntityMethodTraits;

	template<class ClassType, class... Parameters>
	struct EntityMethodTraits<void(ClassType::*)(Parameters...)>
	{
		using OwnerType = ClassType;
		using Signature = nox::EntitySignature<Parameters...>;
		static constexpr bool k_is_const = false;
	};

	template<class ClassType, class... Parameters>
	struct EntityMethodTraits<void(ClassType::*)(Parameters...)const>
	{
		using OwnerType = ClassType;
		using Signature = nox::EntitySignature<Parameters...>;
		static constexpr bool k_is_const = true;
	};

	template<class ClassType, class... Parameters>
	struct EntityMethodTraits<void(ClassType::*)(Parameters...)noexcept>
	{
		using OwnerType = ClassType;
		using Signature = nox::EntitySignature<Parameters...>;
		static constexpr bool k_is_const = false;
	};

	template<class ClassType, class... Parameters>
	struct EntityMethodTraits<void(ClassType::*)(Parameters...)const noexcept>
	{
		using OwnerType = ClassType;
		using Signature = nox::EntitySignature<Parameters...>;
		static constexpr bool k_is_const = true;
	};

	/// @brief System / EntityLogic のメソッドとして妥当な形か。
	/// @details 戻り値void・引数は EntityId / ComponentData参照 / Serviceポインタ のみ。
	template<class MethodPointerType>
	concept EntityMethod =
		requires { typename nox::EntityMethodTraits<MethodPointerType>::Signature; } &&
		nox::EntityMethodTraits<MethodPointerType>::Signature::k_is_valid;

	namespace detail
	{
		/// @brief コンパイルエラーを引数の不備ごとに切り分けて出す。
		template<class MethodPointerType>
		consteval bool ValidateEntityMethod()noexcept
		{
			static_assert(requires { typename nox::EntityMethodTraits<MethodPointerType>::Signature; },
				"戻り値voidの非静的メンバ関数を指定してください");
			using Signature = typename nox::EntityMethodTraits<MethodPointerType>::Signature;
			static_assert(Signature::k_all_parameters_valid,
				"引数は nox::EntityId / ComponentDataの参照 / Serviceのポインタ のいずれかのみ指定できます");
			static_assert(Signature::k_entity_parameter_count <= 1u,
				"nox::EntityIdは1つまでしか指定できません");
			static_assert(Signature::k_entity_parameter_is_leading,
				"nox::EntityIdは第一引数にのみ指定できます");
			static_assert(Signature::k_has_unique_components,
				"同じComponentDataを複数の引数で宣言することはできません");
			return Signature::k_is_valid;
		}
	}
}
