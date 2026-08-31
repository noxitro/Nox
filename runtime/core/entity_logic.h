// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	entity_logic.h
/// @brief	ECSの上にOOPの書き味を載せるための基底。
/// @details 宣言したComponentDataが揃ったentityごとに1インスタンスが自動生成され、欠けたら自動破棄される。
///          個別の状態は普通のメンバとして持てるため、全てをComponentDataにしなくてよい。
///
///          更新メソッドは引数リストがそのままアクセス宣言になる(EntitySystemと同じ規則)ため、
///          OOP的に書いてもSystemと同じ依存解析に載る。
///
///          使い分け: 大量に湧くもの(弾・パーティクル・群れ)はEntitySystem、
///          少数の主要個体(プレイヤー・ボス・UI)はEntityLogic。
#pragma once
#include	"entity_query.h"
#include	"system_phase_type.h"

namespace nox
{
	class World;

	/// @brief EntityLogicの更新メソッド1つ分の記述子。
	struct EntityLogicMethodDescriptor final
	{
		/// @brief TDerivedのメソッドへ静的に束縛された呼び出し。仮想関数を介さない。
		void (*invoke)(
			void* instance,
			nox::World& world,
			nox::Archetype& archetype,
			nox::ArchetypeLocation location,
			nox::EntityId entity);
		/// @brief 読み書きするComponentDataのマスク。依存解析の入力。
		nox::ComponentMask(*make_read_write_mask)()noexcept;
		/// @brief 書き込みするComponentDataのマスク。
		nox::ComponentMask(*make_write_mask)()noexcept;
		std::string_view name;
		nox::SystemPhaseType phase;
	};

	/// @brief EntityLogic型ごとに1つだけ作られる静的記述子。
	struct EntityLogicTypeDescriptor final
	{
		/// @brief インスタンス生成に必要なComponentDataのマスク。
		nox::ComponentMask(*make_required_mask)()noexcept;
		/// @brief 確保済みメモリ上へのインスタンス構築。
		void* (*construct)(void* memory, nox::World& world, nox::EntityId entity);
		/// @brief インスタンス破棄。仮想デストラクタの代わり。
		void (*destruct)(void* instance)noexcept;
		/// @brief 更新メソッド一覧。
		std::span<const nox::EntityLogicMethodDescriptor>(*get_methods)()noexcept;
		nox::uint32 instance_size;
		nox::uint32 instance_alignment;
		std::string_view name;
		/// @brief 静的初期化順に繋がれる次の記述子。
		const nox::EntityLogicTypeDescriptor* next;
	};

	namespace detail
	{
		void RegisterEntityLogicType(nox::EntityLogicTypeDescriptor& descriptor)noexcept;
		[[nodiscard]] const nox::EntityLogicTypeDescriptor* GetEntityLogicTypeListHead()noexcept;
	}

	/// @brief 単一EntityLogic型のインスタンス置き場。
	/// @details インスタンスはブロック単位でまとめて確保し、破棄後はフリーリストへ戻す。
	///          定常状態では生成・破棄ともにヒープを触らない。
	class EntityLogicStorage final
	{
	public:
		struct Entry
		{
			nox::EntityId entity;
			void* instance;
		};

		/// @brief 1ブロックあたりのインスタンス数。
		static constexpr nox::uint32 k_instances_per_block = 64u;

	public:
		explicit EntityLogicStorage(const nox::EntityLogicTypeDescriptor& descriptor);
		~EntityLogicStorage();

		EntityLogicStorage(const EntityLogicStorage&) = delete;
		EntityLogicStorage& operator=(const EntityLogicStorage&) = delete;

		/// @brief entityがマスクを満たすようになったので生成する。既に存在する場合は何もしない。
		void CreateInstance(nox::World& world, nox::EntityId entity);

		/// @brief entityがマスクを満たさなくなった/破棄されたので解体する。
		void DestroyInstance(nox::EntityId entity)noexcept;

		[[nodiscard]] bool Contains(nox::EntityId entity)const noexcept;

		[[nodiscard]] inline const nox::EntityLogicTypeDescriptor& GetDescriptor()const noexcept { return descriptor_; }

		[[nodiscard]] inline std::span<const nox::EntityLogicStorage::Entry> GetEntries()const noexcept
		{
			return std::span(entries_.data(), entries_.size());
		}

	private:
		[[nodiscard]] void* AcquireInstanceMemory();
		[[nodiscard]] nox::int32 FindEntrySlot(nox::EntityId entity)const noexcept;

	private:
		const nox::EntityLogicTypeDescriptor& descriptor_;
		nox::uint32 instance_stride_;
		nox::Vector<nox::uint8*> blocks_;
		nox::Vector<void*> free_instances_;
		nox::Vector<nox::EntityLogicStorage::Entry> entries_;
	};

	/// @brief ECSの上にOOPを載せるための基底。
	/// @tparam TDerived CRTPの派生型。
	/// @tparam RequiredComponents インスタンスが存在するために必要なComponentData。
	template<class TDerived, class... RequiredComponents>
		requires(sizeof...(RequiredComponents) > 0u && (nox::IsComponentDataType<RequiredComponents>() && ...))
	class EntityLogic
	{
	public:
		using RequiredSignature = nox::EntitySignature<RequiredComponents&...>;

		[[nodiscard]] inline nox::EntityId GetEntity()const noexcept { return entity_; }
		[[nodiscard]] inline nox::World& GetWorld()const noexcept { return *world_; }

		/// @brief 型記述子。EntityLogicRegistrarが静的初期化時に参照する。
		[[nodiscard]] static nox::EntityLogicTypeDescriptor& GetTypeDescriptor()noexcept
		{
			static nox::EntityLogicTypeDescriptor descriptor{
				.make_required_mask = []()noexcept { return nox::MakeComponentMask<RequiredComponents...>(); },
				.construct = [](void* memory, nox::World& world, nox::EntityId entity) -> void*
					{
						return new(memory) TDerived(world, entity);
					},
				.destruct = [](void* instance)noexcept { static_cast<TDerived*>(instance)->~TDerived(); },
				.get_methods = []()noexcept { return TDerived::GetEntityLogicMethods(); },
				.instance_size = static_cast<nox::uint32>(sizeof(TDerived)),
				.instance_alignment = static_cast<nox::uint32>(alignof(TDerived)),
				.name = nox::util::GetTypeName<TDerived>(),
				.next = nullptr,
			};
			return descriptor;
		}

	protected:
		inline EntityLogic(nox::World& world, const nox::EntityId entity)noexcept :
			world_(&world),
			entity_(entity)
		{
		}

		//	記述子のdestruct経由でのみ破棄されるため非virtual。
		inline ~EntityLogic() = default;

		EntityLogic(const EntityLogic&) = delete;
		EntityLogic& operator=(const EntityLogic&) = delete;

	private:
		nox::World* world_;
		nox::EntityId entity_;
	};

	/// @brief 更新メソッド1つ分の記述子を作る。NOX_ENTITY_LOGIC_METHODが使う。
	/// @details 引数リストが宣言したComponentDataは、必ずEntityLogicの必須ComponentDataに含まれている必要がある。
	template<auto MethodPointer, nox::SystemPhaseType _Phase>
	[[nodiscard]] constexpr nox::EntityLogicMethodDescriptor MakeEntityLogicMethodDescriptor(const std::string_view name)noexcept
	{
		using Traits = nox::EntityMethodTraits<decltype(MethodPointer)>;
		using OwnerType = typename Traits::OwnerType;
		using Signature = typename Traits::Signature;
		using Invoker = nox::detail::EntityInvokerOf<Signature>;

		static_assert(nox::detail::ValidateEntityMethod<decltype(MethodPointer)>());

		return nox::EntityLogicMethodDescriptor{
			.invoke = [](
				void* instance,
				nox::World& world,
				nox::Archetype& archetype,
				const nox::ArchetypeLocation location,
				const nox::EntityId entity)
				{
					Invoker::InvokeSingle(world, archetype, location, entity, *static_cast<OwnerType*>(instance), MethodPointer);
				},
			.make_read_write_mask = []()noexcept { return Signature::GetReadWriteMask(); },
			.make_write_mask = []()noexcept { return Signature::GetWriteMask(); },
			.name = name,
			.phase = _Phase,
		};
	}

	/// @brief 静的初期化時に型記述子を登録する。NOX_DECLARE_ENTITY_LOGICが埋め込む。
	template<class TLogic>
	struct EntityLogicRegistrar final
	{
		inline EntityLogicRegistrar()noexcept
		{
			nox::detail::RegisterEntityLogicType(TLogic::GetTypeDescriptor());
		}
	};
}

/// @brief EntityLogicの更新メソッドを宣言する。NOX_DECLARE_ENTITY_LOGICの引数に並べる。
/// @param phase_type nox::SystemPhaseTypeの列挙子名(Init / Start / Update / Terminate)。
/// @param method_pointer &ClassName::MethodName 形式のメンバ関数ポインタ。
#define NOX_ENTITY_LOGIC_METHOD(phase_type, method_pointer)									\
	::nox::MakeEntityLogicMethodDescriptor<method_pointer, ::nox::SystemPhaseType::phase_type>(#method_pointer)
//	end define

/// @brief EntityLogicを購読させる。クラス本体に1行書くだけでWorldが自動生成・自動破棄・自動実行する。
/// @details 更新メソッドの宣言より後ろに置くこと。静的初期化で記述子が連結リストに繋がるため、
///          明示的な登録呼び出しは不要。
#define NOX_DECLARE_ENTITY_LOGIC(type, ...)														\
	public:																						\
		[[nodiscard]] static ::std::span<const ::nox::EntityLogicMethodDescriptor>				\
			GetEntityLogicMethods()noexcept														\
		{																						\
			static constexpr ::nox::EntityLogicMethodDescriptor k_methods[]{ __VA_ARGS__ };		\
			return ::std::span<const ::nox::EntityLogicMethodDescriptor>(k_methods);				\
		}																						\
	private:																					\
		friend struct ::nox::EntityLogicRegistrar<type>;										\
		static inline const ::nox::EntityLogicRegistrar<type> k_nox_entity_logic_registrar_{};	\
	public:
//	end define
