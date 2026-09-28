// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	service.h
/// @brief	service
/// @details Worldが所有する共有機能。Worldの破棄がそのままServiceの破棄になるため、
///          シングルトンのような特別な終了処理・再初期化を持たない。
///          System / EntityLogic は引数に Service* を書くだけで受け取れる。
///
///          Serviceは共通の更新(Update)を持たないが、寿命(初期化 / 終了)は持つ。
///          毎フレームの処理は、属性 nox::attr::ServiceMethod を付けたメソッドとして書くと
///          UpdaterGraph のノードになる(引数でアクセスを宣言する。規則は service_method.h)。
///          派生型はprivateな仮想関数 OnInitialize / OnShutdown を上書きする(NVI)。
///          呼ぶのはWorldだけで、派生型の書き手はfriendを書かなくてよい。
///
///          他のServiceへの依存は、派生型にpublicな `using Depends = nox::TypeList<...>;` で宣言する(省略可)。
///          初期化は Depends のトポロジカル順(決まらない箇所は完全修飾型名順。登録順は使わない)、
///          終了はその逆順。依存先の解決は型名の一致で行う(UpdaterGraphの明示辺と同じ方式)。
///          @code
///          class AudioService final : public nox::Service
///          {
///              NOX_DECLARE_OBJECT(game::AudioService, nox::Service);
///          public:
///              using Depends = nox::TypeList<game::SoundBankService>;
///          private:
///              bool OnInitialize(nox::ServiceContext& context)noexcept override
///              {
///                  sound_bank_ = context.Get<game::SoundBankService>();	// Dependsに並べた型だけ引ける
///                  return sound_bank_ != nullptr;
///              }
///              void OnShutdown()noexcept override {}
///              game::SoundBankService* sound_bank_ = nullptr;
///          };
///          @endcode
///          未登録の型への Depends・循環・OnInitialize の false・宣言外の Get は起動失敗になる
///          (nox::World::TryInitializeServices を参照)。
#pragma once
#include	"object.h"

namespace nox
{
	class World;
	class Service;

	/// @brief 型だけを並べる空のリスト。定義と使い方は entity_access.h を参照。
	/// @details Serviceの Depends を書くためだけにこのヘッダから見えるようにしてある。
	///          並べる型の名前しか使わないので、宣言だけで足りる。
	template<class... Types>
	struct TypeList;

	namespace detail
	{
		/// @brief Depends に並べた型の完全修飾名の表。
		/// @details nox::detail::UpdaterOrderNameTable と同じく、名前は nox::util::GetTypeName
		///          (定数初期化された静的記憶域)を指すだけなので表も定数初期化される。
		///          ヒープも動的初期化も使わない。名前の解決(= 登録済みのServiceか)は
		///          nox::World::TryInitializeServices で行う。ここでは型の完全性を要求しない。
		template<class TList>
		struct ServiceDependsNameTable
		{
			static_assert(std::is_void_v<TList> && (std::is_void_v<TList> == false),
				"Depends には nox::TypeList<Service型...> を指定してください");
		};

		template<class... Types>
		struct ServiceDependsNameTable<nox::TypeList<Types...>>
		{
			static_assert(((std::is_class_v<Types> && std::is_same_v<Types, std::remove_cv_t<Types>>) && ... && true),
				"Depends にはcv修飾の無いクラス型(Service)を並べてください");

			static constexpr std::array<std::string_view, sizeof...(Types)> kNames{ nox::util::GetTypeName<Types>()... };

			[[nodiscard]] static constexpr std::span<const std::string_view> Get()noexcept
			{
				if constexpr (sizeof...(Types) == 0u)
				{
					return std::span<const std::string_view>();
				}
				else
				{
					return std::span<const std::string_view>(kNames.data(), kNames.size());
				}
			}
		};

		/// @brief T::Depends に並べた型の名前。宣言が無ければ空。
		/// @details privateに書いたエイリアスはここから見えず、宣言が無いのと同じになる。必ずpublicに書くこと。
		template<class T>
		[[nodiscard]] constexpr std::span<const std::string_view> GetServiceDependsTypeNames()noexcept
		{
			if constexpr (requires { typename T::Depends; })
			{
				return nox::detail::ServiceDependsNameTable<typename T::Depends>::Get();
			}
			else
			{
				return std::span<const std::string_view>();
			}
		}

		/// @brief 名前で引ける登録済みService1つ分。nox::ServiceContext が引く表の要素。
		struct ServiceLookupEntry final
		{
			/// @brief 完全修飾型名(nox::util::GetTypeName と同じ綴り)。
			std::string_view type_name;
			nox::Service* service = nullptr;
		};
	}

	/// @brief Service型の記述子。Worldへの登録時に渡す。
	/// @details 型名は type->GetTypeName() (= nox::util::GetTypeName<T>())。
	struct ServiceTypeDescriptor final
	{
		const nox::reflection::Type* type = nullptr;
		/// @brief このServiceより先に初期化するServiceの完全修飾型名(T::Depends)。
		std::span<const std::string_view> depends;
	};

	/// @brief Service型の記述子を作る。T::Depends があればそれを読む。
	template<class T>
		requires(std::derived_from<T, nox::Service>)
	[[nodiscard]] constexpr nox::ServiceTypeDescriptor MakeServiceTypeDescriptor()noexcept
	{
		return nox::ServiceTypeDescriptor{
			.type = &nox::reflection::Typeof<T>(),
			.depends = nox::detail::GetServiceDependsTypeNames<T>(),
		};
	}

	/// @brief Serviceの初期化(ServiceGraph)が失敗した理由。いずれも宣言(コード)の誤りか、OnInitializeの失敗。
	enum class ServiceInitializeError : nox::uint8
	{
		/// @brief 成功。
		None,
		/// @brief 同じ型のServiceが2つ登録されている(名前で依存を解決できない)。
		DuplicateService,
		/// @brief Depends に並べた型が、Serviceとして登録されていない。
		UnresolvedDependency,
		/// @brief Depends が循環している(自分自身を並べた場合を含む)。
		DependencyCycle,
		/// @brief OnInitialize が false を返した。
		InitializeFailed,
		/// @brief OnInitialize の中で、Depends に並べていない型を nox::ServiceContext::Get で引いた。
		UndeclaredServiceAccess,
	};

	/// @brief Serviceの初期化結果。
	/// @details 名前はいずれも静的記憶域(型名)を指すので、Worldより長く生きる。
	struct ServiceInitializeResult final
	{
		nox::ServiceInitializeError error = nox::ServiceInitializeError::None;
		/// @brief 失敗を検出したService(循環なら後に初期化される側)の型名。
		std::string_view service_type_name;
		/// @brief 関係する相手の型名。未解決なら並べた名前、循環なら先に初期化される側、
		///        宣言外アクセスなら引こうとした型。InitializeFailed では空。
		std::string_view related_type_name;

		[[nodiscard]] constexpr bool IsSuccess()const noexcept { return error == nox::ServiceInitializeError::None; }
	};

	/// @brief OnInitialize の引数。Depends に並べたServiceだけを引ける。
	/// @details 宣言 = 依存解析の唯一の入力、という規則をServiceにも適用する。
	///          宣言していない型を引くと nullptr が返り、その時点で起動失敗が記録される
	///          (OnInitialize が true を返しても nox::ServiceInitializeError::UndeclaredServiceAccess になる)。
	///          Worldへの参照は持たない。作れるのはWorldだけ。
	class ServiceContext final
	{
		friend class World;
	public:
		ServiceContext(const ServiceContext&) = delete;
		ServiceContext& operator=(const ServiceContext&) = delete;

		/// @brief Depends に並べたServiceを引く。
		/// @details Depends の順に初期化済みなので、宣言していれば必ず初期化後の実体が返る。
		///          照合は完全修飾型名(nox::util::GetTypeName)で行う。起動時に数回呼ばれるだけなので線形走査で足りる。
		/// @return 宣言していない型なら nullptr(起動失敗が記録される)。
		template<class T>
			requires(std::derived_from<T, nox::Service> && std::is_same_v<T, std::remove_cv_t<T>>)
		[[nodiscard]] T* Get()noexcept
		{
			return static_cast<T*>(TryGetDeclared(nox::util::GetTypeName<T>()));
		}

	private:
		/// @param declared_type_names 初期化するServiceの Depends。
		/// @param services 登録済みの全Service。名前で引く。
		ServiceContext(
			std::span<const std::string_view> declared_type_names,
			std::span<const nox::detail::ServiceLookupEntry> services)noexcept;

		[[nodiscard]] nox::Service* TryGetDeclared(std::string_view type_name)noexcept;

		/// @brief 宣言外に引こうとした最初の型名。無ければ空。
		[[nodiscard]] inline std::string_view GetUndeclaredTypeName()const noexcept { return undeclared_type_name_; }

	private:
		std::span<const std::string_view> declared_type_names_;
		std::span<const nox::detail::ServiceLookupEntry> services_;
		std::string_view undeclared_type_name_;
	};

	/// @brief Worldに登録される共有機能の基底。
	/// @details 寿命はNVI。Worldだけが Initialize / Shutdown を呼び、派生型は
	///          privateな OnInitialize / OnShutdown を上書きする(既定は何もしないで成功)。
	///          例外は使わない。初期化の失敗は OnInitialize の false で返す。
	class Service : public ::nox::Object
	{
		friend class World;
		NOX_DECLARE_OBJECT(Service, nox::Object);
	protected:
		inline constexpr Service()noexcept = default;

	private:
		/// @brief Worldが Depends の順に1回だけ呼ぶ。
		inline bool Initialize(nox::ServiceContext& context)noexcept { return OnInitialize(context); }

		/// @brief Worldが初期化と逆順に1回だけ呼ぶ。OnInitialize が true を返したServiceにだけ呼ばれる。
		inline void Shutdown()noexcept { OnShutdown(); }

		/// @brief 初期化。Depends に並べたServiceは初期化済みで、context.Get<T>() で引ける。
		/// @return 失敗ならfalse。起動が止まり、初期化済みのServiceだけが逆順に終了する。
		virtual bool OnInitialize([[maybe_unused]] nox::ServiceContext& context)noexcept { return true; }

		/// @brief 終了。このServiceに依存するServiceは終了済みで、依存先はまだ生きている。
		virtual void OnShutdown()noexcept {}
	};

	namespace detail
	{
		/// @brief Worldに登録済みのServiceを型で引く。
		/// @details entity_query.hがworld.hに依存しないための橋渡し。
		[[nodiscard]] nox::Service* TryGetServiceOfWorld(nox::World& world, const nox::reflection::Type& type)noexcept;
	}
}
