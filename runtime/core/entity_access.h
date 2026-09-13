// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	entity_access.h
/// @brief	System / EntityLogic の関数シグネチャからアクセス権限を導出する型レベル基盤。
/// @details 宣言(引数リスト)と実際に触れる範囲が型で一致するため、UpdaterGraphの依存解析の入力と
///          実アクセス権限が乖離しない。宣言外アクセスはコンパイルエラーになる。
#pragma once
#include	"component_type.h"
#include	"entity.h"
#include	"entity_commands.h"
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
		/// @brief nox::EntityCommands& (フェーズ実行中に許されるWorld操作)
		Commands,
	};

	/// @brief 引数リストが宣言したService1つ分のアクセス権限。
	/// @details ComponentDataのようなdense indexを持たないため、型情報のアドレスで同一性を見る。
	///          UpdaterGraphの依存解析(同一Serviceへの書き込みが絡めば直列化)の入力になる。
	struct ServiceAccess final
	{
		/// @brief Serviceの型情報。アドレスがそのまま型の同一性を表す。
		const nox::reflection::Type* type;
		/// @brief 非constで受けている(=書き込み権限がある)か。
		bool write;
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

	//	Serviceは参照でも受けられる。ComponentDataの T& / const T& 特殊化とは制約が排他なので
	//	曖昧にならない(ServiceはObject派生で仮想デストラクタを持つため、IsComponentDataTypeを満たし得ない)。
	//	参照で受けた場合は未登録時にnullを渡せないため、呼び出し側が実行を打ち切る。
	template<nox::ServiceParameter T>
	struct EntityParameterTraits<T&>
	{
		static constexpr nox::EntityParameterKind k_kind = nox::EntityParameterKind::ServiceWrite;
		using RawType = std::remove_cv_t<T>;
	};

	template<nox::ServiceParameter T>
	struct EntityParameterTraits<const T&>
	{
		static constexpr nox::EntityParameterKind k_kind = nox::EntityParameterKind::ServiceRead;
		using RawType = std::remove_cv_t<T>;
	};

	//	EntityCommandsはComponentDataでもServiceでもないため、マスクにも各種カウントにも算入されない。
	//	constでは Destroy を呼べず宣言として意味を成さないので、const参照の特殊化は用意しない
	//	(= Invalid のまま残り、ValidateEntityMethodがコンパイルエラーにする)。
	template<>
	struct EntityParameterTraits<nox::EntityCommands&>
	{
		static constexpr nox::EntityParameterKind k_kind = nox::EntityParameterKind::Commands;
		using RawType = nox::EntityCommands;
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

		/// @brief ServiceAccessを1つ書き出す。Service以外の引数では何もしない。
		/// @details nox::reflection::Typeof<T>() は静的記憶域を持つ定数オブジェクトへの参照を返すため、
		///          そのアドレスは定数式になる。よってこの配列は定数初期化でき、動的初期化もヒープも使わない。
		template<class Parameter, size_t Count>
		static constexpr void AppendServiceAccess(
			std::array<nox::ServiceAccess, Count>& accesses,
			nox::uint32& index)noexcept
		{
			if constexpr (nox::detail::IsServiceParameterKind(Traits<Parameter>::k_kind))
			{
				accesses[index] = nox::ServiceAccess{
					.type = &nox::reflection::Typeof<typename Traits<Parameter>::RawType>(),
					.write = (Traits<Parameter>::k_kind == nox::EntityParameterKind::ServiceWrite),
				};
				++index;
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

		/// @brief nox::EntityCommands& を受けている引数の数。
		/// @details 遅延構造変更を出す宣言そのもの。Chunk並列(k_parallel_for_each)との
		///          両立可否をコンパイル時に判定するために数える。
		static constexpr nox::uint32 k_commands_parameter_count =
			((Traits<Parameters>::k_kind == nox::EntityParameterKind::Commands ? 1u : 0u) + ... + 0u);

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

		/// @brief 引数リストが宣言したServiceのアクセス権限一覧。
		/// @details ComponentDataと違いdense indexを持たないので、マスクではなく型情報の配列で返す。
		///          配列は定数初期化された関数内staticなので、呼び出しても確保は走らない。
		///          EntityCommandsは何も算入しない。
		[[nodiscard]] static std::span<const nox::ServiceAccess> GetServiceAccesses()noexcept
		{
			static constexpr std::array<nox::ServiceAccess, k_service_parameter_count> k_accesses = []()constexpr noexcept
				{
					std::array<nox::ServiceAccess, k_service_parameter_count> accesses{};
					nox::uint32 index = 0u;
					(AppendServiceAccess<Parameters>(accesses, index), ...);
					return accesses;
				}();

			return std::span<const nox::ServiceAccess>(k_accesses.data(), k_accesses.size());
		}
	};

	/// @brief 2つのシグネチャが同一フェーズ内で並列実行できるか。
	/// @details 同一ComponentDataにRWが絡めば直列化、全てROなら並列。UpdaterGraphの依存ルールそのもの。
	template<class SignatureA, class SignatureB>
	inline constexpr bool CanRunConcurrently =
		SignatureA::template IsUnwrittenBy<SignatureB>() && SignatureB::template IsUnwrittenBy<SignatureA>();

	namespace detail
	{
		/// @brief std::tuple の要素をそのまま nox::EntitySignature の引数リストへ移し替える。
		template<class TupleType>
		struct EntitySignatureFromTuple;

		template<class... Parameters>
		struct EntitySignatureFromTuple<std::tuple<Parameters...>>
		{
			using Type = nox::EntitySignature<Parameters...>;
		};

		/// @brief 更新メソッドとして受け付けるメンバ関数ポインタか。
		/// @details 形の網羅 (const / volatile / noexcept / 参照修飾の24通り) は
		///          nox::detail::FunctionSignature に任せ、ここは受け入れ条件だけを見る。
		///          volatile を弾くのは、記述子が保持するのが非volatileのインスタンスであり、
		///          volatileアクセスの前提は UpdaterGraph のジョブ分配とも噛み合わないため。
		///          参照修飾を弾くのは、呼び出しが常に保持済みインスタンスのlvalueに対して
		///          行われるため && は呼び出し不能で、& も宣言として区別する意味が無いため。
		template<class T>
		[[nodiscard]] consteval bool IsEntityMethodPointer()noexcept
		{
			if constexpr (std::is_member_function_pointer_v<T>)
			{
				return std::is_void_v<nox::FunctionResultType<T>> &&
					nox::IsFunctionVolatileValue<T> == false &&
					nox::IsFunctionLValueReference<T> == false &&
					nox::IsFunctionRValueReference<T> == false;
			}
			else
			{
				return false;
			}
		}

		/// @brief 受け入れ条件を満たしたときだけ OwnerType / Signature を持つ。
		/// @details 満たさない場合は空になるので、nox::EntityMethod の requires 節が
		///          ハードエラーにならずにfalseを返せる。
		template<class MethodPointerType, bool Accepted>
		struct EntityMethodTraitsImpl
		{
		};

		template<class MethodPointerType>
		struct EntityMethodTraitsImpl<MethodPointerType, true>
		{
			using OwnerType = nox::FunctionClassType<MethodPointerType>;
			using Signature = typename nox::detail::EntitySignatureFromTuple<
				nox::FunctionArgsTupleType<MethodPointerType>>::Type;
			static constexpr bool k_is_const = nox::IsFunctionConstValue<MethodPointerType>;
		};
	}

	/// @brief メンバ関数ポインタから所有型と引数リストを取り出す。
	/// @details 形ごとの特殊化は手書きせず、nox::detail::FunctionSignature の分解結果を読み替える。
	template<class MethodPointerType>
	using EntityMethodTraits = nox::detail::EntityMethodTraitsImpl<
		MethodPointerType, nox::detail::IsEntityMethodPointer<MethodPointerType>()>;

	/// @brief System / EntityLogic のメソッドとして妥当な形か。
	/// @details 戻り値void・volatile / 参照修飾なしの非静的メンバ関数で、引数は
	///          EntityId / ComponentData参照 / Serviceポインタ・参照 / nox::EntityCommands& のみ。
	template<class MethodPointerType>
	concept EntityMethod =
		requires { typename nox::EntityMethodTraits<MethodPointerType>::Signature; } &&
		nox::EntityMethodTraits<MethodPointerType>::Signature::k_is_valid;

	namespace detail
	{
		/// @brief コンパイルエラーを不備ごとに切り分けて出す。
		template<class MethodPointerType>
		consteval bool ValidateEntityMethod()noexcept
		{
			static_assert(std::is_member_function_pointer_v<MethodPointerType>,
				"非静的メンバ関数へのポインタを指定してください");

			if constexpr (std::is_member_function_pointer_v<MethodPointerType>)
			{
				static_assert(std::is_void_v<nox::FunctionResultType<MethodPointerType>>,
					"更新メソッドの戻り値は void にしてください");
				static_assert(nox::IsFunctionVolatileValue<MethodPointerType> == false,
					"volatile修飾したメソッドは更新メソッドにできません");
				static_assert(
					nox::IsFunctionLValueReference<MethodPointerType> == false &&
					nox::IsFunctionRValueReference<MethodPointerType> == false,
					"参照修飾(& / &&)したメソッドは更新メソッドにできません");

				if constexpr (nox::detail::IsEntityMethodPointer<MethodPointerType>())
				{
					using Signature = typename nox::EntityMethodTraits<MethodPointerType>::Signature;
					static_assert(Signature::k_all_parameters_valid,
						"引数は nox::EntityId / ComponentDataの参照 / Serviceのポインタ・参照 / nox::EntityCommands& のいずれかのみ指定できます");
					static_assert(Signature::k_entity_parameter_count <= 1u,
						"nox::EntityIdは1つまでしか指定できません");
					static_assert(Signature::k_entity_parameter_is_leading,
						"nox::EntityIdは第一引数にのみ指定できます");
					static_assert(Signature::k_has_unique_components,
						"同じComponentDataを複数の引数で宣言することはできません");
					return Signature::k_is_valid;
				}
			}

			return false;
		}
	}
}
