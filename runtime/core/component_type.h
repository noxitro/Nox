// Copyright (C) 2026 NOX ENGINE All rights reserved.

/// @file	component_type.h
/// @brief	ComponentDataの型情報とdense indexの割り当て。
#pragma once
#include	"component.h"

namespace nox
{
	/// @brief ComponentDataに割り当てられる密なインデックス。ComponentMaskのビット位置を兼ねる。
	using ComponentTypeIndex = nox::uint16;

	/// @brief 1ワールドが扱えるComponentData型の上限。ComponentMaskの幅を決める。
	inline constexpr nox::uint32 k_max_component_type_count = 256u;

	inline constexpr nox::ComponentTypeIndex k_invalid_component_type_index =
		std::numeric_limits<nox::ComponentTypeIndex>::max();

	/// @brief ComponentDataの静的な型情報。プロセス内で1型1インスタンス。
	/// @details アドレスがそのまま型の同一性を表す。
	struct ComponentTypeInfo final
	{
		/// @brief 1要素のサイズ
		nox::uint32 size;
		/// @brief 1要素のアラインメント
		nox::uint32 alignment;
		/// @brief 登録順に振られる密なインデックス
		nox::ComponentTypeIndex index;
		/// @brief 型名(デバッグ・エディタ表示用)
		std::string_view name;
	};

	/// @brief ComponentData型の集合をビットで表す。Archetypeの同一性判定とQueryの照合に使う。
	/// @details ゼロアロケーション。ワード数はk_max_component_type_countから決まる。
	class ComponentMask final
	{
	public:
		static constexpr nox::uint32 k_bits_per_word = 64u;
		static constexpr nox::uint32 k_word_count = k_max_component_type_count / k_bits_per_word;

		[[nodiscard]] inline constexpr ComponentMask()noexcept : words_{} {}

		inline constexpr void Set(const nox::ComponentTypeIndex index)noexcept
		{
			words_[index / k_bits_per_word] |= (1ull << (index % k_bits_per_word));
		}

		/// @brief otherのビットを取り込む(和集合)。
		inline constexpr void Merge(const ComponentMask& other)noexcept
		{
			for (nox::uint32 word_index = 0u; word_index < k_word_count; ++word_index)
			{
				words_[word_index] |= other.words_[word_index];
			}
		}

		inline constexpr void Reset(const nox::ComponentTypeIndex index)noexcept
		{
			words_[index / k_bits_per_word] &= ~(1ull << (index % k_bits_per_word));
		}

		[[nodiscard]] inline constexpr bool Test(const nox::ComponentTypeIndex index)const noexcept
		{
			return (words_[index / k_bits_per_word] & (1ull << (index % k_bits_per_word))) != 0ull;
		}

		[[nodiscard]] inline constexpr bool IsEmpty()const noexcept
		{
			for (nox::uint32 word_index = 0u; word_index < k_word_count; ++word_index)
			{
				if (words_[word_index] != 0ull)
				{
					return false;
				}
			}
			return true;
		}

		/// @brief otherのビットを全て含むか。Queryのarchetype照合はこれ1つで済む。
		[[nodiscard]] inline constexpr bool Contains(const ComponentMask& other)const noexcept
		{
			for (nox::uint32 word_index = 0u; word_index < k_word_count; ++word_index)
			{
				if ((words_[word_index] & other.words_[word_index]) != other.words_[word_index])
				{
					return false;
				}
			}
			return true;
		}

		/// @brief 共通のビットを持つか。Systemの依存解析で使う。
		[[nodiscard]] inline constexpr bool Intersects(const ComponentMask& other)const noexcept
		{
			for (nox::uint32 word_index = 0u; word_index < k_word_count; ++word_index)
			{
				if ((words_[word_index] & other.words_[word_index]) != 0ull)
				{
					return true;
				}
			}
			return false;
		}

		/// @brief 立っているビットのインデックスだけを昇順で列挙する。
		/// @details ワード単位で下位ビットから舐めるため、走査コストは立っているビット数に比例する。
		///          確保も間接呼び出しも走らない(呼び出し側のラムダはインライン展開される)。
		template<class Function>
		inline constexpr void ForEachIndex(Function&& function)const noexcept
		{
			for (nox::uint32 word_index = 0u; word_index < k_word_count; ++word_index)
			{
				nox::uint64 word = words_[word_index];
				while (word != 0ull)
				{
					const nox::uint32 bit_index = static_cast<nox::uint32>(std::countr_zero(word));
					word &= (word - 1ull);
					function(static_cast<nox::ComponentTypeIndex>((word_index * k_bits_per_word) + bit_index));
				}
			}
		}

		[[nodiscard]] inline constexpr bool operator==(const ComponentMask& other)const noexcept
		{
			for (nox::uint32 word_index = 0u; word_index < k_word_count; ++word_index)
			{
				if (words_[word_index] != other.words_[word_index])
				{
					return false;
				}
			}
			return true;
		}

		[[nodiscard]] inline constexpr nox::uint64 GetHash()const noexcept
		{
			nox::uint64 hash = 0xcbf29ce484222325ull;
			for (nox::uint32 word_index = 0u; word_index < k_word_count; ++word_index)
			{
				hash = (hash ^ words_[word_index]) * 0x100000001b3ull;
			}
			return hash;
		}

	private:
		std::array<nox::uint64, k_word_count> words_;
	};

	namespace detail
	{
		/// @brief ComponentTypeIndexを登録順に払い出す。ヒープ確保なし。
		[[nodiscard]] nox::ComponentTypeIndex AcquireComponentTypeIndex()noexcept;

		/// @brief 登録済みのComponentTypeInfoを索引する。indexが未登録ならnullptr。
		[[nodiscard]] const nox::ComponentTypeInfo* TryGetComponentTypeInfo(nox::ComponentTypeIndex index)noexcept;

		void RegisterComponentTypeInfo(const nox::ComponentTypeInfo& info)noexcept;
	}

	/// @brief ComponentData型の静的型情報を取得する。初回アクセス時に一度だけ登録される。
	template<class T>
		requires(nox::IsComponentDataType<T>())
	[[nodiscard]] inline const nox::ComponentTypeInfo& ComponentTypeOf()noexcept
	{
		static const nox::ComponentTypeInfo info = []() noexcept
			{
				const nox::ComponentTypeInfo created{
					.size = static_cast<nox::uint32>(sizeof(T)),
					.alignment = static_cast<nox::uint32>(alignof(T)),
					.index = nox::detail::AcquireComponentTypeIndex(),
					.name = nox::util::GetTypeName<T>(),
				};
				return created;
			}();
		[[maybe_unused]] static const bool registered = []() noexcept
			{
				nox::detail::RegisterComponentTypeInfo(info);
				return true;
			}();
		return info;
	}

	template<class T>
		requires(nox::IsComponentDataType<T>())
	[[nodiscard]] inline nox::ComponentTypeIndex ComponentTypeIndexOf()noexcept
	{
		return nox::ComponentTypeOf<T>().index;
	}

	/// @brief 指定した型集合からComponentMaskを構築する。
	template<class... ComponentTypes>
		requires((nox::IsComponentDataType<std::remove_cvref_t<ComponentTypes>>() && ...))
	[[nodiscard]] inline nox::ComponentMask MakeComponentMask()noexcept
	{
		nox::ComponentMask mask{};
		(mask.Set(nox::ComponentTypeIndexOf<std::remove_cvref_t<ComponentTypes>>()), ...);
		return mask;
	}
}
