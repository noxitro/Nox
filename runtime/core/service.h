// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

/// @file	service.h
/// @brief	Worldが所有する共有機能(Service)と、そのフェーズ関数。
/// @details Worldの破棄がそのままServiceの破棄になるため、
///          シングルトンのような特別な終了処理・再初期化を持たない。
///          System / EntityLogic は引数に Service の参照・ポインタを書くだけで受け取れる。
///
///          Service はフェーズ関数(Init / Update / Terminate)を持てる。フェーズ関数は
///          System / EntityLogic と同じ UpdaterGraph のノードになり、同じ依存解析で並べられる。
///          引数に並べた Service がそのまま読み書きの宣言になる(const は読み取り、非 const は書き込み)。
///          フェーズ関数を持つ Service 自身への読み書きは、メンバ関数の const 修飾から暗黙に宣言される。
///
///          @code
///          class TimeService final : public nox::Service<TimeService>
///          {
///          	NOX_ECS_DECLARE_VERIFY(TimeService);
///          private:
///          	void Tick();
///          public:
///          	static constexpr auto kTickPhase = PhaseUpdate<&TimeService::Tick>{};
///          	static constexpr auto kPhaseList = PhaseRegister{ kTickPhase };
///          };
///
///          class SampleService final : public nox::Service<SampleService>
///          {
///          	NOX_ECS_DECLARE_VERIFY(SampleService);
///          private:
///          	void Tick(const TimeService& time);
///          public:
///          	//	TimeService の Tick より後に実行する
///          	static constexpr auto kTickPhase = PhaseUpdate<&SampleService::Tick, After<TimeService::kTickPhase>>{};
///          	static constexpr auto kPhaseList = PhaseRegister{ kTickPhase };
///          };
///          @endcode
///
///          - フェーズ関数は private でよい。呼び出しは記述子が作るサンクを経由する。
///          - ハンドル(kTickPhase など)は、フェーズ関数の宣言より後に書く。
///            他の Service やフェーズから After / Before で指せるよう public に置く。
///          - 一覧(kPhaseList)は関数ではなくデータメンバにする。クラス先頭の NOX_ECS_DECLARE_VERIFY は
///            クラスの全メンバ関数より先に本体が処理されるため、auto を返す関数では戻り値の推論が間に合わない。
///            データメンバなら宣言した時点で型が決まる。フェーズが無ければ PhaseRegister{} とする。
///          - 購読はリフレクション生成コードが行う。ヘッダにクラスを定義するだけで
///            nox::GetServiceTypes() の表に載り、World が生成・破棄する。
#pragma once
#include	"ecs_definitions.h"
#include	"entity_access.h"
#include	"system_phase_type.h"

namespace nox
{
	class World;

	template<class T>
	class Service;

	/// @brief Service のフェーズ関数を実行するフェーズ。
	/// @details System / EntityLogic と同じ UpdaterGraph の同名フェーズで実行される(nox::ToSystemPhaseType)。
	///          System / EntityLogic と宣言が衝突したときは、Init / Update では Service が先に、
	///          Terminate では Service が後に実行される。
	enum class ServicePhaseType : nox::uint8
	{
		Init,
		Update,
		Terminate,
	};

	/// @brief Service のフェーズを、実行する UpdaterGraph のフェーズへ写す。Start は使わない。
	[[nodiscard]] constexpr nox::SystemPhaseType ToSystemPhaseType(const nox::ServicePhaseType phase_type)noexcept
	{
		switch (phase_type)
		{
		case nox::ServicePhaseType::Init:
			return nox::SystemPhaseType::Init;
		case nox::ServicePhaseType::Terminate:
			return nox::SystemPhaseType::Terminate;
		case nox::ServicePhaseType::Update:
		default:
			return nox::SystemPhaseType::Update;
		}
	}

	/// @brief このフェーズより先に実行するフェーズの並び。値はフェーズのハンドル。
	template<auto... Phases>
	struct ServicePhaseAfter final
	{
	};

	/// @brief このフェーズより後に実行するフェーズの並び。値はフェーズのハンドル。
	template<auto... Phases>
	struct ServicePhaseBefore final
	{
	};

	/// @brief Service のフェーズ関数1つ。この型の値がフェーズのハンドルになる。
	/// @details 値は何も持たない。フェーズ・関数・依存はすべて型に載っているので、
	///          ハンドルは定数式のまま他の Service から After / Before で指せる。
	///          同一性はハンドルの型で決まる(nox::ServicePhaseMethodDescriptor::phase_key)。
	/// @tparam PhaseType 実行するフェーズ。
	/// @tparam Func フェーズ関数。Service 自身の非静的メンバ関数へのポインタ。
	/// @tparam Dependencies nox::ServicePhaseAfter / nox::ServicePhaseBefore の並び。
	template<nox::ServicePhaseType PhaseType, auto Func, class... Dependencies>
	struct ServicePhase final
	{
		static constexpr nox::ServicePhaseType kPhaseType = PhaseType;
		static constexpr auto kFunc = Func;
	};

	/// @brief Service のフェーズの一覧(kPhaseList の型)。
	/// @details 値は何も持たない。並びの順が、同じフェーズに並べた関数同士の登録順になる。
	template<class... Phases>
	struct ServicePhaseList final
	{
		constexpr ServicePhaseList(Phases...)noexcept
		{
		}
	};

	/// @brief Service の非テンプレート基底。
	/// @details フェーズの書き方(PhaseInit / PhaseUpdate / PhaseTerminate / After / Before / PhaseRegister)を
	///          派生クラスへ開く。
	class ServiceBase
	{
	public:
		using ServicePhaseType = nox::ServicePhaseType;

	protected:
		template<auto Func, class... Dependencies>
		using PhaseInit = nox::ServicePhase<nox::ServicePhaseType::Init, Func, Dependencies...>;
		template<auto Func, class... Dependencies>
		using PhaseUpdate = nox::ServicePhase<nox::ServicePhaseType::Update, Func, Dependencies...>;
		template<auto Func, class... Dependencies>
		using PhaseTerminate = nox::ServicePhase<nox::ServicePhaseType::Terminate, Func, Dependencies...>;

		template<auto... Phases>
		using After = nox::ServicePhaseAfter<Phases...>;
		template<auto... Phases>
		using Before = nox::ServicePhaseBefore<Phases...>;

		template<class... Phases>
		using PhaseRegister = nox::ServicePhaseList<Phases...>;

	protected:
		constexpr ServiceBase()noexcept = default;
		~ServiceBase() = default;
	};

	/// @brief フェーズ関数の引数1つ。
	struct ServicePhaseParameter final
	{
		/// @brief 引数の Service の型情報。World に置かれた Service と、型情報のアドレスで照合する。
		const nox::reflection::Type* type;
		/// @brief 参照で受けているか。参照には null を渡せないので、見つからなければフェーズ関数を呼べない。
		bool required;
	};

	/// @brief Service のフェーズ関数1つ分の記述子。UpdaterGraph のノード1つに対応する。
	/// @details 全メンバが定数式で埋まるため定数初期化される。ヒープも動的初期化も使わない。
	struct ServicePhaseMethodDescriptor final
	{
		/// @brief フェーズ関数へ静的に束縛した呼び出し。仮想関数を介さない。
		/// @details arguments はフェーズ関数の引数と同じ並び。UpdaterGraph が構築時に一度だけ解決して持つ。
		void (*invoke)(void* instance, void* const* arguments);
		/// @brief 読み書きする Service の一覧。先頭が自分自身(const メンバ関数なら読み取り)。依存解析の入力。
		std::span<const nox::ServiceAccess>(*get_service_accesses)()noexcept;
		/// @brief 引数の Service。invoke に渡す arguments と同じ並び。
		std::span<const nox::ServicePhaseParameter>(*get_parameters)()noexcept;
		/// @brief このフェーズより先に実行するフェーズ。ハンドルの型情報で表す。
		std::span<const nox::reflection::Type* const>(*get_after_phases)()noexcept;
		/// @brief このフェーズより後に実行するフェーズ。ハンドルの型情報で表す。
		std::span<const nox::reflection::Type* const>(*get_before_phases)()noexcept;
		/// @brief このフェーズの識別子。ハンドルの型情報で、他のフェーズの After / Before から指される。
		const nox::reflection::Type* phase_key;
		/// @brief フェーズ関数を持つ Service の型名。ログ用。
		std::string_view service_name;
		/// @brief フェーズ関数の名前。ログ用。
		std::string_view name;
		nox::SystemPhaseType phase;
	};

	/// @brief Service 型ごとに1つだけ作られる静的記述子。
	/// @details 全メンバが定数式で埋まるため定数初期化される。ヒープも動的初期化も使わない。
	struct ServiceTypeDescriptor final
	{
		/// @brief 確保済みメモリ上へのデフォルト構築。戻り値は実体の先頭(Service の型へ static_cast してよい)。
		void* (*construct)(void* memory);
		/// @brief 破棄。仮想デストラクタの代わり。
		void (*destruct)(void* instance)noexcept;
		/// @brief フェーズ関数の一覧。kPhaseList の並び。
		std::span<const nox::ServicePhaseMethodDescriptor>(*get_phase_methods)()noexcept;
		/// @brief Service の型情報。同一性はこのアドレスで見る(nox::ServiceAccess と同じ)。
		const nox::reflection::Type* type;
		nox::uint32 instance_size;
		nox::uint32 instance_alignment;
		std::string_view name;
	};

	/// @brief World に置かれた Service 1つ。UpdaterGraph がフェーズをノードにし、引数を解決するのに使う。
	struct ServiceInstance final
	{
		/// @brief Service の型情報。
		const nox::reflection::Type* type;
		/// @brief 実体の先頭。
		void* instance;
		/// @brief 記述子。nox::World::RegisterService で登録した旧 Service は nullptr(フェーズを持たない)。
		const nox::ServiceTypeDescriptor* descriptor;
	};

	namespace detail
	{
		template<class T>
		struct is_service_phase : std::false_type
		{
		};

		template<nox::ServicePhaseType PhaseType, auto Func, class... Dependencies>
		struct is_service_phase<nox::ServicePhase<PhaseType, Func, Dependencies...>> : std::true_type
		{
		};

		/// @brief フェーズのハンドルの型か。
		template<class T>
		inline constexpr bool is_service_phase_v = nox::detail::is_service_phase<std::remove_cv_t<T>>::value;

		template<class T>
		struct is_service_phase_list : std::false_type
		{
		};

		template<class... Phases>
		struct is_service_phase_list<nox::ServicePhaseList<Phases...>> : std::true_type
		{
		};

		/// @brief kPhaseList の型か。
		template<class T>
		inline constexpr bool is_service_phase_list_v = nox::detail::is_service_phase_list<std::remove_cv_t<T>>::value;

		/// @brief フェーズ関数の引数として受け付ける型か。nox::Service<T> を継承した型の参照・ポインタだけを通す。
		/// @details 旧 Service (nox::legacy::Service) は World::RegisterService でいつでも足せるため、
		///          構築時に引数を解決し切れない。フェーズ関数の引数には取らない。
		template<class Parameter>
		[[nodiscard]] consteval bool IsServicePhaseParameter()noexcept
		{
			using Traits = nox::EntityParameterTraits<Parameter>;
			if constexpr (nox::detail::IsServiceParameterKind(Traits::k_kind))
			{
				return std::is_base_of_v<nox::ServiceBase, typename Traits::RawType>;
			}
			else
			{
				return false;
			}
		}

		/// @brief ハンドルの値が、指定したフェーズのハンドルか。
		template<nox::ServicePhaseType PhaseType, auto Phase>
		[[nodiscard]] consteval bool IsServicePhaseHandleOf()noexcept
		{
			using HandleType = std::remove_cvref_t<decltype(Phase)>;
			if constexpr (nox::detail::is_service_phase_v<HandleType>)
			{
				return HandleType::kPhaseType == PhaseType;
			}
			else
			{
				return false;
			}
		}

		/// @brief 依存の並び1つ(After / Before)の解析。ハンドルを型情報の並びへ写す。
		template<class Dependency>
		struct ServicePhaseDependency
		{
			static constexpr bool is_dependency = false;
			static constexpr nox::uint32 kAfterCount = 0u;
			static constexpr nox::uint32 kBeforeCount = 0u;

			template<nox::ServicePhaseType PhaseType>
			[[nodiscard]] static consteval bool IsEveryHandleOf()noexcept
			{
				return true;
			}

			template<size_t Count>
			static constexpr void AppendAfter(std::array<const nox::reflection::Type*, Count>&, nox::uint32&)noexcept
			{
			}

			template<size_t Count>
			static constexpr void AppendBefore(std::array<const nox::reflection::Type*, Count>&, nox::uint32&)noexcept
			{
			}
		};

		template<auto... Phases>
		struct ServicePhaseDependency<nox::ServicePhaseAfter<Phases...>>
		{
			static constexpr bool is_dependency = true;
			static constexpr nox::uint32 kAfterCount = static_cast<nox::uint32>(sizeof...(Phases));
			static constexpr nox::uint32 kBeforeCount = 0u;

			template<nox::ServicePhaseType PhaseType>
			[[nodiscard]] static consteval bool IsEveryHandleOf()noexcept
			{
				return (nox::detail::IsServicePhaseHandleOf<PhaseType, Phases>() && ... && true);
			}

			template<size_t Count>
			static constexpr void AppendAfter(std::array<const nox::reflection::Type*, Count>& keys, nox::uint32& index)noexcept
			{
				((keys[index++] = &nox::reflection::Typeof<std::remove_cvref_t<decltype(Phases)>>()), ...);
			}

			template<size_t Count>
			static constexpr void AppendBefore(std::array<const nox::reflection::Type*, Count>&, nox::uint32&)noexcept
			{
			}
		};

		template<auto... Phases>
		struct ServicePhaseDependency<nox::ServicePhaseBefore<Phases...>>
		{
			static constexpr bool is_dependency = true;
			static constexpr nox::uint32 kAfterCount = 0u;
			static constexpr nox::uint32 kBeforeCount = static_cast<nox::uint32>(sizeof...(Phases));

			template<nox::ServicePhaseType PhaseType>
			[[nodiscard]] static consteval bool IsEveryHandleOf()noexcept
			{
				return (nox::detail::IsServicePhaseHandleOf<PhaseType, Phases>() && ... && true);
			}

			template<size_t Count>
			static constexpr void AppendAfter(std::array<const nox::reflection::Type*, Count>&, nox::uint32&)noexcept
			{
			}

			template<size_t Count>
			static constexpr void AppendBefore(std::array<const nox::reflection::Type*, Count>& keys, nox::uint32& index)noexcept
			{
				((keys[index++] = &nox::reflection::Typeof<std::remove_cvref_t<decltype(Phases)>>()), ...);
			}
		};

		/// @brief 解決済みの引数をフェーズ関数の引数の型へ戻す。
		template<class Parameter>
		[[nodiscard]] inline Parameter BindServicePhaseArgument(void* const argument)noexcept
		{
			if constexpr (std::is_reference_v<Parameter>)
			{
				return *static_cast<std::remove_reference_t<Parameter>*>(argument);
			}
			else
			{
				//	ポインタで受けた Service は、World に無ければ nullptr のまま渡る。
				return static_cast<Parameter>(argument);
			}
		}

		/// @brief フェーズ関数の引数リストの解析。依存解析の宣言と、呼び出しのサンクを作る。
		/// @tparam SelfWrite フェーズ関数が非 const(= 自分自身へ書き込む)か。
		template<class T, bool SelfWrite, class ParameterTuple>
		struct ServicePhaseParameterList;

		template<class T, bool SelfWrite, class... Parameters>
		struct ServicePhaseParameterList<T, SelfWrite, std::tuple<Parameters...>>
		{
			static constexpr bool is_valid = (nox::detail::IsServicePhaseParameter<Parameters>() && ... && true);

			static constexpr bool is_self_absent =
				((std::is_same_v<typename nox::EntityParameterTraits<Parameters>::RawType, T> == false) && ... && true);

			/// @brief 自分自身(先頭)と、引数に並べた Service への読み書き。
			[[nodiscard]] static std::span<const nox::ServiceAccess> GetServiceAccesses()noexcept
			{
				static constexpr std::array<nox::ServiceAccess, sizeof...(Parameters) + 1u> kAccesses{
					nox::ServiceAccess{ .type = &nox::reflection::Typeof<T>(), .write = SelfWrite },
					nox::ServiceAccess{
						.type = &nox::reflection::Typeof<typename nox::EntityParameterTraits<Parameters>::RawType>(),
						.write = (nox::EntityParameterTraits<Parameters>::k_kind == nox::EntityParameterKind::ServiceWrite),
					}...
				};
				return std::span<const nox::ServiceAccess>(kAccesses.data(), kAccesses.size());
			}

			[[nodiscard]] static std::span<const nox::ServicePhaseParameter> GetParameters()noexcept
			{
				static constexpr std::array<nox::ServicePhaseParameter, sizeof...(Parameters)> kParameters{
					nox::ServicePhaseParameter{
						.type = &nox::reflection::Typeof<typename nox::EntityParameterTraits<Parameters>::RawType>(),
						.required = std::is_reference_v<Parameters>,
					}...
				};
				return std::span<const nox::ServicePhaseParameter>(kParameters.data(), kParameters.size());
			}

			template<auto Func>
			static void Invoke(void* const instance, void* const* const arguments)
			{
				InvokeImpl<Func>(instance, arguments, std::index_sequence_for<Parameters...>{});
			}

		private:
			template<auto Func, size_t... Indices>
			static void InvokeImpl(void* const instance, [[maybe_unused]] void* const* const arguments, std::index_sequence<Indices...>)
			{
				(static_cast<T*>(instance)->*Func)(nox::detail::BindServicePhaseArgument<Parameters>(arguments[Indices])...);
			}
		};

		template<auto Func>
		[[nodiscard]] consteval std::string_view GetServicePhaseMethodSignature()noexcept
		{
#if defined(__clang__)
			return __PRETTY_FUNCTION__;
#else
			return __FUNCSIG__;
#endif
		}

		/// @brief フェーズ関数の名前(修飾を除いた識別子)をコンパイル時に取り出す。ログ用。
		template<auto Func>
		[[nodiscard]] consteval std::string_view GetServicePhaseMethodName()noexcept
		{
			const std::string_view signature = nox::detail::GetServicePhaseMethodSignature<Func>();
#if defined(__clang__)
			constexpr std::string_view k_marker = "[Func = ";
			constexpr char k_terminator = ']';
#else
			constexpr std::string_view k_marker = "GetServicePhaseMethodSignature<";
			constexpr char k_terminator = '(';
#endif
			const size_t marker_position = signature.find(k_marker);
			if (marker_position == std::string_view::npos)
			{
				return signature;
			}

			const size_t begin = marker_position + k_marker.size();
			const size_t end = signature.find(k_terminator, begin);
			const std::string_view qualified_name =
				signature.substr(begin, (end == std::string_view::npos) ? std::string_view::npos : (end - begin));
			const size_t separator = qualified_name.rfind("::");
			return (separator == std::string_view::npos) ? qualified_name : qualified_name.substr(separator + 2u);
		}

		/// @brief フェーズ1つ分の解析。宣言の検査と記述子の材料をまとめる。
		template<class T, class Phase>
		struct ServicePhaseTraits;

		template<class T, nox::ServicePhaseType PhaseType, auto Func, class... Dependencies>
		struct ServicePhaseTraits<T, nox::ServicePhase<PhaseType, Func, Dependencies...>>
		{
			using PhaseHandleType = nox::ServicePhase<PhaseType, Func, Dependencies...>;
			using MethodPointerType = decltype(Func);

			static constexpr bool is_member_function = std::is_member_function_pointer_v<MethodPointerType>;
			static constexpr nox::uint32 kAfterCount = (nox::detail::ServicePhaseDependency<Dependencies>::kAfterCount + ... + 0u);
			static constexpr nox::uint32 kBeforeCount = (nox::detail::ServicePhaseDependency<Dependencies>::kBeforeCount + ... + 0u);

			/// @brief 宣言の検査。不備ごとにコンパイルエラーを切り分けて出す。
			[[nodiscard]] static consteval bool Verify()noexcept
			{
				static_assert(is_member_function,
					"フェーズ関数には Service の非静的メンバ関数へのポインタを指定してください");

				if constexpr (is_member_function)
				{
					using ParameterList = nox::detail::ServicePhaseParameterList<
						T, true, nox::function_args_tuple_t<MethodPointerType>>;

					static_assert(std::is_same_v<nox::function_class_t<MethodPointerType>, T>,
						"フェーズ関数には、その Service 自身のメンバ関数を指定してください");
					static_assert(std::is_void_v<nox::function_result_t<MethodPointerType>>,
						"フェーズ関数の戻り値は void にしてください");
					static_assert(nox::is_function_volatile_v<MethodPointerType> == false,
						"volatile 修飾したメソッドはフェーズ関数にできません");
					static_assert(
						nox::is_function_lvalue_reference_v<MethodPointerType> == false &&
						nox::is_function_rvalue_reference_v<MethodPointerType> == false,
						"参照修飾(& / &&)したメソッドはフェーズ関数にできません");
					static_assert(ParameterList::is_valid,
						"フェーズ関数の引数には、nox::Service<T> を継承した Service の参照かポインタだけを指定できます");
					static_assert(ParameterList::is_self_absent,
						"フェーズ関数の引数に自分自身は指定できません(const 修飾から暗黙に宣言されます)");
				}

				static_assert((nox::detail::ServicePhaseDependency<Dependencies>::is_dependency && ... && true),
					"フェーズの依存には After<...> / Before<...> だけを指定できます");
				static_assert((nox::detail::ServicePhaseDependency<Dependencies>::template IsEveryHandleOf<PhaseType>() && ... && true),
					"After / Before には、同じフェーズ(Init / Update / Terminate)のハンドルだけを指定できます");
				return true;
			}

			[[nodiscard]] static std::span<const nox::reflection::Type* const> GetAfterPhases()noexcept
			{
				static constexpr std::array<const nox::reflection::Type*, kAfterCount> kKeys = []()constexpr noexcept
					{
						std::array<const nox::reflection::Type*, kAfterCount> keys{};
						[[maybe_unused]] nox::uint32 index = 0u;
						(nox::detail::ServicePhaseDependency<Dependencies>::AppendAfter(keys, index), ...);
						return keys;
					}();
				return std::span<const nox::reflection::Type* const>(kKeys.data(), kKeys.size());
			}

			[[nodiscard]] static std::span<const nox::reflection::Type* const> GetBeforePhases()noexcept
			{
				static constexpr std::array<const nox::reflection::Type*, kBeforeCount> kKeys = []()constexpr noexcept
					{
						std::array<const nox::reflection::Type*, kBeforeCount> keys{};
						[[maybe_unused]] nox::uint32 index = 0u;
						(nox::detail::ServicePhaseDependency<Dependencies>::AppendBefore(keys, index), ...);
						return keys;
					}();
				return std::span<const nox::reflection::Type* const>(kKeys.data(), kKeys.size());
			}

			[[nodiscard]] static consteval nox::ServicePhaseMethodDescriptor MakeDescriptor()noexcept
			{
				using ParameterList = nox::detail::ServicePhaseParameterList<
					T,
					(nox::is_function_const_v<MethodPointerType> == false),
					nox::function_args_tuple_t<MethodPointerType>>;

				return nox::ServicePhaseMethodDescriptor{
					.invoke = &ParameterList::template Invoke<Func>,
					.get_service_accesses = &ParameterList::GetServiceAccesses,
					.get_parameters = &ParameterList::GetParameters,
					.get_after_phases = &GetAfterPhases,
					.get_before_phases = &GetBeforePhases,
					.phase_key = &nox::reflection::Typeof<PhaseHandleType>(),
					.service_name = nox::util::GetTypeName<T>(),
					.name = nox::detail::GetServicePhaseMethodName<Func>(),
					.phase = nox::ToSystemPhaseType(PhaseType),
				};
			}
		};

		/// @brief 2つのハンドルが、同じフェーズの同じ関数を指しているか。
		template<class Left, class Right>
		[[nodiscard]] consteval bool IsSameServicePhaseMethod()noexcept
		{
			using LeftFunc = std::remove_cv_t<decltype(Left::kFunc)>;
			using RightFunc = std::remove_cv_t<decltype(Right::kFunc)>;
			if constexpr (std::is_same_v<LeftFunc, RightFunc>)
			{
				return (Left::kPhaseType == Right::kPhaseType) && (Left::kFunc == Right::kFunc);
			}
			else
			{
				return false;
			}
		}

		/// @brief kPhaseList の解析。宣言の検査と、フェーズ関数の記述子の表を持つ。
		template<class T, class PhaseList>
		struct ServicePhaseMethodTable;

		template<class T, class... Phases>
		struct ServicePhaseMethodTable<T, nox::ServicePhaseList<Phases...>>
		{
			static constexpr bool is_every_phase = (nox::detail::is_service_phase_v<Phases> && ... && true);

			template<class Phase>
			[[nodiscard]] static consteval nox::uint32 CountSameMethod()noexcept
			{
				return ((nox::detail::IsSameServicePhaseMethod<Phase, Phases>() ? 1u : 0u) + ... + 0u);
			}

			[[nodiscard]] static consteval bool Verify()noexcept
			{
				static_assert(is_every_phase,
					"kPhaseList には PhaseInit / PhaseUpdate / PhaseTerminate のハンドルだけを並べてください");

				if constexpr (is_every_phase)
				{
					static_assert((nox::detail::ServicePhaseTraits<T, Phases>::Verify() && ... && true));
					static_assert(((CountSameMethod<Phases>() == 1u) && ... && true),
						"同じフェーズに同じフェーズ関数を2回並べることはできません");
				}
				return true;
			}

			[[nodiscard]] static std::span<const nox::ServicePhaseMethodDescriptor> GetMethods()noexcept
			{
				static constexpr std::array<nox::ServicePhaseMethodDescriptor, sizeof...(Phases)> kMethods{
					nox::detail::ServicePhaseTraits<T, Phases>::MakeDescriptor()...
				};
				return std::span<const nox::ServicePhaseMethodDescriptor>(kMethods.data(), kMethods.size());
			}
		};

		/// @brief Service の宣言の検査。不備ごとにコンパイルエラーを切り分けて出す。
		/// @details NOX_ECS_DECLARE_VERIFY を書けば定義した時点で、書かなくても生成コードが
		///          記述子を実体化する時点で走る。
		template<class T>
		[[nodiscard]] consteval bool VerifyServiceDeclaration()noexcept
		{
			static_assert(std::is_base_of_v<nox::Service<T>, T>,
				"nox::Service<T> の T には、継承する Service 自身を指定してください");
			static_assert(std::is_final_v<T>,
				"Service には final を付けてください(派生の派生は購読されず、型による同一性も崩れるため)");
			static_assert(std::is_polymorphic_v<T> == false,
				"Service は仮想関数を持てません");
			static_assert(std::is_default_constructible_v<T>,
				"Service はデフォルト構築できる必要があります(World が生成するため)");
			static_assert(requires { T::kPhaseList; },
				"Service には public な static constexpr auto kPhaseList = PhaseRegister{ ... }; を定義してください"
				"(フェーズが無ければ PhaseRegister{})");

			if constexpr (requires { T::kPhaseList; })
			{
				using PhaseListType = std::remove_cv_t<decltype(T::kPhaseList)>;
				static_assert(nox::detail::is_service_phase_list_v<PhaseListType>,
					"kPhaseList は PhaseRegister{ ... } で定義してください");

				if constexpr (nox::detail::is_service_phase_list_v<PhaseListType>)
				{
					static_assert(nox::detail::ServicePhaseMethodTable<T, PhaseListType>::Verify());
				}
			}
			return true;
		}
	}

	/// @brief Worldが所有する共有機能の基底。
	/// @details 生成も破棄も World が行う(nox::ServiceTypeDescriptor)。仮想関数は持たない。
	/// @tparam T CRTP の派生型。final を付けること。
	template<class T>
	class Service : public nox::ServiceBase
	{
	protected:
		constexpr Service()noexcept = default;
		~Service() = default;

		Service(const Service&) = delete;
		Service& operator=(const Service&) = delete;

		/// @brief 宣言の検査。NOX_ECS_DECLARE_VERIFY から呼ばれ、定義した時点でコンパイルエラーにする。
		[[nodiscard]] static consteval bool StaticDeclareVerify()noexcept
		{
			return nox::detail::VerifyServiceDeclaration<T>();
		}
	};

	/// @brief Service 型の記述子を作る。
	template<class T>
	[[nodiscard]] constexpr nox::ServiceTypeDescriptor MakeServiceTypeDescriptor()noexcept
	{
		static_assert(nox::detail::VerifyServiceDeclaration<T>());

		using PhaseListType = std::remove_cv_t<decltype(T::kPhaseList)>;
		return nox::ServiceTypeDescriptor{
			.construct = [](void* const memory) -> void* { return new(memory) T(); },
			.destruct = [](void* const instance)noexcept { static_cast<T*>(instance)->~T(); },
			.get_phase_methods = &nox::detail::ServicePhaseMethodTable<T, PhaseListType>::GetMethods,
			.type = &nox::reflection::Typeof<T>(),
			.instance_size = static_cast<nox::uint32>(sizeof(T)),
			.instance_alignment = static_cast<nox::uint32>(alignof(T)),
			.name = nox::util::GetTypeName<T>(),
		};
	}

	/// @brief Service 型ごとの記述子の実体。
	/// @details 生成コードが型ごとの .cpp で明示的実体化し、その .rdata へ置く。
	///          テストは生成コードを介さず、このアドレスを並べて nox::World::CreateServices へ渡せる。
	template<class T>
	inline constexpr nox::ServiceTypeDescriptor kServiceTypeDescriptor = nox::MakeServiceTypeDescriptor<T>();
}
