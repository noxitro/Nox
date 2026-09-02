// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	entity_logic.h
/// @brief	ECSの上にOOPの書き味を載せるための基底。
/// @details 宣言したComponentDataが揃ったentityごとに1インスタンスが自動生成され、欠けたら自動破棄される。
///          個別の状態は普通のメンバとして持てるため、全てをComponentDataにしなくてよい。
///
///          更新メソッドは引数リストがそのままアクセス宣言になる(EntitySystemと同じ規則)ため、
///          OOP的に書いてもSystemと同じ依存解析に載る。
///
///          更新メソッドの購読は NOX_ATTR(nox::attr::EntityLogicMethod(フェーズ)) を付けるだけでよい。
///          リフレクション生成コードが nox::EntityLogicMethodTable の明示的特殊化を書き出し、
///          nox::GetEntityLogicTypes() の表に載せる。
///
///          使い分け: 大量に湧くもの(弾・パーティクル・群れ)はEntitySystem、
///          少数の主要個体(プレイヤー・ボス・UI)はEntityLogic。
#pragma once
#include	"entity_query.h"
#include	"system_phase_type.h"
#include	"entity_logic_attribute.h"

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
	/// @details 全メンバが定数式で埋まるため定数初期化される。
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
	};

	template<class TLogic>
	[[nodiscard]] constexpr nox::EntityLogicTypeDescriptor MakeEntityLogicTypeDescriptor()noexcept;

	/// @brief EntityLogicの構築をエンジンに限定するためのpasskey。
	/// @details コンストラクタがprivateで、nox::MakeEntityLogicTypeDescriptor()だけがfriend。
	///          そのため EntityLogic の派生型のコンストラクタをpublicにしても、
	///          記述子のconstruct経由(=エンジン)以外からは実体を作れない。
	class EntityLogicKey final
	{
	private:
		inline constexpr EntityLogicKey()noexcept = default;

		template<class TLogic>
		friend constexpr nox::EntityLogicTypeDescriptor nox::MakeEntityLogicTypeDescriptor()noexcept;
	};

	/// @brief EntityLogic型の更新メソッド表。
	/// @details 一次テンプレートは宣言のみ。リフレクション生成コードが型ごとに
	///          明示的特殊化(static constexpr k_methods[] と GetMethods())を定義する。
	///          生成器が名前を書けない型(クラステンプレート・無名名前空間)は、
	///          この特殊化を手書きすれば同じ経路に載せられる。
	template<class TLogic>
	struct EntityLogicMethodTable;

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
	/// @details インスタンスが存在するための必須ComponentDataは、購読した更新メソッドの引数から
	///          自動的に導出される(全メソッドが宣言したComponentDataの和集合)。
	///          基底のテンプレート引数に型を並べ直す必要はなく、メソッドの引数リストが唯一の宣言になる。
	/// @tparam TDerived CRTPの派生型。
	/// @tparam ExtraRequiredComponents どのメソッドも引数に取らないが、存在を必須にしたいComponentData
	///         (タグ用)。通常は指定しない。
	template<class TDerived, class... ExtraRequiredComponents>
		requires((nox::IsComponentDataType<ExtraRequiredComponents>() && ...))
	class EntityLogic
	{
	public:
		[[nodiscard]] inline nox::EntityId GetEntity()const noexcept { return entity_; }
		[[nodiscard]] inline nox::World& GetWorld()const noexcept { return *world_; }

		/// @brief どのメソッドも宣言しないが必須にしたいComponentDataのマスク。
		/// @details ComponentTypeIndexは実行時に採番されるため定数式にはならない。
		[[nodiscard]] static nox::ComponentMask MakeExtraRequiredMask()noexcept
		{
			return nox::MakeComponentMask<ExtraRequiredComponents...>();
		}

	protected:
		//	passkeyは派生型からそのまま受け流すだけ。エンジン以外から実体を作らせないための鍵。
		inline EntityLogic(nox::EntityLogicKey, nox::World& world, const nox::EntityId entity)noexcept :
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

	/// @brief 更新メソッド1つ分の記述子を作る。手書きの特殊化(エスケープハッチ)用。
	/// @details メソッドのアドレスを通常の文脈で取るため、publicなメソッドにしか使えない。
	///          生成コードは nox::detail::MakeEntityLogicMethodDescriptorViaTag を使う。
	///
	///          引数リストは任意。EntityId / ComponentDataの参照 / Serviceの参照・ポインタ を自由に並べられる。
	///          ここで宣言したComponentDataがEntityLogicの必須ComponentDataに算入される。
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

	/// @brief EntityLogic型の記述子を作る。
	/// @details nox::EntityLogicMethodTable<TLogic> だけを読む。CRTP基底には何も持たせない。
	template<class TLogic>
	[[nodiscard]] constexpr nox::EntityLogicTypeDescriptor MakeEntityLogicTypeDescriptor()noexcept
	{
		using MethodTable = nox::EntityLogicMethodTable<TLogic>;

		//	必須マスクが空だと全entityに付いてしまうため、メソッド0個は誤りとみなす。
		static_assert(MethodTable::GetMethods().empty() == false,
			"EntityLogicには nox::attr::EntityLogicMethod を付けた更新メソッドが1つ以上必要です");

		return nox::EntityLogicTypeDescriptor{
			.make_required_mask = []()noexcept
				{
					//	必須ComponentData = 購読された全メソッドが宣言したComponentDataの和集合。
					//	メソッドを足せば必要な条件も自動で広がるので、宣言が二重管理にならない。
					nox::ComponentMask mask = TLogic::MakeExtraRequiredMask();
					for (const nox::EntityLogicMethodDescriptor& method : MethodTable::GetMethods())
					{
						mask.Merge(method.make_read_write_mask());
					}
					return mask;
				},
			.construct = [](void* memory, nox::World& world, nox::EntityId entity) -> void*
				{
					return new(memory) TLogic(nox::EntityLogicKey{}, world, entity);
				},
			.destruct = [](void* instance)noexcept { static_cast<TLogic*>(instance)->~TLogic(); },
			.get_methods = []()noexcept { return MethodTable::GetMethods(); },
			.instance_size = static_cast<nox::uint32>(sizeof(TLogic)),
			.instance_alignment = static_cast<nox::uint32>(alignof(TLogic)),
			.name = nox::util::GetTypeName<TLogic>(),
		};
	}
}

namespace nox::gen
{
	/// @brief private な更新メソッドを、対象クラスにfriendを足さずに購読へ載せるための実行サンク。
	/// @details [temp.explicit] により、明示的実体化の宣言に現れる名前にはアクセス検査が適用されない。
	///          そのため生成コードは private なメソッドのアドレスをテンプレート実引数として渡せる。
	///          この実体化が Tag に宣言された friend 関数(=実行サンク)を定義する。
	///          サンク本体はテンプレート実引数の値を使うだけで、private な名前を綴らない。
	///
	///          MSVC はこの形で定義した friend を定数評価できないため、記述子側は定数式で friend を
	///          呼ばず、invokeラムダの中から実行時に呼ぶ。呼び出しは静的に束縛されインライン化される。
	///
	///          Tag と同じ名前空間に置く必要がある(クラス内で定義したfriendは最も内側の
	///          名前空間のメンバになるため、nox::detail に置くと Tag の宣言と別物になり未解決になる)。
	/// @tparam Tag 生成コードが宣言するメソッド1つ分のタグ型。
	/// @tparam MethodPointer 対象メソッドへのメンバ関数ポインタ。
	template<class Tag, auto MethodPointer>
	struct PrivateEntityLogicMethodInvoker final
	{
		using OwnerType = typename nox::EntityMethodTraits<decltype(MethodPointer)>::OwnerType;
		using Signature = typename nox::EntityMethodTraits<decltype(MethodPointer)>::Signature;

		//	依存型を引数に取る friend のため、汎用リフレクションの対象からは外す。
		NOX_ATTR(nox::reflection::attr::IgnoreReflection())
		friend void InvokeEntityLogicMethod(
			Tag,
			void* instance,
			nox::World& world,
			nox::Archetype& archetype,
			const nox::ArchetypeLocation location,
			const nox::EntityId entity)
		{
			nox::detail::EntityInvokerOf<Signature>::InvokeSingle(
				world, archetype, location, entity, *static_cast<OwnerType*>(instance), MethodPointer);
		}
	};
}

namespace nox::detail
{
	/// @brief 更新メソッド1つ分の記述子を、生成コードのタグ経由で作る。
	/// @details メソッドが public でも private でも同じ経路に載る。記述子はメソッド名を一切綴らず、
	///          Tag が持つ型情報(所有型・メンバ関数ポインタ型)だけを読む。
	/// @tparam Tag 生成コードが宣言したタグ型。OwnerType / MethodPointerType / Signature を持つ。
	template<class Tag, nox::SystemPhaseType _Phase>
	[[nodiscard]] constexpr nox::EntityLogicMethodDescriptor MakeEntityLogicMethodDescriptorViaTag(const std::string_view name)noexcept
	{
		using Signature = typename Tag::Signature;

		//	引数リストの妥当性検査は手書き経路と同一。
		static_assert(nox::detail::ValidateEntityMethod<typename Tag::MethodPointerType>());

		return nox::EntityLogicMethodDescriptor{
			.invoke = [](
				void* instance,
				nox::World& world,
				nox::Archetype& archetype,
				const nox::ArchetypeLocation location,
				const nox::EntityId entity)
				{
					//	定数評価されるのはラムダ→関数ポインタ変換だけ。friendの呼び出しは実行時。
					InvokeEntityLogicMethod(Tag{}, instance, world, archetype, location, entity);
				},
			.make_read_write_mask = []()noexcept { return Signature::GetReadWriteMask(); },
			.make_write_mask = []()noexcept { return Signature::GetWriteMask(); },
			.name = name,
			.phase = _Phase,
		};
	}
}
